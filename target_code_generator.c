#include "headers/target_code_generator.h"

Register registers[MAX_REGISTERS];
Data data_storage[MAX_DATA];
int data_count = 0;

int assembly_code_count = 0;
ASSEMBLY assembly_code[MAX_ASSEMBLY_CODE];

void add_assembly_line(const char *format, ...)
{
    if (assembly_code_count >= MAX_ASSEMBLY_CODE)
        return;

    va_list args;
    va_start(args, format);
    vsprintf(assembly_code[assembly_code_count++].assembly, format, args);
    va_end(args);
}

void display_assembly_code()
{   
    printf("===== ASSEMBLY CODE =====\n");
    for (int i = 0; i < assembly_code_count; i++)
        printf("%s", assembly_code[i].assembly);
    printf("===== ASSEMBLY CODE END =====\n\n");
}

void initialize_registers()
{
    for (int i = 0; i < MAX_REGISTERS; i++)
    {
        sprintf(registers[i].name, "r%d", i + 1);
        registers[i].used = 0;
        registers[i].assigned_temp[0] = '\0';

        // debug
        // printf("%s\n", registers[i].name);
    }
}

void add_to_data_storage(char *data)
{
    strcpy(data_storage[data_count++].data, data);
}

void display_data_storage()
{
    printf("The data storage: ");

    for (int i = 0; i < data_count; i++)
    {
        printf("%s ", data_storage[i].data);
    }
}

int is_in_data_storage(char *data)
{
    if (data_count == 0)
        return 0;

    for (int i = 0; i < data_count; i++)
    {
        if (strcmp(data_storage[i].data, data) == 0)
            return 1;
    }

    return 0;
}

Register *get_available_register()
{
    for (int i = 0; i < MAX_REGISTERS; i++)
    {
        if (registers[i].used == 0)
        {
            return &registers[i];
        }
    }

    return NULL;
}

int is_tac_temporary(char *tac)
{
    if (tac[0] == 't' && isdigit(tac[1]))
        return 1;

    return 0;
}

Register *find_temp_reg(char *temp)
{
    for (int i = 0; i < MAX_REGISTERS; i++)
    {
        if (registers[i].used && strcmp(registers[i].assigned_temp, temp) == 0)
            return &registers[i];
    }

    return NULL;
}

void generate_data_section()
{
    add_assembly_line(".data\n");

    for (int i = 0; i < symbol_count; i++)
    {
        add_assembly_line("%s: .word64 0\n", symbol_table[i].name);
        add_to_data_storage(symbol_table[i].name);
    }
}

int is_digit(char *value)
{
    // Check for NULL or empty string
    if (value == NULL || *value == '\0')
        return 0;

    // Allow optional leading '-'
    if (*value == '-')
        value++;

    // After '-', there must be at least one digit
    if (!*value)
        return 0;

    // Check that all remaining characters are digits
    while (*value)
    {
        if (!isdigit(*value))
            return 0;
        value++;
    }

    return 1;
}

void display_tac_as_comment(TACInstruction ins)
{
    if (strlen(ins.arg2) == 0)
        add_assembly_line("; %s = %s\n", ins.result, ins.arg1);
    else
        add_assembly_line("; %s = %s %s %s\n", ins.result, ins.arg1, ins.op, ins.arg2);
}

void perform_operation(char *result, char *arg1, char *op, char *arg2, Register *reg1, Register *reg2, Register *reg3, int is_for_temporary)
{
    // determine operation
    if (strcmp(op, "+") == 0)
    {
        add_assembly_line("daddu %s, %s, %s\n", reg3->name, reg1->name, reg2->name);
    }
    else if (strcmp(op, "-") == 0)
    {
        add_assembly_line("dsub %s, %s, %s\n", reg3->name, reg1->name, reg2->name);
    }
    else if (strcmp(op, "*") == 0)
    {
        add_assembly_line("dmult %s, %s\n", reg1->name, reg2->name);
        add_assembly_line("mflo %s\n", reg3->name);
    }
    else if (strcmp(op, "/") == 0)
    {
        add_assembly_line("ddiv %s, %s\n", reg1->name, reg2->name);
        add_assembly_line("mflo %s\n", reg3->name);
    }

    if (!is_for_temporary)
    {
        add_assembly_line("sd %s, %s(r0)\n", reg3->name, result);

        // reset registers
        initialize_registers();
    }
    else
        strcpy(reg3->assigned_temp, result);
}

