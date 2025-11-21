// lexical_analyzer.c
// Fixed to match the provided lex.l rules and TokenType enum in lexical_analyzer.h

#include "headers/lexical_analyzer.h" // keep your original include path
#include <stdio.h>
#include <string.h>
#include <ctype.h>

TOKEN tokens[MAX_TOKENS];
int token_count = 0;
int error_found = 0;

static void add_to_tokens(const char *lexeme, TokenType token_type)
{
    if (token_count >= MAX_TOKENS)
    {
        fprintf(stderr, "Error adding token: Reached max tokens.\n");
        error_found = 1;
        return;
    }

    tokens[token_count].type = token_type;
    strncpy(tokens[token_count].lexeme, lexeme, MAX_VALUE_LENGTH - 1);
    tokens[token_count].lexeme[MAX_VALUE_LENGTH - 1] = '\0';
    token_count++;
}

int is_datatype(const char *token)
{
    /* In your lex.l 'KUAN', 'ENTEGER', 'CHAROT', 'PRENT' are keywords (tokens),
       but your header's is_datatype function previously checked "int"/"char".
       Keep this helper in case other code expects it. */
    return (strcmp(token, "int") == 0 || strcmp(token, "char") == 0);
}

int is_operator_char(char c)
{
    return (c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '!' || c == '<' || c == '>');
}

int is_delimiter(char c)
{
    return (c == '!' || c == ',');
}

void reset_tokens(int *iterator, char *temp_token)
{
    *iterator = 0;
    if (temp_token)
        temp_token[0] = '\0';
}

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
    (*s_iterator)--; // step back because main loop will advance
}

void get_token_as_value(const char *source, char *target, int *s_iterator, int *t_iterator)
{
    *t_iterator = 0;
    int len = (int)strlen(source);

    // optional sign
    char sign = '\0';
    if (source[*s_iterator] == '+' || source[*s_iterator] == '-')
    {
        sign = source[*s_iterator];
        (*s_iterator)++;
    }

    // collect digits
    while (*s_iterator < len && isdigit((unsigned char)source[*s_iterator]))
    {
        if (*t_iterator < MAX_BUFFER_LEN - 1)
            target[(*t_iterator)++] = source[*s_iterator];
        (*s_iterator)++;
    }

    target[*t_iterator] = '\0';
    (*s_iterator)--; // backtrack one because main loop will ++

    if (sign == '-')
    {
        // prepend minus
        char tmp[MAX_BUFFER_LEN];
        snprintf(tmp, sizeof(tmp), "-%s", target);
        strncpy(target, tmp, MAX_BUFFER_LEN - 1);
        target[MAX_BUFFER_LEN - 1] = '\0';
    }
    // if sign == '+' do nothing (normalize +2 -> 2)
}

void skip_single_line_comment(const char *source_code, int *i)
{
    // assumes current pos at '/'; skip two chars ("//")
    *i += 2;
    while (source_code[*i] != '\0' && source_code[*i] != '\n')
        (*i)++;
}

int skip_multi_line_comment(const char *src, int *i)
{
    // assumes current pos at '/'; skip "/*"
    int len = (int)strlen(src);
    *i += 2;
    while (*i < len && !(src[*i] == '*' && src[*i + 1] == '/'))
        (*i)++;

    if (*i >= len)
    {
        fprintf(stderr, "Error: unclosed multi-line comment\n");
        return 1;
    }

    *i += 1; // main loop will ++ and move past '/'
    return 0;
}

const char *token_type_to_string(TokenType type)
{
    switch (type)
    {
    case KUAN:
        return "KUAN";
    case ENTEGER:
        return "ENTEGER";
    case CHAROT:
        return "CHAROT";
    case PRENT:
        return "PRENT";

    case PLUS_EQUAL:
        return "PLUS_EQUAL";
    case MINUS_EQUAL:
        return "MINUS_EQUAL";
    case DIV_EQUAL:
        return "DIV_EQUAL";
    case MUL_EQUAL:
        return "MUL_EQUAL";

    case PLUSPLUS:
        return "PLUSPLUS";
    case MINUSMINUS:
        return "MINUSMINUS";

    case PLUS:
        return "PLUS";
    case MINUS:
        return "MINUS";
    case MUL:
        return "MUL";
    case DIV:
        return "DIV";
    case EQUAL:
        return "EQUAL";

    case EXCLAM:
        return "EXCLAM";
    case LPAREN:
        return "LPAREN";
    case RPAREN:
        return "RPAREN";
    case COMMA:
        return "COMMA";

    case INT_LITERAL:
        return "INT_LITERAL";
    case CHAR_LITERAL:
        return "CHAR_LITERAL";
    case STRING_LITERAL:
        return "STRING_LITERAL";
    case IDENTIFIER:
        return "IDENTIFIER";

    default:
        return "UNKNOWN";
    }
}

