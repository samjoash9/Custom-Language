#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    D_HASHTAG,
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

int main()
{
    // READ FROM A FILE
    FILE *fp = fopen("main.txt", "r");

    if (fp == NULL)
    {
        printf("Error: file not found");
        return 0;
    }

    int max_lex_length = 0, max_rows = 0;

    // iterators
    int rows = 0, colum = 0;

    // allocate first LEx
    LEX *lexer = (LEX *)malloc(sizeof(LEX));

    if (lexer == NULL)
    {
        printf("Failed to allocate lexer");
        return 0;
    }

    //

    fclose(fp);
    return 0;
}