#include <stdio.h>
#include <stdlib.h>
#include "api.h"

/* These come from lex/yacc */
extern int yyparse(void);
extern FILE *yyin;
extern int parse_failed;

/* -----------------------------
   API FUNCTION
   ----------------------------- */
int compile_input(const char *filename)
{
    yyin = fopen(filename, "r");
    if (!yyin)
    {
        perror("[COMPILER_API] Cannot open input file");
        return 1;
    }

    parse_failed = 0;

    printf("[COMPILER_API] Parsing %s...\n", filename);

    int result = yyparse();

    fclose(yyin);
    yyin = NULL;

    if (result == 0 && !parse_failed)
    {
        printf("[COMPILER_API] Parsing succeeded.\n");
        return 0;
    }
    else
    {
        printf("[COMPILER_API] Parsing failed.\n");
        return 2;
    }
}