void generate_code_section()
{
    add_assembly_line("\n.code\n");

    for (int i = 0; i < optimizedCount; i++)
    {
        TACInstruction ins = optimizedCode[i];
        display_tac_as_comment(ins);

        // case 1 : assignment only
        if (strlen(ins.arg2) == 0)
        {
            // case 1 : variable = constant (for constant: check if positive or negative)
            if (is_in_data_storage(ins.result) &&
                (isdigit(ins.arg1[0]) || (ins.arg1[0] == '-' && isdigit(ins.arg1[1]))))

            {
                Register *reg = get_available_register();
                reg->used = 1;

                add_assembly_line("daddiu %s, r0, %s\n", reg->name, ins.arg1);
                add_assembly_line("sd %s, %s(r0)\n", reg->name, ins.result);

                reg->used = 0;
            }
            // case 2 : variable = variable
            else if (is_in_data_storage(ins.result) && is_in_data_storage(ins.arg1))
            {
                Register *arg1_val_reg = get_available_register();
                arg1_val_reg->used = 1;

                add_assembly_line("ld %s, %s(r0)\n", arg1_val_reg->name, ins.arg1);
                add_assembly_line("sd %s, %s(r0)\n", arg1_val_reg->name, ins.result);

                arg1_val_reg->used = 0;
            }
            // case 3 : variable = temp
            else if (is_in_data_storage(ins.result) && is_tac_temporary(ins.arg1))
            {
                // find register temp
                Register *temp_reg = find_temp_reg(ins.arg1);

                add_assembly_line("sd %s, %s(r0)\n", temp_reg->name, ins.result);
            }
            // case 4 : temp = variable
            else if (is_tac_temporary(ins.result) && is_in_data_storage(ins.arg1))
            {
                Register *var_reg = get_available_register();
                Register *temp_reg = find_temp_reg(ins.result);

                add_assembly_line("ld %s, %s(r0)\n", var_reg->name, ins.arg1);
                add_assembly_line("daddu %s, %s, r0\n", temp_reg->name, var_reg->name);

                temp_reg->used = 1;
                strcpy(temp_reg->assigned_temp, ins.result);
            }
            // case 5 : temp = constant
            else if (is_tac_temporary(ins.result) && (isdigit(ins.arg1[0] || ins.arg1[0] == '-' && isdigit(ins.arg1[1]))))
            {
                Register *temp_reg = get_available_register();
                temp_reg->used = 1;
                strcpy(temp_reg->assigned_temp, ins.result);

                add_assembly_line("daddiu %s, r0, %s\n", temp_reg->name, ins.arg1);
            }
            // case 6 : temp = temp
            else if (is_tac_temporary(ins.result) && is_tac_temporary(ins.arg1))
            {
                // Find both temp registers
                Register *temp_res = find_temp_reg(ins.result);
                Register *temp_arg1 = find_temp_reg(ins.arg1);

                // If result temp does not yet have a register, allocate one
                if (!temp_res)
                {
                    temp_res = get_available_register();
                    temp_res->used = 1;
                    strcpy(temp_res->assigned_temp, ins.result);
                }

                // If argument temp does not exist (shouldn’t normally happen, but safe to check)
                if (!temp_arg1)
                {
                    temp_arg1 = get_available_register();
                    temp_arg1->used = 1;
                    strcpy(temp_arg1->assigned_temp, ins.arg1);
                }

                // Move value from arg1 temp into result temp
                add_assembly_line("daddu %s, %s, r0\n", temp_res->name, temp_arg1->name);
            }
        }
        // case 2 : assignment + operation
        else
        {
            Register *reg1 = get_available_register();
            reg1->used = 1;
            Register *reg2 = get_available_register();
            reg2->used = 1;
            Register *reg3 = get_available_register();
            reg3->used = 1;

            // variable = constant op constant
            if (is_in_data_storage(ins.result) && is_digit(ins.arg1) && is_digit(ins.arg2))
            {
                add_assembly_line("daddiu %s, r0, %s\n", reg1->name, ins.arg1);
                add_assembly_line("daddiu %s, r0, %s\n", reg2->name, ins.arg2);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 0);
            }
            // variable = variable op variable
            else if (is_in_data_storage(ins.result) && is_in_data_storage(ins.arg1) && is_in_data_storage(ins.arg2))
            {
                add_assembly_line("ld %s, %s(r0)\n", reg1->name, ins.arg1);
                add_assembly_line("ld %s, %s(r0)\n", reg2->name, ins.arg2);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 0);
            }
            // variable = variable op constant
            else if (is_in_data_storage(ins.result) && is_in_data_storage(ins.arg1) && is_digit(ins.arg2))
            {
                add_assembly_line("ld %s, %s(r0)\n", reg1->name, ins.arg1);
                add_assembly_line("daddiu %s, r0, %s\n", reg2->name, ins.arg2);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 0);
            }
            // variable = constant op variable
            else if (is_in_data_storage(ins.result) && is_digit(ins.arg1) && is_in_data_storage(ins.arg2))
            {
                add_assembly_line("daddiu %s, r0, %s\n", reg1->name, ins.arg1);
                add_assembly_line("ld %s, %s(r0)\n", reg2->name, ins.arg2);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 0);
            }
            // variable = temp op temp
            else if (is_in_data_storage(ins.result) && is_tac_temporary(ins.arg1) && is_tac_temporary(ins.arg2))
            {
                reg1->used = 0;
                reg2->used = 0;
                reg3->used = 0;

                reg1 = find_temp_reg(ins.arg1);
                reg2 = find_temp_reg(ins.arg2);
                reg3 = get_available_register();

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 0);
            }
            // variable = temp op variable
            else if (is_in_data_storage(ins.result) && is_tac_temporary(ins.arg1) && is_in_data_storage(ins.arg2))
            {
                reg1->used = 0;
                reg2->used = 0;
                reg3->used = 0;

                reg1 = find_temp_reg(ins.arg1);
                reg1->used = 1;
                reg2 = get_available_register();
                reg2->used = 1;
                reg3 = get_available_register();
                reg3->used = 1;

                add_assembly_line("ld %s, %s(r0)\n", reg2->name, ins.arg2);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 0);
            }
            // variable = variable op temp
            else if (is_in_data_storage(ins.result) && is_in_data_storage(ins.arg1) && is_tac_temporary(ins.arg2))
            {
                reg1->used = 0;
                reg2->used = 0;
                reg3->used = 0;

                reg1 = get_available_register();
                reg1->used = 1;
                reg2 = find_temp_reg(ins.arg2);
                reg2->used = 1;
                reg3 = get_available_register();
                reg3->used = 1;

                add_assembly_line("ld %s, %s(r0)\n", reg1->name, ins.arg1);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 0);
            }
            // variable = temp op constant
            else if (is_in_data_storage(ins.result) && is_tac_temporary(ins.arg1) && is_digit(ins.arg2))
            {
                reg1->used = 0;
                reg2->used = 0;
                reg3->used = 0;

                reg1 = find_temp_reg(ins.arg1);
                reg1->used = 1;
                reg2 = get_available_register();
                reg2->used = 1;
                reg3 = get_available_register();
                reg3->used = 1;

                add_assembly_line("daddiu %s, r0, %s\n", reg2->name, ins.arg2);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 0);
            }
            // variable = constant op temp
            else if (is_in_data_storage(ins.result) && is_digit(ins.arg1) && is_tac_temporary(ins.arg2))
            {
                reg1->used = 0;
                reg2->used = 0;
                reg3->used = 0;

                reg1 = get_available_register();
                reg1->used = 1;
                reg2 = find_temp_reg(ins.arg2);
                reg2->used = 1;
                reg3 = get_available_register();
                reg3->used = 1;

                add_assembly_line("daddiu %s, r0, %s\n", reg1->name, ins.arg1);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 0);
            }
            // temp = constant op constant
            else if (is_tac_temporary(ins.result) && is_digit(ins.arg1) && is_digit(ins.arg2))
            {
                add_assembly_line("daddiu %s, r0, %s\n", reg1->name, ins.arg1);
                add_assembly_line("daddiu %s, r0, %s\n", reg2->name, ins.arg2);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 1);
            }
            // temp = constant op temp
            else if (is_tac_temporary(ins.result) && is_digit(ins.arg1) && is_tac_temporary(ins.arg2))
            {
                reg1->used = 0;
                reg2->used = 0;
                reg3->used = 0;

                reg1 = get_available_register();
                reg1->used = 1;
                reg2 = find_temp_reg(ins.arg2);
                reg2->used = 1;
                reg3 = get_available_register();
                reg3->used = 1;

                add_assembly_line("daddiu %s, r0, %s\n", reg1->name, ins.arg1);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 1);
            }
            // temp = temp op constant
            else if (is_tac_temporary(ins.result) && is_tac_temporary(ins.arg1) && is_digit(ins.arg2))
            {
                reg1->used = 0;
                reg2->used = 0;
                reg3->used = 0;

                reg1 = find_temp_reg(ins.arg1);
                reg1->used = 1;
                reg2 = get_available_register();
                reg2->used = 1;
                reg3 = get_available_register();
                reg3->used = 1;

                add_assembly_line("daddiu %s, r0, %s\n", reg2->name, ins.arg2);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 1);
            }
            // temp = constant op variable
            else if (is_tac_temporary(ins.result) && is_digit(ins.arg1) && is_in_data_storage(ins.arg2))
            {
                add_assembly_line("daddiu %s, r0, %s\n", reg1->name, ins.arg1);
                add_assembly_line("ld %s, %s(r0)\n", reg2->name, ins.arg2);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 1);
            }
            // temp = variable op constant
            else if (is_tac_temporary(ins.result) && is_in_data_storage(ins.arg1) && is_digit(ins.arg2))
            {
                add_assembly_line("ld %s, %s(r0)\n", reg1->name, ins.arg1);
                add_assembly_line("daddiu %s, r0, %s\n", reg2->name, ins.arg2);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 1);
            }
            // temp = variable op variable
            else if (is_tac_temporary(ins.result) && is_in_data_storage(ins.arg1) && is_in_data_storage(ins.arg2))
            {
                add_assembly_line("ld %s, %s(r0)\n", reg1->name, ins.arg1);
                add_assembly_line("ld %s, %s(r0)\n", reg2->name, ins.arg2);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 1);
            }
            // temp = temp op variable
            else if (is_tac_temporary(ins.result) && is_tac_temporary(ins.arg1) && is_in_data_storage(ins.arg2))
            {
                reg1->used = 0;
                reg2->used = 0;
                reg3->used = 0;

                reg1 = find_temp_reg(ins.arg1);
                reg1->used = 1;
                reg2 = get_available_register();
                reg2->used = 1;
                reg3 = get_available_register();
                reg3->used = 1;

                add_assembly_line("ld %s, %s(r0)\n", reg2->name, ins.arg2);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 1);
            }
            // temp = variable op temp
            else if (is_tac_temporary(ins.result) && is_in_data_storage(ins.arg1) && is_tac_temporary(ins.arg2))
            {
                reg1->used = 0;
                reg2->used = 0;
                reg3->used = 0;

                reg1 = get_available_register();
                reg1->used = 1;
                reg2 = find_temp_reg(ins.arg2);
                reg2->used = 1;
                reg3 = get_available_register();
                reg3->used = 1;

                add_assembly_line("ld %s, %s(r0)\n", reg1->name, ins.arg1);

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 1);
            }
            // temp = temp op temp
            else if (is_tac_temporary(ins.result) && is_tac_temporary(ins.arg1) && is_tac_temporary(ins.arg2))
            {
                reg1->used = 0;
                reg2->used = 0;
                reg3->used = 0;

                reg1 = find_temp_reg(ins.arg1);
                reg1->used = 1;
                reg2 = find_temp_reg(ins.arg2);
                reg2->used = 1;
                reg3 = get_available_register();
                reg3->used = 1;

                perform_operation(ins.result, ins.arg1, ins.op, ins.arg2, reg1, reg2, reg3, 1);
            }
        }

        add_assembly_line("\n");
    }

    // for (int i = 0; i < MAX_REGISTERS; i++)
    // {
    //     if (registers[i].used)
    //         printf("%s (%s) = %s\n", registers[i].name, (registers[i].used ? "used" : "unused"), registers[i].assigned_temp);
    // }
}

void generate_target_code()
{
    initialize_registers();
    generate_data_section();
    generate_code_section();

    // display_data_storage();
    display_assembly_code();
}