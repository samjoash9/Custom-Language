#ifndef INTERMEDIATE_CODE_GENERATOR_H
#define INTERMEDIATE_CODE_GENERATOR_H

// Forward declare ASTNode to avoid circular includes
typedef struct ASTNode ASTNode;

void generate_intermediate_code(ASTNode *root);

#endif // INTERMEDIATE_CODE_GENERATOR_H
