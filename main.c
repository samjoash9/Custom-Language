#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// include header files
#include "headers/symbol_table.h"
#include "headers/lexical_analyzer.h"
#include "headers/syntax_analyzer.h"
#include "headers/semantic_analyzer.h"
#include "headers/intermediate_code_generator.h"
#include "headers/target_code_generator.h"

int main()
{
    // MAIN READS THE ENTIRE SOURCE CODE
    FILE *fp = fopen("input.txt", "r");

    if (fp == NULL)
    {
        printf("Error: unable to open file.\n");
        return 1;
    }

    // Determine source code length
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    rewind(fp);

    char *source_code = malloc(length + 1);
    if (source_code == NULL)
    {
        printf("Error: Source code memory allocation failed.\n");
        fclose(fp);
        return 1;
    }

    size_t bytesRead = fread(source_code, 1, length, fp);
    source_code[bytesRead] = '\0';
    fclose(fp);

    // === STEP 1: LEXICAL ANALYSIS ===
    lexer(source_code);

    // === STEP 2: SYNTAX ANALYSIS ===
    syntax_analyzer();

    // === STEP 3: SEMANTIC ANALYSIS ===
    semantic_analyzer();

    // STEP 4 : INTERMEDIATE CODE GENERATOR
    generate_intermediate_code(syntax_tree);

    // STEP 5 : TARGET CODE GENERATOR (MIPS64)
    printf("\n===== MIPS64 TARGET CODE =====\n");
    generate_target_code();

    // === SYMBOL TABLE ===
    printf("\n\n===== SYMBOL TABLE (AFTER ANALYSIS) =====\n");
    display_symbol_table();

    // Free memory
    free(source_code);
    if (optimizedCode)
        free(optimizedCode);

    return 0;
}
