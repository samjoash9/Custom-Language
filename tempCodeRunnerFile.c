#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// include header files
#include "headers/symbol_table.h"
#include "headers/lexical_analyzer.h"

int main()
{
    // MAIN READS THE ENTIRE SOURCE CODE
    FILE *fp = fopen("input.txt", "r");

    // check for file input
    if (fp == NULL)
    {
        printf("Error: unable to open file.\n");
        return 1;
    }

    // determine the length of the source_code code
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
    printf("%s\n", source_code);
    fclose(fp);

    // STEP 1: LEXICAL ANALYSIS
    printf("%d\n", is_datatype("int"));

    // STEP 2: SYNTAX ANALYSIS
    // call syntax analyzer to process output of lexer? and store it in a variable?

    free(source_code);
    return 0;
}
