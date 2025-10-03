#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int is_char_soperator(char c)
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

int concatenate_char_to_string(char *destination, char source)
{
    if (source == '\0')
        return 0;

    int len = strlen(destination);

    destination[len] = source;
    destination[len + 1] = '\0';

    return 1;
}

int is_string_number(char *source)
{
    if (source == NULL || source[0] == '\0')
    {
        printf("Not a number.");
        return 0;
    }

    int i = 0;

    if (source[0] == '-')
        i = 1;

    for (; i < strlen(source); i++)
        if (!isdigit(source[i]))
            return 0;

    return 1;
}

int string_to_int(char *source)
{
    if (source == NULL || source == '\0' || is_string_number(source) != 1)
    {
        printf("Error: source is empty or source is not pure integer.");
        return 0;
    }

    int res = 0, len = strlen(source), i = 0, sign = 1;

    // handle negative values
    if (source[0] == '-')
    {
        sign = -1;
        i++;
    }

    for (; i < len; i++)
        res = res * 10 + (source[i] - 48);

    return res * sign;
}