#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helper.c"

int main()
{
    FILE *fp;
    char buffer[MAX], file_name[TOKEN_VALUE_MAX_LEN] = "temp.hehe";
    int res = read_from_file(buffer, fp, file_name);

    if (!res)
        printf("Error, file not found.");

    printf("%s", buffer);

    return 0;
}