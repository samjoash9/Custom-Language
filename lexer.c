#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEXEME 50
#define MAX_BUFFER 256

enum LITERAL_TYPE
{
    L_INT,
    L_CHAR,
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
    D_NEWLINE,
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
    int line;
    int column;

    union
    {
        int int_value;
        char char_value;
    } value;
} LEX;

int concatenate_char_to_string(char *destination, char source)
{
    if (source == '\0')
        return 0;

    int len = strlen(destination);

    destination[len] = source;
    destination[len + 1] = '\0';

    return 1;
}

int issoperator(char c)
{
    switch (c)
    {
    case '+':
    case '-':
    case '*':
    case '/':
    case '=':
    case '&':
    case '|':
    case '!':
        return 1;
    default:
        return 0;
    }
}

int main()
{
    // READ FROM A FILE
    FILE *fp = fopen("main.txt", "r");

    if (fp == NULL)
    {
        printf("Error: file not found");
        return 0;
    }

    int max_lex_length = 0, max_rows = 0, rows = 0, colum = 0;
    char c, current_token[MAX_BUFFER] = "\0";

    // allocate first LEX
    LEX *lexer = (LEX *)malloc(sizeof(LEX));

    if (lexer == NULL)
    {
        printf("Failed to allocate lexer");
        return 0;
    }

    // create manual DFA in switch
    while ((c = fgetc(fp)) != EOF)
    {
        if (isalnum(c) || issoperator(c) || isdigit(c))
        {
            // concatenate to token
            if (!concatenate_char_to_string(current_token, c))
            {
                printf("Error: could not concatenate token.");
                return 0;
            };
        }
        else if (isspace(c))
        {
            // store to LEX
            printf("%s\n", current_token);

            // reset token
            current_token[0] = '\0';
        }
        else if (c == ';')
        {
            // store to LEX
            printf("%s\n", current_token);

            // reset token and go to next line
            current_token[0] = '\0';
        }
        else if (c == '#')
        {
            // SKIP
        }
    }

    free(lexer);
    fclose(fp);
    return 0;
}