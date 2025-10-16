#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// include header files
#include "headers/symbol_table.h"
#include "headers/lexical_analyzer.h"
#include "headers/syntax_analyzer.h"

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

    // === STEP 3: DISPLAY SYMBOL TABLE (AFTER PARSING) ===
    printf("\n===== SYMBOL TABLE (AFTER SYNTAX ANALYSIS) =====\n");
    display_symbol_table();

    // Free memory
    free(source_code);

    return 0;
}
