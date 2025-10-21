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
int is_delimiter(char c)
{
    return (c == ';' || c == ',');
}

// Add lexeme to tokens array
void add_to_tokens(const char *lexeme, TokenType token_type)
{
    if (token_count >= MAX_TOKENS)
    {
        printf("Error adding token: Reached max tokens.\n");
        return;
    }

    tokens[token_count].type = token_type;
    strncpy(tokens[token_count].lexeme, lexeme, MAX_VALUE_LENGTH - 1);
    tokens[token_count].lexeme[MAX_VALUE_LENGTH - 1] = '\0';
    token_count++;
}

// Extract datatype or identifier token
void get_token_as_dt_or_id(const char *source, char *target, int *s_iterator, int *t_iterator)
{
    *t_iterator = 0;
    while (source[*s_iterator] != '\0' &&
           (isalnum((unsigned char)source[*s_iterator]) || source[*s_iterator] == '_'))
    {
        if (*t_iterator < MAX_BUFFER_LEN - 1)
            target[(*t_iterator)++] = source[*s_iterator];
        (*s_iterator)++;
    }
    target[*t_iterator] = '\0';
    (*s_iterator)--; // step back since main loop increments
}

// Extract integer literal (with optional unary + or -)
void get_token_as_value(const char *source, char *target, int *s_iterator, int *t_iterator)
{
    *t_iterator = 0;
    // Optional sign (+ or -)
    if (source[*s_iterator] == '+' || source[*s_iterator] == '-')
    {
        if (*t_iterator < MAX_BUFFER_LEN - 1)
            target[(*t_iterator)++] = source[*s_iterator];
        (*s_iterator)++;
    }

    while (isdigit((unsigned char)source[*s_iterator]))
    {
        if (*t_iterator < MAX_BUFFER_LEN - 1)
            target[(*t_iterator)++] = source[*s_iterator];
        (*s_iterator)++;
    }
    target[*t_iterator] = '\0';
    (*s_iterator)--; // backtrack one since for-loop will ++
}

// Reset temporary token buffer
void reset_tokens(int *iterator, char *temp_token)
{
    *iterator = 0;
    if (temp_token)
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
    printf("\n----- TOKENS -----\n");
    if (token_count == 0)
    {
        printf("No tokens found.\n");
    }
    for (int i = 0; i < token_count; i++)
    {
        printf("Token %d: %-12s | %s\n",
               i + 1,
               token_type_to_string(tokens[i].type),
               tokens[i].lexeme);
    }
    printf("------------------\n\n");
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
    if (!src)
    {
        printf("Lexer: source is NULL\n");
        return;
    }

    int len = (int)strlen(src);
    char temp_token[MAX_BUFFER_LEN];
    int t_iter = 0;

    for (int i = 0; i < len; i++)
    {
        char c = src[i];

        // Ignore carriage returns (Windows CRLF)
        if (c == '\r')
            continue;

        // Skip whitespace
        if (isspace((unsigned char)c))
            continue;

        // Skip comments
        if (c == '/' && i + 1 < len && src[i + 1] == '/')
        {
            skip_single_line_comment(src, &i);
            continue;
        }
        else if (c == '/' && i + 1 < len && src[i + 1] == '*')
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

        // Integer literal
        if (isdigit((unsigned char)c) ||
            ((c == '+' || c == '-') && i + 1 < len && isdigit((unsigned char)src[i + 1])))
        {
            get_token_as_value(src, temp_token, &i, &t_iter);
            add_to_tokens(temp_token, TOK_INT_LITERAL);
            reset_tokens(&t_iter, temp_token);
            continue;
        }

        // Character literal
        if (c == '\'')
        {
            int j = i + 1;
            char ch = '\0';
            if (j < len)
            {
                if (src[j] == '\\' && j + 1 < len) // escape
                {
                    ch = src[j + 1];
                    j += 2;
                }
                else
                {
                    ch = src[j];
                    j += 1;
                }
                if (j < len && src[j] == '\'')
                {
                    char tmp[2] = {ch, '\0'};
                    add_to_tokens(tmp, TOK_CHAR_LITERAL);
                    i = j; // advance past closing quote
                    continue;
                }
            }
            // if malformed, fall through to warning
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
        if (is_delimiter(c))
        {
            temp_token[0] = c;
            temp_token[1] = '\0';
            add_to_tokens(temp_token, TOK_DELIMITER);
            continue;
        }

        // Operators (single or compound)
        if (is_operator_char(c))
        {
            char next = (i + 1 < len) ? src[i + 1] : '\0';
            // compound: ++ -- += -= *= /= == != <= >=
            if ((c == '+' && next == '+') || (c == '-' && next == '-') ||
                (next == '=' && (c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '!' || c == '<' || c == '>')) ||
                (c == '=' && next == '=') || (c == '!' && next == '=') ||
                (c == '<' && next == '=') || (c == '>' && next == '='))
            {
                temp_token[0] = c;
                temp_token[1] = next;
                temp_token[2] = '\0';
                add_to_tokens(temp_token, TOK_OPERATOR);
                i++; // consume compound char
            }
            else
            {
                temp_token[0] = c;
                temp_token[1] = '\0';
                add_to_tokens(temp_token, TOK_OPERATOR);
            }
            continue;
        }

        // If we get here, unknown char
        printf("Lexer Warning: Unknown symbol (ASCII %d) '%c'\n", (unsigned char)c, c);
    }

    display_tokens();
}