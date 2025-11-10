#ifndef LEXICAL_ANALYZER_H
#define LEXICAL_ANALYZER_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "symbol_table.h"

#define MAX_TOKENS 4096
#define MAX_BUFFER_LEN 256
#define MAX_VALUE_LENGTH 256

// === TOKEN TYPES ===
typedef enum
{
    TOK_DATATYPE,
    TOK_IDENTIFIER,
    TOK_INT_LITERAL,
    TOK_CHAR_LITERAL,
    TOK_ASSIGN,
    TOK_OPERATOR,
    TOK_COMPOUND_ASSIGN,
    TOK_DELIMITER,
    TOK_PARENTHESIS,
    TOK_UNKNOWN
} TokenType;

// === TOKEN STRUCT ===
typedef struct
{
    TokenType type;
    char lexeme[MAX_VALUE_LENGTH];
} TOKEN;

// === GLOBALS ===
extern TOKEN tokens[MAX_TOKENS];
extern int token_count;
extern int error_found;

// === FUNCTION DECLARATIONS ===
int lexer(const char *source_code);
int is_datatype(const char *token);
int is_delimiter(char c);
int is_operator_char(char c);
void reset_tokens(int *iterator, char *temp_token);
void get_token_as_dt_or_id(const char *source, char *target, int *s_iterator, int *t_iterator);
void get_token_as_value(const char *source, char *target, int *s_iterator, int *t_iterator);
void skip_single_line_comment(const char *source_code, int *i);
int skip_multi_line_comment(const char *src, int *i);
void display_tokens();
const char *token_type_to_string(TokenType type);

#endif
