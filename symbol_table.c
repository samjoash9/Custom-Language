#include "headers/symbol_table.h"

SYMBOL_TABLE symbol_table[MAX_SYMBOLS];
int symbol_count = 0;

// Add a new symbol to the symbol table
int add_symbol(const char *name, const char *datatype, const char *value_str, int initialized)
{
    // Check for redeclaration
    for (int i = 0; i < symbol_count; i++)
    {
        if (strcmp(symbol_table[i].name, name) == 0)
        {
            printf("Semantic Error: Redeclaration of variable '%s' (previously declared as '%s')\n",
                   name, symbol_table[i].datatype);
            return 0;
        }
    }

    // Check for overflow
    if (symbol_count >= MAX_SYMBOLS)
    {
        printf("Symbol Table Overflow: Too many symbols.\n");
        return 0;
    }

    // Add new symbol
    strcpy(symbol_table[symbol_count].name, name);
    strcpy(symbol_table[symbol_count].datatype, datatype);
    strcpy(symbol_table[symbol_count].value_str, value_str);
    symbol_table[symbol_count].initialized = initialized;

    symbol_count++;
    return 1;
}

// Find symbol by name (returns index or -1 if not found)
int find_symbol(const char *name)
{
    for (int i = 0; i < symbol_count; i++)
    {
        if (strcmp(symbol_table[i].name, name) == 0)
            return i;
    }
    return -1;
}

// Update value and initialization flag for an existing symbol
int update_symbol_value(const char *id, const char *datatype, const char *value_str)
{
    int index = find_symbol(id);

    if (index == -1)
    {
        printf("Semantic Error: Undeclared variable '%s' used in assignment.\n", id);
        return 0;
    }

    // Check for type mismatch if datatype provided
    if (datatype && strlen(datatype) > 0 &&
        strlen(symbol_table[index].datatype) > 0 &&
        strcmp(symbol_table[index].datatype, datatype) != 0)
    {
        printf("Semantic Warning: Type mismatch assigning to '%s' (%s <- %s)\n",
               id, symbol_table[index].datatype, datatype);
    }

    strcpy(symbol_table[index].value_str, value_str);
    symbol_table[index].initialized = 1;

    return 1;
}

// Display the symbol table
void display_symbol_table()
{
    printf("Symbol count: %d\n", symbol_count);
    printf("Name            Type       Value/Expr                     Init?\n");
    printf("---------------------------------------------------------------\n");

    for (int i = 0; i < symbol_count; i++)
    {
        printf("%-15s %-10s %-30s %s\n",
               symbol_table[i].name,
               symbol_table[i].datatype,
               symbol_table[i].value_str[0] ? symbol_table[i].value_str : "(empty)",
               symbol_table[i].initialized ? "Yes" : "No");
    }
}
