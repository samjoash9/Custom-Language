#include <stdio.h>
#include <string.h>
#include <ctype.h>

int is_string_number(char *source)
{
    // check if source is valid
    if (source == NULL || source == '\0' || strlen(source) == 0)
    {
        printf("Error function call: is string_number(). Source is not valid.\n");
        return -1;
    }

    int len = strlen(source);

    for (int i = 0; i < len; i++)
    {
        if (!isdigit(source[i]))
            return -1;
    }

    return 1;
}

int main()
{
    printf("%d", is_string_number("0"));
}