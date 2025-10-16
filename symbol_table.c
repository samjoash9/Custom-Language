#include <stdio.h>
#include <string.h>

#include "headers/symbol_table.h"

SYMBOL_TABLE symbol_table[MAX_SYMBOLS];
int symbol_count = 0;

int add_symbol(const char *name, const char *type, int value, int initialized)
{
    // Check for duplicate symbol
    for (int i = 0; i < symbol_count; i++)
    {
        if (strcmp(symbol_table[i].name, name) == 0)
        {
            printf("Warning: Redeclaration of variable '%s'\n", name);
            return i;
        }
    }

    if (symbol_count >= MAX_SYMBOLS)
    {
        printf("Error: Symbol table overflow.\n");
        return -1;
    }

    strcpy(symbol_table[symbol_count].name, name);
    strcpy(symbol_table[symbol_count].datatype, type);

    if (strcmp(type, "int") == 0)
        symbol_table[symbol_count].value.integer_val = initialized ? value : 0;
    else if (strcmp(type, "char") == 0)
        symbol_table[symbol_count].value.character_val = initialized ? (char)value : '\0';

    symbol_count++;
    return symbol_count - 1;
}

int find_symbol(const char *name)
{
    for (int i = 0; i < symbol_count; i++)
    {
        if (strcmp(symbol_table[i].name, name) == 0)
            return i;
    }
    return -1; // not found
}

void display_symbol_table()
{
    if (symbol_count == 0)
    {
        printf("Symbol table is empty.\n");
        return;
    }

    printf("%-15s %-10s %-10s\n", "Name", "Type", "Value");
    printf("--------------------------------\n");

    for (int i = 0; i < symbol_count; i++)
    {
        if (strcmp(symbol_table[i].datatype, "int") == 0)
            printf("%-15s %-10s %d\n",
                   symbol_table[i].name,
                   symbol_table[i].datatype,
                   symbol_table[i].value.integer_val);
        else if (strcmp(symbol_table[i].datatype, "char") == 0)
            printf("%-15s %-10s '%c'\n",
                   symbol_table[i].name,
                   symbol_table[i].datatype,
                   symbol_table[i].value.character_val);
    }
}
