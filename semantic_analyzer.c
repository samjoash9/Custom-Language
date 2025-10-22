#include "headers/semantic_analyzer.h"
#include "headers/symbol_table.h"

int has_semantic_error = 0;

int semantic_analyzer()
{
    printf("\n====== SEMANTIC ANALYZER ======\n");
    printf("Semantic Analyzer is running.\n\n");

    if (symbol_count <= 0)
    {
        printf("Symbol table is empty.\n");
        return 0;
    }

    // Loop through the symbol table
    for (int i = 0; i < symbol_count; i++)
    {
        SYMBOL_TABLE sym = symbol_table[i];

        // --- [1] Undeclared identifier ---
        if (strcmp(sym.datatype, "") == 0)
        {
            printf("Semantic Error: Undeclared identifier '%s'.\n", sym.name);
            has_semantic_error = 1;
            continue;
        }

        // --- [2] Declared but not initialized ---
        if (sym.initialized == 0)
        {
            printf("Warning: Variable '%s' declared but not initialized.\n", sym.name);
        }

        // --- [3] Expression (RHS) analysis ---
        if (strlen(sym.value_str) > 0)
        {
            // Check for variables used inside value_str
            for (int j = 0; j < symbol_count; j++)
            {
                // skip self
                if (i == j)
                    continue;

                // If RHS (value_str) contains another variable name
                if (strstr(sym.value_str, symbol_table[j].name))
                {
                    SYMBOL_TABLE used = symbol_table[j];

                    // Undeclared variable in expression
                    if (strcmp(used.datatype, "") == 0)
                    {
                        printf("Semantic Error: Variable '%s' uses undeclared variable '%s' in its expression.\n",
                               sym.name, used.name);
                        has_semantic_error = 1;
                    }
                    // Uninitialized variable in expression
                    else if (used.initialized == 0)
                    {
                        printf("Semantic Error: Variable '%s' uses uninitialized variable '%s' in its expression.\n",
                               sym.name, used.name);
                        has_semantic_error = 1;
                    }
                }
            }
        }

        // --- [4] Type consistency check ---
        // (optional but good practice)
        if (strlen(sym.value_str) > 0 && strcmp(sym.datatype, "int") == 0)
        {
            // Check if RHS looks like a char literal, e.g. 'a'
            if (sym.value_str[0] == '\'' && sym.value_str[strlen(sym.value_str) - 1] == '\'')
            {
                printf("Type Warning: Assigning char value %s to int variable '%s'.\n",
                       sym.value_str, sym.name);
                has_semantic_error = 1;
            }
        }
        else if (strlen(sym.value_str) > 0 && strcmp(sym.datatype, "char") == 0)
        {
            // Check if RHS looks like a number
            if (isdigit(sym.value_str[0]))
            {
                printf("Type Warning: Assigning numeric value %s to char variable '%s'.\n",
                       sym.value_str, sym.name);
                has_semantic_error = 1;
            }
        }
    }

    if (!has_semantic_error)
        printf("\nNo semantic errors found.\n");

    printf("\n====== SEMANTIC ANALYZER END ======\n\n");
    return 1;
}
