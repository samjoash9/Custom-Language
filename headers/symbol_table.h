#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#define MAX_SYMBOLS 1024

#include <ctype.h>

// Struct for one symbol entry
typedef struct
{
    char name[50];
    char datatype[10];
    char value_str[100];
    int initialized;
} SYMBOL_TABLE;

// Global variables (shared with other files)
extern SYMBOL_TABLE symbol_table[MAX_SYMBOLS];
extern int symbol_count;

// Function declarations
int add_symbol(const char *name, const char *type, const char *value_str, int initialized);
int find_symbol(const char *name);
int update_symbol_value(const char *id, const char *datatype, const char *value_str);
void display_symbol_table();

#endif
