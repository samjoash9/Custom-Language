#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "headers/lexical_analyzer.h"

// Global token array and count
TOKEN tokens[MAX_TOKENS];
int token_count = 0;

// Check if token is a valid datatype
int is_datatype(const char *token)
{
    return (strcmp(token, "int") == 0 || strcmp(token, "char") == 0);
}

// Check if character can start an operator
int is_operator_char(char c)
{
    return (c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '!' || c == '<' || c == '>');
}

// Check if character is a delimiter
int is_delimeter(char c)
{
    return (c == ';' || c == ',');
}

// Add lexeme to tokens array
void add_to_tokens(char *lexeme, TokenType token_type)
{
    if (token_count < MAX_TOKENS)
    {
        tokens[token_count].type = token_type;
        strcpy(tokens[token_count].lexeme, lexeme);
        token_count++;
    }
}

// Extract datatype or identifier token
void get_token_as_dt_or_id(const char *source, char *target, int *s_iterator, int *t_iterator)
{
    while (source[*s_iterator] != '\0' &&
           (isalnum((unsigned char)source[*s_iterator]) || source[*s_iterator] == '_'))
    {
        target[*t_iterator] = source[*s_iterator];
        (*t_iterator)++;
        (*s_iterator)++;
    }
    target[*t_iterator] = '\0';
    (*s_iterator)--; // step back since main loop increments
}

// Extract integer literal (with optional unary + or -)
void get_token_as_value(const char *source, char *target, int *s_iterator, int *t_iterator)
{
    // Optional sign (+ or -)
    if (source[*s_iterator] == '+' || source[*s_iterator] == '-')
    {
        target[*t_iterator] = source[*s_iterator];
        (*t_iterator)++;
        (*s_iterator)++;
    }

    while (isdigit((unsigned char)source[*s_iterator]))
    {
        target[*t_iterator] = source[*s_iterator];
        (*t_iterator)++;
        (*s_iterator)++;
    }
    target[*t_iterator] = '\0';
    (*s_iterator)--; // backtrack one since for-loop will ++
}

// Reset temporary token buffer
void reset_tokens(int *iterator, char *temp_token)
{
    *iterator = 0;
    temp_token[0] = '\0';
}

// Convert TokenType to string
const char *token_type_to_string(TokenType type)
{
    switch (type)
    {
    case TOK_DATATYPE:
        return "DATATYPE";
    case TOK_IDENTIFIER:
        return "IDENTIFIER";
    case TOK_INT_LITERAL:
        return "INT_LITERAL";
    case TOK_CHAR_LITERAL:
        return "CHAR_LITERAL";
    case TOK_OPERATOR:
        return "OPERATOR";
    case TOK_DELIMITER:
        return "DELIMITER";
    case TOK_PARENTHESIS:
        return "PARENTHESIS";
    default:
        return "UNKNOWN";
    }
}

// Display all tokens
void display_tokens()
{
    printf("----- TOKENS -----\n");
    for (int i = 0; i < token_count; i++)
    {
        printf("Token %d: %-12s | %s\n",
               i + 1,
               token_type_to_string(tokens[i].type),
               tokens[i].lexeme);
    }
    printf("------------------\n");
}

// Skip single-line comment
void skip_single_line_comment(const char *src, int *i)
{
    *i += 2;
    while (src[*i] != '\0' && src[*i] != '\n')
        (*i)++;
}

// Skip multi-line comment
void skip_multi_line_comment(const char *src, int *i)
{
    *i += 2;
    while (src[*i] != '\0' && !(src[*i] == '*' && src[*i + 1] == '/'))
        (*i)++;
    if (src[*i] != '\0')
        *i += 1; // skip final '/'
}

// Main lexer function
void lexer(const char *src)
{
    int len = strlen(src);
    char temp_token[MAX_BUFFER_LEN];
    int t_iter = 0;

    for (int i = 0; i < len; i++)
    {
        char c = src[i];

        // Skip whitespace
        if (isspace((unsigned char)c))
            continue;

        // Skip comments
        if (c == '/' && src[i + 1] == '/')
        {
            skip_single_line_comment(src, &i);
            continue;
        }
        else if (c == '/' && src[i + 1] == '*')
        {
            skip_multi_line_comment(src, &i);
            continue;
        }

        // Datatype or identifier
        if (isalpha((unsigned char)c) || c == '_')
        {
            get_token_as_dt_or_id(src, temp_token, &i, &t_iter);
            if (is_datatype(temp_token))
                add_to_tokens(temp_token, TOK_DATATYPE);
            else
                add_to_tokens(temp_token, TOK_IDENTIFIER);
            reset_tokens(&t_iter, temp_token);
            continue;
        }

        // Integer literal (with possible unary + or -)
        if (isdigit((unsigned char)c) ||
            ((c == '+' || c == '-') && isdigit((unsigned char)src[i + 1])))
        {
            get_token_as_value(src, temp_token, &i, &t_iter);
            add_to_tokens(temp_token, TOK_INT_LITERAL);
            reset_tokens(&t_iter, temp_token);
            continue;
        }

        // Character literal
        if (c == '\'')
        {
            i++;
            if (src[i] != '\0')
            {
                char ch = src[i];
                if (ch == '\\')
                {
                    i++;
                    ch = src[i]; // handle escape
                }
                i++; // move to closing quote
                if (src[i] == '\'')
                {
                    temp_token[0] = ch;
                    temp_token[1] = '\0';
                    add_to_tokens(temp_token, TOK_CHAR_LITERAL);
                }
            }
            continue;
        }

        // Parentheses
        if (c == '(' || c == ')')
        {
            temp_token[0] = c;
            temp_token[1] = '\0';
            add_to_tokens(temp_token, TOK_PARENTHESIS);
            continue;
        }

        // Delimiters
        if (is_delimeter(c))
        {
            temp_token[0] = c;
            temp_token[1] = '\0';
            add_to_tokens(temp_token, TOK_DELIMITER);
            continue;
        }

        // Operators (single or compound)
        if (is_operator_char(c))
        {
            char next = src[i + 1];
            if ((next == '=' && (c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '!')) ||
                (c == '+' && next == '+') || (c == '-' && next == '-'))
            {
                temp_token[0] = c;
                temp_token[1] = next;
                temp_token[2] = '\0';
                add_to_tokens(temp_token, TOK_OPERATOR);
                i++;
            }
            else
            {
                temp_token[0] = c;
                temp_token[1] = '\0';
                add_to_tokens(temp_token, TOK_OPERATOR);
            }
            continue;
        }

        printf("Lexer Warning: Unknown symbol '%c'\n", c);
    }

    display_tokens();
}
