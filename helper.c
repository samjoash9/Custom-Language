/*
==========================================================================
 H E L P E R   F U N C T I O N
==========================================================================

*/

#include <stdio.h>
#include <string.h>

#define MAX 100
#define MAX_TYPE 50
#define TOKEN_VALUE_MAX_LEN 20

typedef struct
{
    char value[TOKEN_VALUE_MAX_LEN];
    char type[MAX_TYPE];
} Token;

// T Y P E S

int read_from_file(char *buffer, FILE *fp, char *filename)
{
    fp = fopen(filename, "rb"); // read in binary mode

    if (fp == NULL)
        return 0;

    fgets(buffer, MAX, fp);

    return 1;
}

int tokenize(char *buffer, char *source)
{
}