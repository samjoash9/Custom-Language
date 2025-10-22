#include <stdio.h>
#include <string.h>
#include "headers/target_code_generator.h"
#include "headers/symbol_table.h"

extern TACInstruction *optimizedCode;
extern int optimizedCount;

// Temporary registers for t0-t9
#define TEMP_COUNT 10
static char *tempRegs[TEMP_COUNT] = {"r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10"};
static int tempCounter = 0;

// Find variable index in symbol table
static int findVarIndex(const char *name)
{
    if (!name)
        return -1;
    for (int i = 0; i < symbol_count; i++)
        if (strcmp(symbol_table[i].name, name) == 0)
            return i;
    return -1;
}

static void generateDataSection()
{
    printf(".data\n");
    for (int i = 0; i < symbol_count; i++)
    {
        printf("%s: .dword 0\n", symbol_table[i].name);
    }
    printf("\n");
}

// Generate .code section
static void generateCodeSection()
{
    printf(".code\n");
    tempCounter = 0;

    for (int i = 0; i < optimizedCount; i++)
    {
        TACInstruction *inst = &optimizedCode[i];
        int idxRes = findVarIndex(inst->result);
        int idxArg1 = findVarIndex(inst->arg1);
        int idxArg2 = findVarIndex(inst->arg2);

        // Assignment
        if (strcmp(inst->op, "=") == 0)
        {
            if (idxArg1 != -1) // arg1 is variable
            {
                // Load variable into r1 and store
                printf("lw r1, %s(r0)\n", inst->arg1);
                if (idxRes != -1)
                    printf("sw r1, %s(r0)\n", inst->result);
            }
            else if (inst->arg1 && strlen(inst->arg1) > 0) // constant
            {
                // Load immediate directly into r1 and store
                if (idxRes != -1)
                {
                    printf("daddiu r1, r0, %s\n", inst->arg1);
                    printf("sw r1, %s(r0)\n", inst->result);
                }
            }
            continue;
        }

        // Binary operations (+, -, *, /)
        char *rd = tempRegs[tempCounter++ % TEMP_COUNT];
        char *rs = tempRegs[tempCounter++ % TEMP_COUNT];
        char *rt = tempRegs[tempCounter++ % TEMP_COUNT];

        // Load arg1
        if (idxArg1 != -1)
            printf("lw %s, %s(r0)\n", rs, inst->arg1);
        else if (inst->arg1 && strlen(inst->arg1) > 0)
            printf("daddiu %s, r0, %s\n", rs, inst->arg1);

        // Load arg2
        if (idxArg2 != -1)
            printf("lw %s, %s(r0)\n", rt, inst->arg2);
        else if (inst->arg2 && strlen(inst->arg2) > 0)
            printf("daddiu %s, r0, %s\n", rt, inst->arg2);

        // Perform operation
        if (strcmp(inst->op, "+") == 0)
            printf("DADD %s, %s, %s\n", rd, rs, rt);
        else if (strcmp(inst->op, "-") == 0)
            printf("DSUB %s, %s, %s\n", rd, rs, rt);
        else if (strcmp(inst->op, "*") == 0)
        {
            printf("DMULT %s, %s\n", rs, rt);
            printf("MFLO %s\n", rd);
        }
        else if (strcmp(inst->op, "/") == 0)
        {
            printf("DDIV %s, %s\n", rs, rt);
            printf("MFLO %s\n", rd);
        }

        if (idxRes != -1)
            printf("sw %s, %s(r0)\n", rd, inst->result);
    }
}

// Public function
void generate_target_code()
{
    tempCounter = 0;
    generateDataSection();
    generateCodeSection();
}
