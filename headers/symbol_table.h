#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#define MAX_SYMBOLS 1024

// LIBRARIES NEEDED
#include <ctype.h>

// Struct for one symbol entry
typedef struct
{
    char name[50];
    char datatype[10];
    union
    {
        int integer_val;
        char character_val;
    } value;
} SYMBOL_TABLE;

// Global variables (shared with other files)
extern SYMBOL_TABLE symbol_table[MAX_SYMBOLS];
extern int symbol_count;

// Function declarations
int add_symbol(const char *name, const char *type, int value, int initialized);
int find_symbol(const char *name);
void display_symbol_table();

#endif
