#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "headers/syntax_analyzer.h"
#include "headers/symbol_table.h"
#include "headers/intermediate_code_generator.h"

typedef struct {
    char result[50];
    char arg1[50];
    char op[10];
    char arg2[50];
} TACInstruction;

static TACInstruction *code = NULL;
static TACInstruction *optimizedCode = NULL;
static int codeCount = 0;
static int optimizedCount = 0;
static int tempCount = 0;

// Allocate a new temporary variable name
static char *newTemp()
{
    char *name = (char *)malloc(10 * sizeof(char));
    sprintf(name, "t%d", tempCount++);
    return name;
}

// Emit a TAC instruction
static void emit(const char *result, const char *arg1, const char *op, const char *arg2)
{
    code = realloc(code, sizeof(TACInstruction) * (codeCount + 1));
    strcpy(code[codeCount].result, result ? result : "");
    strcpy(code[codeCount].arg1, arg1 ? arg1 : "");
    strcpy(code[codeCount].op, op ? op : "");
    strcpy(code[codeCount].arg2, arg2 ? arg2 : "");
    codeCount++;
}

// Generate code for an expression node
static char *generateExpression(ASTNode *node)
{
    if (!node) return NULL;

    // Factor: literal or identifier
    if (node->type == NODE_FACTOR && node->right == NULL)
        return strdup(node->value);

    // Unary operator (e.g., -a)
    if (node->type == NODE_FACTOR && node->right != NULL)
    {
        char *right = generateExpression(node->right);
        char *temp = newTemp();
        emit(temp, node->value, "", right);
        free(right);
        return temp;
    }

    // Comma expression
    if (node->type == NODE_EXPRESSION && strcmp(node->value, ",") == 0)
    {
        generateExpression(node->left);
        char *res = generateExpression(node->right);
        return res;
    }

    // Increment/Decrement (++ / --)
    if (node->type == NODE_EXPRESSION &&
        (strcmp(node->value, "++") == 0 || strcmp(node->value, "--") == 0))
    {
        const char *id = node->left->value;
        char *temp = newTemp();
        const char *op = strcmp(node->value, "++") == 0 ? "+" : "-";
        emit(temp, id, op, "1");
        emit((char *)id, temp, "=", NULL);
        return strdup(id);
    }

    // Binary operator (EXPRESSION or TERM)
    if ((node->type == NODE_EXPRESSION || node->type == NODE_TERM) &&
        node->left && node->right)
    {
        char *left = generateExpression(node->left);
        char *right = generateExpression(node->right);
        char *temp = newTemp();
        emit(temp, left, node->value, right);
        free(left);
        free(right);
        return temp;
    }

    // Assignment operators
    if (node->type == NODE_ASSIGNMENT)
    {
        if (strcmp(node->value, "+=") == 0)
        {
            char *rhs = generateExpression(node->right);
            emit(node->left->value, node->left->value, "+", rhs);
            free(rhs);
            return strdup(node->left->value);
        }
        else if (strcmp(node->value, "-=") == 0)
        {
            char *rhs = generateExpression(node->right);
            emit(node->left->value, node->left->value, "-", rhs);
            free(rhs);
            return strdup(node->left->value);
        }
        else if (strcmp(node->value, "*=") == 0)
        {
            char *rhs = generateExpression(node->right);
            emit(node->left->value, node->left->value, "*", rhs);
            free(rhs);
            return strdup(node->left->value);
        }
        else if (strcmp(node->value, "/=") == 0)
        {
            char *rhs = generateExpression(node->right);
            emit(node->left->value, node->left->value, "/", rhs);
            free(rhs);
            return strdup(node->left->value);
        }
        else if (strcmp(node->value, "=") == 0)
        {
            char *rhs = generateExpression(node->right);
            emit(node->left->value, rhs, "=", NULL);
            free(rhs);
            return strdup(node->left->value);
        }
    }

    return NULL;
}



// Generate code for statements
static void generateCode(ASTNode *node)
{
    if (!node) return;

    switch (node->type)
    {
        case NODE_PROGRAM:
            generateCode(node->left);
            break;

        case NODE_STATEMENT_LIST:
            generateCode(node->left);
            generateCode(node->right);
            break;

        case NODE_STATEMENT:
            generateCode(node->left);
            break;

        case NODE_DECLARATION:
        {
            ASTNode *cur = node->left;
            while (cur)
            {
            //printf("%s %s\n", node->value, cur->value);
                if (cur->left)
                {
                    char *rhs = generateExpression(cur->left);
                    emit(cur->value, rhs, "=", NULL);
                    free(rhs);
                }
                cur = cur->right;
            }
            break;
        }

        case NODE_ASSIGNMENT:
        case NODE_EXPRESSION:
        {
            char *res = generateExpression(node);
            free(res);
            break;
        }

        default:
            break;
    }
}

// Optimization: remove redundant temporaries
static void removeRedundantTemporaries()
{
    optimizedCode = malloc(sizeof(TACInstruction) * codeCount);
    memcpy(optimizedCode, code, sizeof(TACInstruction) * codeCount);
    optimizedCount = codeCount;

    for (int i = 0; i < optimizedCount - 1; i++) {
        TACInstruction *cur = &optimizedCode[i];
        TACInstruction *next = &optimizedCode[i + 1];

        // Match pattern: tX = A op B  and next is  D = tX
        if (strncmp(cur->result, "t", 1) == 0 &&        // result is temp
            strcmp(next->arg1, cur->result) == 0 &&     // next uses it
            strcmp(next->op, "=") == 0 && strlen(next->arg2) == 0)
        {
            // Rewrite: D = A op B
            strcpy(next->arg1, cur->arg1);
            strcpy(next->op, cur->op);
            strcpy(next->arg2, cur->arg2);

            // Mark current as removed
            strcpy(cur->result, "");
        }
    }

    // Compact the code array (remove blanks)
    int j = 0;
    for (int i = 0; i < optimizedCount; i++) {
        if (strlen(optimizedCode[i].result) > 0)
            optimizedCode[j++] = optimizedCode[i];
    }
    optimizedCount = j;
}

// Display TAC
static void displayTAC()
{
    printf("===== INTERMEDIATE CODE (TAC) =====\n");
    for (int i = 0; i < codeCount; i++) {
        TACInstruction *inst = &code[i];
        if (strcmp(inst->op, "=") == 0 && strlen(inst->arg2) == 0)
            printf("%s = %s\n", inst->result, inst->arg1);
        else
            printf("%s = %s %s %s\n", inst->result, inst->arg1, inst->op, inst->arg2);
    }
    printf("===== INTERMEDIATE CODE (TAC) END =====\n\n");
}

// Display optimized TAC
static void displayOptimizedTAC()
{
    printf("===== OPTIMIZED CODE =====\n");
    for (int i = 0; i < optimizedCount; i++) {
        TACInstruction *inst = &optimizedCode[i];
        if (strcmp(inst->op, "=") == 0 && strlen(inst->arg2) == 0)
            printf("%s = %s\n", inst->result, inst->arg1);
        else
            printf("%s = %s %s %s\n", inst->result, inst->arg1, inst->op, inst->arg2);
    }
    printf("===== OPTIMIZED CODE END =====\n");
}

// Public interface
void generate_intermediate_code(ASTNode *root)
{
    code = NULL;
    codeCount = 0;
    tempCount = 0;

    if (root)
        generateCode(root);

    // Show original TAC
    displayTAC();

    // Run optimization
    removeRedundantTemporaries();

    // Show optimized version
    displayOptimizedTAC();

    free(code);
    free(optimizedCode);
}
