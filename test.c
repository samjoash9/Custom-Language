#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

void get_char_value(char *source_code, char *target, int *s_iterator, int *t_iterator)
{
    if (source_code[*s_iterator] == '\'')
    {
        target[(*t_iterator)++] = source_code[(*s_iterator)++];
    }

    // check what's inside
    if (source_code[*s_iterator] == '\n')
    {
        printf("Hehe");
    }

    
}

int main()
{
    char source_code[10] = "'\n'";
    int s_iterator = 0, t_iterator = 0;
    char buffer[10];

    get_char_value(source_code, buffer, &s_iterator, &t_iterator);

    return 0;
}