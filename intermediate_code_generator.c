#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headers/syntax_analyzer.h"
#include "headers/symbol_table.h"
#include "headers/intermediate_code_generator.h"

static int tempCount = 0;

// Allocate a new temporary variable
static char *newTemp()
{
    char *name = (char *)malloc(10 * sizeof(char));
    sprintf(name, "t%d", tempCount++);
    return name;
}

// Generate code for an expression node
static char *generateExpression(ASTNode *node)
{
    if (!node) return NULL;

    // Factor: literal or identifier
    if (node->type == NODE_FACTOR && node->right == NULL)
    {
        char *val = strdup(node->value);
        return val;
    }

    // Unary operator (e.g., -a)
    if (node->type == NODE_FACTOR && node->right != NULL)
    {
        char *right = generateExpression(node->right);
        char *temp = newTemp();
        printf("%s = %s%s\n", temp, node->value, right);
        free(right);
        return temp;
    }

    // Comma expression
    if (node->type == NODE_EXPRESSION && strcmp(node->value, ",") == 0)
    {
        generateExpression(node->left);       // side effect
        char *res = generateExpression(node->right); // value of last
        return res;
    }

    // Binary operator (EXPRESSION or TERM)
    if ((node->type == NODE_EXPRESSION || node->type == NODE_TERM) &&
        node->left && node->right)
    {
        char *left = generateExpression(node->left);
        char *right = generateExpression(node->right);
        char *temp = newTemp();
        printf("%s = %s %s %s\n", temp, left, node->value, right);
        free(left);
        free(right);
        return temp;
    }

    // Assignment operator
    if (node->type == NODE_ASSIGNMENT)
    {
        char *rhs = generateExpression(node->right);
        printf("%s = %s\n", node->left->value, rhs);
        free(rhs);
        return strdup(node->left->value);
    }

    return NULL;
}

static void generateCode(ASTNode *node)
{
    if (!node) return;

    switch (node->type)
    {
        case NODE_PROGRAM:
            generateCode(node->left); // Program -> statement list
            break;

        case NODE_STATEMENT_LIST:
            generateCode(node->left);  // first statement
            generateCode(node->right); // rest of the list
            break;

        case NODE_STATEMENT:
            generateCode(node->left); // could be DECL, ASSIGN, or EXPR
            break;

        case NODE_DECLARATION:
        {
            ASTNode *cur = node->left;
            while (cur)
            {
                printf("%s %s\n", node->value, cur->value);
                if (cur->left) // initializer
                {
                    char *rhs = generateExpression(cur->left);
                    printf("%s = %s\n", cur->value, rhs);
                }
                cur = cur->right;
            }
            break;
        }

        case NODE_ASSIGNMENT:
        case NODE_EXPRESSION:
        {
            char *res = generateExpression(node);
            break;
        }

        default:
            break;
    }
}


// Public interface
void generate_intermediate_code(ASTNode *root)
{
    printf("===== INTERMEDIATE CODE (TAC) =====\n");
    if (root)
        generateCode(root);
    printf("===== INTERMEDIATE CODE (TAC) END =====\n");
}
