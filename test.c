#include <stdio.h>
#include <string.h>

int main()
{
    char token[100];

    strcpy(token, "hello");
    printf("Before reset: '%s'\n", token);

    token[0] = '\0'; // reset
    printf("After reset: '%s'\n", token);

    return 0;
}
