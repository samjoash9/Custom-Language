#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "helper.c"

#define MAX_LEXEME 50
#define MAX_BUFFER 256

enum LITERAL_TYPE
{
    L_INT,
    L_CHAR,
    L_STRING,
};

enum KEYWORD_TYPE
{
    K_INT,
    K_CHAR,
    K_IF,
    K_ELSE,
};

enum OPERATOR_TYPE
{
    // arithmetic operators
    ADD,      // +
    SUBTRACT, // -
    MULTIPLY, // *
    DIVIDE,   // /

    // assignment
    ASSIGN, // =

    // COMPARISON
    EQUAL, // ==
    AND,   // &&
    OR,    // ||
    NOT,   // !
};

enum DELIMITER_TYPE
{
    D_SEMICOLON,
};

enum TOKEN_TYPE
{
    KEYWORD,
    IDENTIFIER,
    LITERAL,
    OPERATOR,
    DELIMITER,
    END_OF_FILE,
    ERROR
};

typedef struct
{
    enum TOKEN_TYPE type;
    union
    {
        enum LITERAL_TYPE literal;
        enum KEYWORD_TYPE keyword;
        enum OPERATOR_TYPE operator;
        enum DELIMITER_TYPE delimiter;
    } subtype;

    char lexeme[MAX_LEXEME];
    int row;
    int column;

    union
    {
        int int_value;
        char char_value;
    } value;
} LEX;

LEX store_to_lex(char *token, int row, int column)
{
    LEX res;
    strcpy(res.lexeme, token);
    res.row = row;
    res.column = column;

    // Keywords
    if (strcmp(token, "int") == 0)
    {
        res.type = KEYWORD;
        res.subtype.keyword = K_INT;
        return res;
    }
    if (strcmp(token, "char") == 0)
    {
        res.type = KEYWORD;
        res.subtype.keyword = K_CHAR;
        return res;
    }
    if (strcmp(token, "if") == 0)
    {
        res.type = KEYWORD;
        res.subtype.keyword = K_IF;
        return res;
    }
    if (strcmp(token, "else") == 0)
    {
        res.type = KEYWORD;
        res.subtype.keyword = K_ELSE;
        return res;
    }

    // Literal
    if (is_string_number(token))
    {
        res.type = LITERAL;
        res.subtype.literal = L_INT;
        res.value.int_value = string_to_int(token);
        return res;
    }

    // Delimiter
    if (strcmp(token, ";") == 0)
    {
        res.type = DELIMITER;
        res.subtype.delimiter = D_SEMICOLON;
        return res;
    }

    // Operator
    if (strcmp(token, "+") == 0)
    {
        res.type = OPERATOR;
        res.subtype.operator = ADD;
        return res;
    }
    if (strcmp(token, "-") == 0)
    {
        res.type = OPERATOR;
        res.subtype.operator = SUBTRACT;
        return res;
    }
    if (strcmp(token, "*") == 0)
    {
        res.type = OPERATOR;
        res.subtype.operator = MULTIPLY;
        return res;
    }
    if (strcmp(token, "/") == 0)
    {
        res.type = OPERATOR;
        res.subtype.operator = DIVIDE;
        return res;
    }
    if (strcmp(token, "=") == 0)
    {
        res.type = OPERATOR;
        res.subtype.operator = ASSIGN;
        return res;
    }
    if (strcmp(token, "==") == 0)
    {
        res.type = OPERATOR;
        res.subtype.operator = EQUAL;
        return res;
    }
    if (strcmp(token, "&&") == 0)
    {
        res.type = OPERATOR;
        res.subtype.operator = AND;
        return res;
    }
    if (strcmp(token, "||") == 0)
    {
        res.type = OPERATOR;
        res.subtype.operator = OR;
        return res;
    }
    if (strcmp(token, "!") == 0)
    {
        res.type = OPERATOR;
        res.subtype.operator = NOT;
        return res;
    }

    // Otherwise, identifier
    res.type = IDENTIFIER;
    return res;
}

