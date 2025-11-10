#ifndef SYNTAX_ANALYZER_H
#define SYNTAX_ANALYZER_H

#define MAX_VALUE_LEN 256
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lexical_analyzer.h"
#include "symbol_table.h"
#include "intermediate_code_generator.h"

// === AST NODE TYPES ===
typedef enum
{
    NODE_START,
    NODE_STATEMENT_LIST,
    NODE_STATEMENT,
    NODE_DECLARATION,
    NODE_ASSIGNMENT,
    NODE_EXPRESSION,
    NODE_TERM,
    NODE_FACTOR,
    NODE_UNARY_OP,
    NODE_POSTFIX_OP
} NodeType;

// === AST NODE STRUCT ===
typedef struct ASTNode
{
    NodeType type;
    char *value; // Pointer to dynamically allocated string
    struct ASTNode *left;
    struct ASTNode *right;
} ASTNode;

extern ASTNode *syntax_tree;

// === CORE FUNCTIONS ===
ASTNode *create_node(NodeType type, const char *value, ASTNode *left, ASTNode *right);
void free_ast(ASTNode *node);
void print_ast(ASTNode *node, int depth);

// === PARSER ENTRY POINT ===
ASTNode *parse_program();
int syntax_analyzer();

// === PARSER SUBFUNCTIONS (Forward Declarations) ===
ASTNode *parse_statement_list();
ASTNode *parse_statement();
ASTNode *parse_declaration();
ASTNode *parse_assignment();
ASTNode *parse_additive();
ASTNode *parse_term();
ASTNode *parse_factor();
ASTNode *parse_expression();

#endif