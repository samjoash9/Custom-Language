#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    int value;
} my_number;

int main()
{
    int lex_length = 1, max_lex_length = 5;

    my_number *numbers;
    numbers = (my_number *)malloc(lex_length * sizeof(my_number));

    if (numbers == NULL)
    {
        printf("Failed to reallocate.");
        return 0;
    }

    for (int i = 0; i < max_lex_length; i++)
    {
        lex_length++;
        // memory allocate
        numbers = realloc(numbers, lex_length * sizeof(my_number));
        if (numbers == NULL)
        {
            printf("Failed to memory allocate.");
            return 0;
        }
    }

    free(numbers);

    return 0;
}