// for debugging
void print_lexer_details(LEX *lexer, int token_count)
{
    if (!lexer)
    {
        printf("Lexer array is NULL.\n");
        return;
    }

    for (int i = 0; i < token_count; i++)
    {
        LEX token = lexer[i];
        printf("[Token] '%s'  Type: ", token.lexeme);

        switch (token.type)
        {
        case KEYWORD:
            printf("KEYWORD (");
            switch (token.subtype.keyword)
            {
            case K_INT:
                printf("int");
                break;
            case K_CHAR:
                printf("char");
                break;
            case K_IF:
                printf("if");
                break;
            case K_ELSE:
                printf("else");
                break;
            }
            printf(")");
            break;

        case IDENTIFIER:
            printf("IDENTIFIER");
            break;

        case LITERAL:
            printf("LITERAL (INT: %d)", token.value.int_value);
            break;

        case OPERATOR:
            printf("OPERATOR (");
            switch (token.subtype.operator)
            {
            case ADD:
                printf("+");
                break;
            case SUBTRACT:
                printf("-");
                break;
            case MULTIPLY:
                printf("*");
                break;
            case DIVIDE:
                printf("/");
                break;
            case ASSIGN:
                printf("=");
                break;
            case EQUAL:
                printf("==");
                break;
            case AND:
                printf("&&");
                break;
            case OR:
                printf("||");
                break;
            case NOT:
                printf("!");
                break;
            }
            printf(")");
            break;

        case DELIMITER:
            printf("DELIMITER (;)");
            break;

        default:
            printf("UNKNOWN");
        }

        printf("  Row: %d  Col: %d\n", token.row, token.column);
    }
}

int main()
{
    // READ FROM A FILE
    FILE *fp = fopen("main.txt", "r");
    if (fp == NULL)
    {
        printf("Error: file not found\n");
        return 0;
    }

    int current_lex = 0, max_rows = 0, row = 1, colum = 1;
    int lexer_capacity = 10; // initial capacity
    char c, current_token[MAX_BUFFER] = "\0";

    // allocate first LEX array
    LEX *lexer = (LEX *)malloc(sizeof(LEX) * lexer_capacity);
    if (lexer == NULL)
    {
        printf("Failed to allocate lexer\n");
        return 0;
    }

    while ((c = fgetc(fp)) != EOF)
    {
        if (isalnum(c) || is_char_soperator(c))
        {
            // concatenate to token
            if (!concatenate_char_to_string(current_token, c))
            {
                printf("Error: could not concatenate token.\n");
                free(lexer);
                return 0;
            };
        }
        else if (isspace(c))
        {
            if (strlen(current_token) == 0)
                continue;

            // store to LEX
            // printf("%s\n", current_token);

            // check if we need to grow the array
            if (current_lex >= lexer_capacity)
            {
                lexer_capacity *= 2; // double the size
                LEX *tmp = realloc(lexer, sizeof(LEX) * lexer_capacity);
                if (!tmp)
                {
                    printf("Error: failed to reallocate lexer\n");
                    free(lexer);
                    return 0;
                }
                lexer = tmp;
            }

            lexer[current_lex++] = store_to_lex(current_token, row, colum);

            colum++;
            current_token[0] = '\0';
        }
        else if (c == ';')
        {
            if (current_token[0] != '\0')
            {
                // store to lex current token
                // printf("%s\n", current_token);

                if (current_lex >= lexer_capacity)
                {
                    lexer_capacity *= 2;
                    LEX *tmp = realloc(lexer, sizeof(LEX) * lexer_capacity);
                    if (!tmp)
                    {
                        printf("Error: failed to reallocate lexer\n");
                        free(lexer);
                        return 0;
                    }
                    lexer = tmp;
                }

                lexer[current_lex++] = store_to_lex(current_token, row, colum);
                current_token[0] = '\0';
            }

            colum++;

            // store semicolon as its own token
            // printf(";\n");

            if (current_lex >= lexer_capacity)
            {
                lexer_capacity *= 2;
                LEX *tmp = realloc(lexer, sizeof(LEX) * lexer_capacity);
                if (!tmp)
                {
                    printf("Error: failed to reallocate lexer\n");
                    free(lexer);
                    return 0;
                }
                lexer = tmp;
            }

            lexer[current_lex++] = store_to_lex(";", row, colum);
        }
        else if (c == '#')
        {
            // SKIP
        }
        else if (c == '\n')
        {
            row++;
            colum = 1; // reset column on new line
        }
        else
        {
            // Error
            printf("Error: unknown character '%c' at row %d, col %d\n", c, row, colum);
            free(lexer);
            fclose(fp);
            return 0;
        }
    }

    // Flush last token if exists
    if (strlen(current_token) > 0)
    {
        printf("%s\n", current_token);
        if (current_lex >= lexer_capacity)
        {
            lexer_capacity *= 2;
            LEX *tmp = realloc(lexer, sizeof(LEX) * lexer_capacity);
            if (!tmp)
            {
                printf("Error: failed to reallocate lexer\n");
                free(lexer);
                return 0;
            }
            lexer = tmp;
        }
        lexer[current_lex++] = store_to_lex(current_token, row, colum);
    }

    printf("Total tokens: %d\n", current_lex);

    print_lexer_details(lexer, current_lex);

    free(lexer);
    fclose(fp);
    return 0;
}