%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "yacc.tab.h"

    extern void yyerror(const char *s);
%}

%x COMMENT
%option yylineno

%%

[ \t\r]+                  ;

"//".*                    ;

"/*"                      { BEGIN(COMMENT); }

<COMMENT>"*/"             { BEGIN(INITIAL); }
<COMMENT>.                ;
<COMMENT>\n               ;

"KUAN"      { printf("[LEX] KUAN\n"); return KUAN; }
"ENTEGER"   { printf("[LEX] ENTEGER\n"); return ENTEGER; }
"CHAROT"    { printf("[LEX] CHAROT\n"); return CHAROT; }
"PRENT"     { printf("[LEX] PRENT\n"); return PRENT; }

"+="        { printf("[LEX] PLUS_EQUAL\n"); return PLUS_EQUAL; }
"-="        { printf("[LEX] MINUS_EQUAL\n"); return MINUS_EQUAL; }
"/="        { printf("[LEX] DIV_EQUAL\n"); return DIV_EQUAL; }
"*="        { printf("[LEX] MUL_EQUAL\n"); return MUL_EQUAL; }

"++"        { printf("[LEX] PLUSPLUS\n"); return PLUSPLUS; }
"--"        { printf("[LEX] MINUSMINUS\n"); return MINUSMINUS; }

"+"         { printf("[LEX] PLUS\n"); return PLUS; }
"-"         { printf("[LEX] MINUS\n"); return MINUS; }
"*"         { printf("[LEX] MUL\n"); return MUL; }
"/"         { printf("[LEX] DIV\n"); return DIV; }
"="         { printf("[LEX] EQUAL\n"); return EQUAL; }

"!"         { printf("[LEX] EXCLAM\n"); return EXCLAM; }
"("         { printf("[LEX] LPAREN\n"); return LPAREN; }
")"         { printf("[LEX] RPAREN\n"); return RPAREN; }
","         { printf("[LEX] COMMA\n"); return COMMA; }

[0-9]+ {
    printf("[LEX] INT_LITERAL (%s)\n", yytext);
    return INT_LITERAL;
}

'([^'\\]|\\.)' {
    printf("[LEX] CHAR_LITERAL (%s)\n", yytext);
    return CHAR_LITERAL;
}

\"([^\"\\]|\\.)*\" {
    printf("[LEX] STRING_LITERAL (%s)\n", yytext);
    return STRING_LITERAL;
}

[A-Za-z_][A-Za-z0-9_]* {
    printf("[LEX] IDENTIFIER (%s)\n", yytext);
    return IDENTIFIER;
}

\n { printf("[LEX] NEWLINE\n"); }

.  {
    printf("[LEX] INVALID CHARACTER (%s)\n", yytext);
    yyerror("Invalid Character");
}

%%

int yywrap(void) {
    return 1;
}
