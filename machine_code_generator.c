#include "headers/machine_code_generator.h"

int start_code_counter = 0;

const char *R_TYPE[R_TYPE_COUNT] = {"daddu", "dsub", "dmult", "ddiv", "mflo"};
const char *I_TYPE[I_TYPE_COUNT] = {"daddiu", "ld", "sd"};


typedef struct {
    char label[32];
    int address;
} DataSymbol;

DataSymbol data_symbols[100];
int data_symbol_count = 0;
int current_data_address = 0xFFF8; // base address for .data section 


void trim(char *str)
{
    char *end;
    while (isspace((unsigned char)*str))
        str++; // skip leading spaces
    if (*str == 0)
        return; // all spaces
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--; // trim trailing
    *(end + 1) = 0;
}


void remove_data_and_code_section()
{
    int i = 0;
    while (i < assembly_code_count)
    {   

        char *line = assembly_code[i].assembly;

        char label[32];
        if (sscanf(line, "%31[^:]:", label) == 1) {
            trim(label);
            strcpy(data_symbols[data_symbol_count].label, label);
            data_symbols[data_symbol_count].address = current_data_address;
            data_symbol_count++;
            current_data_address += 8; // each .word takes 8 bytes
        }

        if (strstr(assembly_code[i].assembly, ".code") != NULL)
        {
            start_code_counter = i + 1;
            break;
        }
        i++;
    }

    int j = 0;
    for (i = start_code_counter; i < assembly_code_count; i++, j++)
    {
        strcpy(assembly_code[j].assembly, assembly_code[i].assembly);
    }
    assembly_code_count = j;
}

void convert_to_binary(int num, int bits, char *output)
{
    output[bits] = '\0';
    for (int i = bits - 1; i >= 0; i--)
    {
        output[i] = (num & 1) ? '1' : '0'; // logical comparison for each bit
        num >>= 1;
    }
}

int parse_register(const char *token)
{
    if (token[0] == 'r' || token[0] == 'R')
        return atoi(token + 1);
    return 0;
}

int get_opcode(const char *mnemonic)
{
    if (strcmp(mnemonic, "daddu") == 0)
        return 0x00;
    if (strcmp(mnemonic, "dsub") == 0)
        return 0x00;
    if (strcmp(mnemonic, "dmult") == 0)
        return 0x00;
    if (strcmp(mnemonic, "ddiv") == 0)
        return 0x00;
    if (strcmp(mnemonic, "mflo") == 0)
        return 0x00;
    if (strcmp(mnemonic, "daddiu") == 0)
        return 0x19;
    if (strcmp(mnemonic, "ld") == 0)
        return 0x37;
    if (strcmp(mnemonic, "sd") == 0)
        return 0x3F;
    return 0;
}

int get_funct(const char *mnemonic)
{
    if (strcmp(mnemonic, "daddu") == 0)
        return 0x2D;
    if (strcmp(mnemonic, "dsub") == 0)
        return 0x2E;
    if (strcmp(mnemonic, "dmult") == 0)
        return 0x1C;
    if (strcmp(mnemonic, "ddiv") == 0)
        return 0x1E;
    if (strcmp(mnemonic, "mflo") == 0)
        return 0x12;
    return 0;
}


void convert_to_machine_code() {
    
    char mnemonic[32], operands[128];
    char bin_opcode[7], bin_rs[6], bin_rt[6], bin_rd[6], bin_shamt[6], bin_funct[7];
    char bin_imm[17];

    // to hold the full binary string
    char full_bin[40];

    for (int i = 0; i < assembly_code_count; i++)
    {
        assembly_code[i].assembly[strcspn(assembly_code[i].assembly, "\n")] = '\0';
        if (strstr(assembly_code[i].assembly, ";"))
            continue; // skip comments

        if (sscanf(assembly_code[i].assembly, "%31s %[^\n]", mnemonic, operands) < 1)
            continue;

        trim(operands);

        int opcode = get_opcode(mnemonic);
        int funct = get_funct(mnemonic);

        int rs = 0, rt = 0, rd = 0, imm = 0;
        char *tok;

        // --- Tokenize Operands ---
        if (strcmp(mnemonic, "mflo") == 0){ //SPECIAL R-type (Release 5)
            tok = strtok(operands, ", ");
            if (tok)
                rd = parse_register(tok);
        }
        else if (strcmp(mnemonic, "dmult") == 0 || strcmp(mnemonic, "ddiv") == 0){ //Special R-type (Release 5)
            tok = strtok(operands, ", ");
            if (tok)
                rs = parse_register(tok);
            tok = strtok(NULL, ", ");
            if (tok)
                rt = parse_register(tok);
        }
        else if (opcode == 0x00){ // R-type
            tok = strtok(operands, ", ");
            if (tok)
                rd = parse_register(tok);
            tok = strtok(NULL, ", ");
            if (tok)
                rs = parse_register(tok);
            tok = strtok(NULL, ", ");
            if (tok)
                rt = parse_register(tok);
        }
        else{ // I-type
            tok = strtok(operands, ", ");
            if (tok)
                rt = parse_register(tok);
            tok = strtok(NULL, ", ");
            if (tok)
            {
                // check for label(offset)
                char *paren = strchr(tok, '(');
                if (paren)
                {
                    *paren = '\0';
                    trim(tok);
                    char *base_reg = paren + 1;
                    base_reg[strcspn(base_reg, ")")] = 0;
                    rs = parse_register(base_reg);

                    // --- Check if tok is a label ---
                    int found = 0;
                    for (int d = 0; d < data_symbol_count; d++) {
                        if (strcmp(tok, data_symbols[d].label) == 0) {
                            imm = data_symbols[d].address;
                            found = 1;
                            break;
                        }
                    }
                    if (!found) imm = atoi(tok); // fallback to numeric
                }
                else {
                    rs = parse_register(tok);
                    tok = strtok(NULL, ", ");
                    if (tok) imm = atoi(tok);
                }
            }
        }

        // --- Convert Fields to Binary ---
        convert_to_binary(opcode, 6, bin_opcode);
        convert_to_binary(rs, 5, bin_rs);
        convert_to_binary(rt, 5, bin_rt);
        convert_to_binary(rd, 5, bin_rd);
        convert_to_binary(0, 5, bin_shamt);
        convert_to_binary(funct, 6, bin_funct);
        convert_to_binary(imm & 0xFFFF, 16, bin_imm);

        // --- Form the Full 32-bit String ---
        if (opcode == 0x00)
            snprintf(full_bin, sizeof(full_bin), "%s %s %s %s %s %s", bin_opcode, bin_rs, bin_rt, bin_rd, bin_shamt, bin_funct);
        else
            snprintf(full_bin, sizeof(full_bin), "%s %s %s %s", bin_opcode, bin_rs, bin_rt, bin_imm);


        // --- Convert Binary String to Hexadecimal ---
        unsigned int full_bin_value = 0;
        for (int j = 0; j < strlen(full_bin); j++) {
            if (full_bin[j] == '1') {
                full_bin_value = (full_bin_value << 1) | 1;
            } else if (full_bin[j] == '0') {
                full_bin_value <<= 1;
            }
        }   

        // --- Output Binary + Hex ---
        printf("%-20s\t->\t%s\t(0x%08X)\n", assembly_code[i].assembly, full_bin, full_bin_value);
    }
}

void generate_machine_code()
{   
    remove_data_and_code_section();
    printf("===== MACHINE CODE =====\n");
    convert_to_machine_code();
    printf("===== MACHINE CODE END =====\n");
}
