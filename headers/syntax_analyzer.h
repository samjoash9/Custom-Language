#ifndef SYNTAX_ANALYZER_H
#define SYNTAX_ANALYZER_H

#include "lexical_analyzer.h"

#define MAX_VALUE_LEN 100

typedef enum
{
    NODE_PROGRAM,
    NODE_STATEMENT_LIST,
    NODE_STATEMENT,
    NODE_DECLARATION,
    NODE_ASSIGNMENT,
    NODE_EXPRESSION,
    NODE_TERM,
    NODE_FACTOR,
    NODE_UNARY_OP,
    NODE_POSTFIX_OP,
} NodeType;

typedef struct ASTNode
{
    NodeType type;
    char value[MAX_VALUE_LEN];
    struct ASTNode *left;
    struct ASTNode *right;
} ASTNode;

extern ASTNode *syntax_tree;

void syntax_analyzer();
ASTNode *create_node(NodeType type, const char *value, ASTNode *left, ASTNode *right);
void print_ast(ASTNode *node, int depth);
void free_ast(ASTNode *node);
void generate_intermediate_code(ASTNode *root);

ASTNode *parse_program();
ASTNode *parse_statement_list();
ASTNode *parse_statement();
ASTNode *parse_declaration();
ASTNode *parse_assignment();
ASTNode *parse_assignment_element();
ASTNode *parse_expression();
ASTNode *parse_term();
ASTNode *parse_factor();

#endif
