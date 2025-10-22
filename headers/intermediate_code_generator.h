#ifndef INTERMEDIATE_CODE_GENERATOR_H
#define INTERMEDIATE_CODE_GENERATOR_H

#include "syntax_analyzer.h"
#include "symbol_table.h"

typedef struct
{
    char result[64];
    char arg1[64];
    char op[16];
    char arg2[64];
} TACInstruction;

extern TACInstruction *optimizedCode;
extern int optimizedCount;

// Forward declare ASTNode to avoid circular includes
typedef struct ASTNode ASTNode;

void generate_intermediate_code(ASTNode *root);
TACInstruction *getOptimizedCode(int *count);

#endif // INTERMEDIATE_CODE_GENERATOR_H
