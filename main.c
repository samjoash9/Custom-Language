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
#include "headers/machine_code_generator.h"

int main()
{
    // === STEP 0: READ SOURCE CODE ===
    FILE *fp = fopen("input.txt", "r");
    if (fp == NULL)
    {
        printf("Error: unable to open file.\n");
        return 1;
    }

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
    if (lexer(source_code))
    {
        printf("Compilation stopped: Lexical errors found.\n");
        free(source_code);
        return 1;
    }

    // === STEP 2: SYNTAX ANALYSIS ===
    printf("\n===== SYNTAX ANALYSIS START =====\n");
    int syntax_status = syntax_analyzer();
    if (syntax_status != 0)
    {
        printf("\nCompilation aborted due to syntax error.\n");
        free(source_code);
        return 1;
    }

    // === STEP 3: SEMANTIC ANALYSIS ===
    printf("====== SEMANTIC ANALYZER ======\n");
    int semantic_status = semantic_analyzer();
    if (semantic_status != 0)
    {
        printf("\nCompilation aborted due to semantic error.\n");
        free(source_code);
        return 1;
    }
    printf("====== SEMANTIC ANALYZER END ======\n\n");

    // === STEP 4: INTERMEDIATE CODE GENERATION ===
    generate_intermediate_code(syntax_tree);

    // === STEP 5: TARGET CODE (MIPS64) ===
    generate_target_code();

    // === STEP 6: MACHINE CODE GENERATION ===
    generate_machine_code();

    // === SYMBOL TABLE ===
    printf("\n\n===== SYMBOL TABLE (AFTER ANALYSIS) =====\n");
    display_symbol_table();

    // === CLEANUP ===
    free(source_code);
    if (optimizedCode)
        free(optimizedCode);

    return 0;
}
