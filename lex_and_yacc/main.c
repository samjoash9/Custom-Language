#include <stdio.h>
#include <stdlib.h>

/* Declarations from Lex/Yacc */
extern int yyparse(void);
extern FILE *yyin;
extern int parse_failed;

int main(void)
{
    // SAMPLE MAIN

    /* Always read from input.txt */
    yyin = fopen("input.txt", "r");
    if (!yyin)
    {
        perror("Cannot open input.txt");
        return 1;
    }

    printf("[MAIN] Starting parser using input.txt...\n");

    /* Call the parser */
    int result = yyparse();

    if (result == 0 && !parse_failed)
    {
        printf("[MAIN] Parse Successful!\n");
    }
    else
    {
        printf("[MAIN] Parse Failed!\n");
    }

    fclose(yyin);

    /* You can add semantic analysis, IR gen, code gen here next */
    // semantic_analysis();
    // generate_intermediate_code();
    // target_code_generator();
    // machine_code_generator();

    return 0;
}
