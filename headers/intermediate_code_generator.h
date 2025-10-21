#ifndef INTERMEDIATE_CODE_GENERATOR_H
#define INTERMEDIATE_CODE_GENERATOR_H

#include "syntax_analyzer.h"

// Forward declare ASTNode so we can use it as a pointer
typedef struct ASTNode ASTNode;

void generate_intermediate_code(ASTNode *root);

#endif