void display_tokens()
{
    printf("----- TOKENS -----\n");
    if (token_count == 0)
    {
        printf("No tokens found.\n");
    }
    for (int i = 0; i < token_count; i++)
    {
        printf("Token %d: %-14s | %s\n",
               i + 1,
               token_type_to_string(tokens[i].type),
               tokens[i].lexeme);
    }
    printf("------------------\n");
}

int lexer(const char *src)
{
    printf("\n===== LEXICAL ANALYSIS START =====\n");

    if (!src)
    {
        fprintf(stderr, "Lexer: source is NULL\n");
        return 1;
    }

    int len = (int)strlen(src);
    char temp_token[MAX_BUFFER_LEN];
    int t_iter = 0;

    for (int i = 0; i < len; i++)
    {
        char c = src[i];

        if (c == '\r')
            continue;

        // whitespace
        if (isspace((unsigned char)c))
        {
            if (c == '\n')
            {
                // optional: print newline token like the lex file did
                // printf("[LEX] NEWLINE\n");
            }
            continue;
        }

        // comments
        if (c == '/' && i + 1 < len && src[i + 1] == '/')
        {
            skip_single_line_comment(src, &i);
            continue;
        }
        if (c == '/' && i + 1 < len && src[i + 1] == '*')
        {
            if (skip_multi_line_comment(src, &i))
            {
                error_found = 1;
                break;
            }
            continue;
        }

        // keywords (exact matches): KUAN, ENTEGER, CHAROT, PRENT
        // Recognize identifiers/keywords
        if (isalpha((unsigned char)c) || c == '_')
        {
            get_token_as_dt_or_id(src, temp_token, &i, &t_iter);

            // check for the specific keywords used in lex.l
            if (strcmp(temp_token, "KUAN") == 0)
                add_to_tokens("KUAN", KUAN);
            else if (strcmp(temp_token, "ENTEGER") == 0)
                add_to_tokens("ENTEGER", ENTEGER);
            else if (strcmp(temp_token, "CHAROT") == 0)
                add_to_tokens("CHAROT", CHAROT);
            else if (strcmp(temp_token, "PRENT") == 0)
                add_to_tokens("PRENT", PRENT);
            else
                add_to_tokens(temp_token, IDENTIFIER);

            reset_tokens(&t_iter, temp_token);
            continue;
        }

        // integer literals (allow unary + or - in contexts handled by this lexer)
        if (isdigit((unsigned char)c) ||
            ((c == '+' || c == '-') &&
             i + 1 < len && isdigit((unsigned char)src[i + 1]) &&
             (i == 0 || src[i - 1] == '(' || src[i - 1] == '=' || isspace((unsigned char)src[i - 1]))))
        {
            get_token_as_value(src, temp_token, &i, &t_iter);
            add_to_tokens(temp_token, INT_LITERAL);
            reset_tokens(&t_iter, temp_token);
            continue;
        }

        // char literal: 'a' or '\n' style
        if (c == '\'')
        {
            int j = i + 1;
            char lit[MAX_VALUE_LENGTH];
            int idx = 0;
            lit[idx++] = '\'';

            if (j < len)
            {
                if (src[j] == '\\' && j + 1 < len)
                {
                    // escaped
                    lit[idx++] = '\\';
                    lit[idx++] = src[j + 1];
                    j += 2;
                }
                else
                {
                    lit[idx++] = src[j];
                    j += 1;
                }

                if (j < len && src[j] == '\'')
                {
                    lit[idx++] = '\'';
                    lit[idx] = '\0';
                    add_to_tokens(lit, CHAR_LITERAL);
                    i = j; // advance to closing quote
                    continue;
                }
            }

            fprintf(stderr, "Lexer Error: Unterminated or invalid character literal\n");
            error_found = 1;
            continue;
        }

        // string literal: "..." with escapes
        if (c == '"')
        {
            int j = i + 1;
            char lit[MAX_VALUE_LENGTH];
            int idx = 0;
            lit[idx++] = '"';

            while (j < len)
            {
                if (src[j] == '\\' && j + 1 < len)
                {
                    // escaped char
                    if (idx < MAX_VALUE_LENGTH - 2)
                    {
                        lit[idx++] = '\\';
                        lit[idx++] = src[j + 1];
                    }
                    j += 2;
                    continue;
                }
                if (src[j] == '"')
                {
                    lit[idx++] = '"';
                    lit[idx] = '\0';
                    add_to_tokens(lit, STRING_LITERAL);
                    i = j; // move past closing quote
                    break;
                }
                // normal char
                if (idx < MAX_VALUE_LENGTH - 1)
                    lit[idx++] = src[j];
                j++;
            }

            if (j >= len)
            {
                fprintf(stderr, "Lexer Error: Unterminated string literal\n");
                error_found = 1;
            }
            continue;
        }

        // parentheses
        if (c == '(' || c == ')')
        {
            temp_token[0] = c;
            temp_token[1] = '\0';
            if (c == '(')
                add_to_tokens(temp_token, LPAREN);
            else
                add_to_tokens(temp_token, RPAREN);
            continue;
        }

        // comma
        if (c == ',')
        {
            temp_token[0] = c;
            temp_token[1] = '\0';
            add_to_tokens(temp_token, COMMA);
            continue;
        }

        // exclamation '!' delimiter/operator
        if (c == '!')
        {
            temp_token[0] = c;
            temp_token[1] = '\0';
            add_to_tokens(temp_token, EXCLAM);
            continue;
        }

        // operators and compound operators
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '=')
        {
            char next = (i + 1 < len) ? src[i + 1] : '\0';

            // compound operators: ++, --, +=, -=, *=, /=
            if ((c == '+' && next == '+') ||
                (c == '-' && next == '-') ||
                (next == '=' && (c == '+' || c == '-' || c == '*' || c == '/')))
            {
                // build token string
                temp_token[0] = c;
                temp_token[1] = next;
                temp_token[2] = '\0';

                if (c == '+' && next == '+')
                    add_to_tokens(temp_token, PLUSPLUS);
                else if (c == '-' && next == '-')
                    add_to_tokens(temp_token, MINUSMINUS);
                else if (c == '+' && next == '=')
                    add_to_tokens(temp_token, PLUS_EQUAL);
                else if (c == '-' && next == '=')
                    add_to_tokens(temp_token, MINUS_EQUAL);
                else if (c == '*' && next == '=')
                    add_to_tokens(temp_token, MUL_EQUAL);
                else if (c == '/' && next == '=')
                    add_to_tokens(temp_token, DIV_EQUAL);
                else
                    add_to_tokens(temp_token, IDENTIFIER); // fallback

                i++; // consume compound second char
                continue;
            }

            // single operators
            temp_token[0] = c;
            temp_token[1] = '\0';
            if (c == '+')
                add_to_tokens(temp_token, PLUS);
            else if (c == '-')
                add_to_tokens(temp_token, MINUS);
            else if (c == '*')
                add_to_tokens(temp_token, MUL);
            else if (c == '/')
                add_to_tokens(temp_token, DIV);
            else if (c == '=')
                add_to_tokens(temp_token, EQUAL);
            else
                add_to_tokens(temp_token, IDENTIFIER); // fallback
            continue;
        }

        // anything else is invalid char (similar to lex rule '.' -> INVALID CHARACTER)
        {
            char unknown[8] = {0};
            unknown[0] = c;
            unknown[1] = '\0';
            fprintf(stderr, "Lexer Warning: INVALID CHARACTER '%s' (ASCII %d)\n", unknown, (unsigned char)c);
            error_found = 1;
            continue;
        }
    } // end for

    display_tokens();

    printf("===== LEXICAL ANALYSIS END =====\n");

    return error_found ? 1 : 0;
}
