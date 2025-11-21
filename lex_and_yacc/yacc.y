%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yacc.tab.h"

void yyerror(const char *s);
int yylex(void);

int parse_failed = 0;   /* flag to track whether parsing failed */
%}

/* tokens (must match lex) */
%token KUAN ENTEGER CHAROT
%token PRENT
%token EXCLAM
%token PLUS_EQUAL MINUS_EQUAL DIV_EQUAL MUL_EQUAL EQUAL
%token PLUS MINUS MUL DIV
%token PLUSPLUS MINUSMINUS
%token LPAREN RPAREN COMMA
%token IDENTIFIER INT_LITERAL CHAR_LITERAL STRING_LITERAL

%start S

%%

/* ---------- Program ---------- */
S:
    STATEMENT_LIST
;

/* list of statements (can be empty) */
STATEMENT_LIST:
      STATEMENT STATEMENT_LIST
    | /* empty */
;

/* statements: everything must end with EXCLAM '!' except a bare EXCLAM */
STATEMENT:
      DECLARATION EXCLAM
    | ASSIGNMENT EXCLAM
    | SIMPLE_EXPR EXCLAM
    | PRINTING EXCLAM
    | EXCLAM
;

/* ---------- Printing ---------- */
PRINTING:
    PRENT PRINT_LIST
;

PRINT_LIST:
      PRINT_ITEM PRINT_LIST_PRIME
    | /* empty */
;

PRINT_LIST_PRIME:
      COMMA PRINT_ITEM PRINT_LIST_PRIME
    | /* empty */
;

PRINT_ITEM:
      SIMPLE_EXPR
    | STRING_LITERAL
;

/* ---------- Declarations ---------- */
DECLARATION:
    DATATYPE INIT_DECLARATOR_LIST
;

DATATYPE:
      CHAROT
    | ENTEGER
    | KUAN
;

INIT_DECLARATOR_LIST:
      INIT_DECLARATOR
    | INIT_DECLARATOR COMMA INIT_DECLARATOR_LIST
;

INIT_DECLARATOR:
      DECLARATOR
    | DECLARATOR EQUAL SIMPLE_EXPR
;

DECLARATOR:
      IDENTIFIER
    | LPAREN DECLARATOR RPAREN
;

/* ---------- Assignment ---------- */
ASSIGNMENT:
      IDENTIFIER ASSIGN_OP ASSIGNMENT
    | IDENTIFIER ASSIGN_OP SIMPLE_EXPR
;

ASSIGN_OP:
      EQUAL
    | PLUS_EQUAL
    | MINUS_EQUAL
    | DIV_EQUAL
    | MUL_EQUAL
;

/* ---------- Expressions ---------- */
SIMPLE_EXPR:
    ADD_EXPR
;

/* Addition/subtraction */
ADD_EXPR:
      ADD_EXPR PLUS TERM
    | ADD_EXPR MINUS TERM
    | TERM
;

/* Multiplication/division */
TERM:
      TERM MUL FACTOR
    | TERM DIV FACTOR
    | FACTOR
;

/* ---------- FIXED UNARY & POSTFIX RULES ---------- */
/* Two main types: UNARY and POSTFIX */

FACTOR:
      UNARY
    | POSTFIX
;

/* Unary operators:
   +FACTOR, -FACTOR behave normally (chainable)
   ++POSTFIX, --POSTFIX: only ONE prefix ++/-- allowed
*/
UNARY:
      PLUS FACTOR
    | MINUS FACTOR
    | PLUSPLUS POSTFIX
    | MINUSMINUS POSTFIX
;

/* POSTFIX can have AT MOST ONE postfix op */
POSTFIX:
      PRIMARY POSTFIX_OPT
;

POSTFIX_OPT:
      /* empty */
    | PLUSPLUS
    | MINUSMINUS
;

/* Primary atoms */
PRIMARY:
      IDENTIFIER
    | INT_LITERAL
    | CHAR_LITERAL
    | LPAREN SIMPLE_EXPR RPAREN
;

%%

void yyerror(const char *s) {
    parse_failed = 1;
    fprintf(stderr, "[PARSE] Rejected (%s)\n", s ? s : "syntax error");
}
