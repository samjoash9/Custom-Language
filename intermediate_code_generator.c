// intermediate_code_generator.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "headers/intermediate_code_generator.h"

static TACInstruction *code = NULL;
TACInstruction *optimizedCode = NULL;
static int codeCount = 0;
int optimizedCount = 0;
static int tempCount = 0;

// === Utility ===
TACInstruction *getOptimizedCode(int *count)
{
    if (count)
        *count = optimizedCount;
    return optimizedCode;
}

static char *newTemp()
{
    char buf[32];
    snprintf(buf, sizeof(buf), "temp%d", tempCount++);
    return strdup(buf);
}

static void emit(const char *result, const char *arg1, const char *op, const char *arg2)
{
    TACInstruction *tmp = realloc(code, sizeof(TACInstruction) * (codeCount + 1));
    if (!tmp)
    {
        fprintf(stderr, "Memory allocation failed in emit()\n");
        exit(1);
    }
    code = tmp;

    snprintf(code[codeCount].result, sizeof(code[codeCount].result), "%s", result ? result : "");
    snprintf(code[codeCount].arg1, sizeof(code[codeCount].arg1), "%s", arg1 ? arg1 : "");
    snprintf(code[codeCount].op, sizeof(code[codeCount].op), "%s", op ? op : "");
    snprintf(code[codeCount].arg2, sizeof(code[codeCount].arg2), "%s", arg2 ? arg2 : "");
    codeCount++;
}

// === Expression Generator ===
static char *generateExpression(ASTNode *node)
{
    if (!node)
        return NULL;

    // Leaf node (identifier or literal)
    if (node->left == NULL && node->right == NULL)
        return strdup(node->value ? node->value : "");

    // Assignment (simple or compound)
    if (node->type == NODE_ASSIGNMENT && node->left && node->right)
    {
        char *lhs = node->left->value;

        // Simple assignment
        if (strcmp(node->value, "=") == 0)
        {
            char *rhs = generateExpression(node->right);
            emit(lhs, rhs, "=", NULL);
            free(rhs);
            return strdup(lhs);
        }
        // Compound assignment (+=, -=, *=, /=)
        else if (strcmp(node->value, "+=") == 0 ||
                 strcmp(node->value, "-=") == 0 ||
                 strcmp(node->value, "*=") == 0 ||
                 strcmp(node->value, "/=") == 0)
        {
            char op[2] = {node->value[0], '\0'}; // "+=" -> '+'
            char *rhs = generateExpression(node->right);
            emit(lhs, lhs, op, rhs); // x = x op rhs
            free(rhs);
            return strdup(lhs);
        }
    }

    // Postfix operations (++ / --)
    if (node->type == NODE_POSTFIX_OP && node->left)
    {
        char *var = node->left->value;
        if (strcmp(node->value, "++") == 0)
            emit(var, var, "+", "1");
        else if (strcmp(node->value, "--") == 0)
            emit(var, var, "-", "1");
        return strdup(var);
    }

    // Unary operators (++ / -- / + / -)
    if (node->type == NODE_UNARY_OP && node->left)
    {
        char *lhs = generateExpression(node->left);
        if (strcmp(node->value, "++") == 0)
        {
            emit(lhs, lhs, "+", "1");
            return lhs;
        }
        else if (strcmp(node->value, "--") == 0)
        {
            emit(lhs, lhs, "-", "1");
            return lhs;
        }
        else if (strcmp(node->value, "-") == 0)
        {
            char *tmp = newTemp();
            emit(tmp, "0", "-", lhs);
            free(lhs);
            return tmp;
        }
        else if (strcmp(node->value, "+") == 0)
        {
            return lhs;
        }
    }

    // Binary operations (+, -, *, /)
    if (node->left && node->right)
    {
        char *left = generateExpression(node->left);
        char *right = generateExpression(node->right);
        char *tmp = newTemp();
        emit(tmp, left, node->value, right);
        free(left);
        free(right);
        return tmp;
    }

    // fallback
    return strdup(node->value ? node->value : "");
}

// === Code Generator ===
static void generateCode(ASTNode *node)
{
    if (!node)
        return;

    switch (node->type)
    {
    case NODE_START:
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
            if (cur->left)
            {
                char *rhs = generateExpression(cur->left);
                if (rhs)
                {
                    emit(cur->value, rhs, "=", NULL);
                    free(rhs);
                }
            }
            cur = cur->right;
        }
        break;
    }

    case NODE_ASSIGNMENT:
    case NODE_EXPRESSION:
    case NODE_POSTFIX_OP:
    case NODE_UNARY_OP:
    {
        char *res = generateExpression(node);
        if (res)
            free(res);
        break;
    }

    default:
        break;
    }
}

// === Optimization: remove redundant temporaries ===
static void removeRedundantTemporaries()
{
    if (codeCount == 0)
        return;

    optimizedCode = malloc(sizeof(TACInstruction) * codeCount);
    memcpy(optimizedCode, code, sizeof(TACInstruction) * codeCount);
    optimizedCount = codeCount;

    for (int i = 0; i < optimizedCount - 1; i++)
    {
        TACInstruction *cur = &optimizedCode[i];
        TACInstruction *next = &optimizedCode[i + 1];

        if (cur->result[0] == 't' &&
            strcmp(next->arg1, cur->result) == 0 &&
            strcmp(next->op, "=") == 0)
        {
            snprintf(next->arg1, sizeof(next->arg1), "%s", cur->arg1);
            snprintf(next->op, sizeof(next->op), "%s", cur->op);
            snprintf(next->arg2, sizeof(next->arg2), "%s", cur->arg2);
            cur->result[0] = '\0';
        }
    }

    int j = 0;
    for (int i = 0; i < optimizedCount; i++)
        if (strlen(optimizedCode[i].result) > 0)
            optimizedCode[j++] = optimizedCode[i];

    optimizedCount = j;
}

// === Display ===
static void displayTAC()
{
    printf("===== INTERMEDIATE CODE (TAC) =====\n");
    for (int i = 0; i < codeCount; i++)
    {
        TACInstruction *inst = &code[i];
        if (strcmp(inst->op, "=") == 0 && strlen(inst->arg2) == 0)
            printf("%s = %s\n", inst->result, inst->arg1);
        else
            printf("%s = %s %s %s\n", inst->result, inst->arg1, inst->op, inst->arg2);
    }
    printf("===== INTERMEDIATE CODE (TAC) END =====\n\n");
}

static void displayOptimizedTAC()
{
    printf("===== OPTIMIZED CODE =====\n");
    for (int i = 0; i < optimizedCount; i++)
    {
        TACInstruction *inst = &optimizedCode[i];
        if (strcmp(inst->op, "=") == 0 && strlen(inst->arg2) == 0)
            printf("%s = %s\n", inst->result, inst->arg1);
        else
            printf("%s = %s %s %s\n", inst->result, inst->arg1, inst->op, inst->arg2);
    }
    printf("===== OPTIMIZED CODE END =====\n\n");
}

// === Public Interface ===
void generate_intermediate_code(ASTNode *root)
{
    if (code)
        free(code);
    if (optimizedCode)
        free(optimizedCode);
    code = NULL;
    optimizedCode = NULL;
    codeCount = 0;
    optimizedCount = 0;
    tempCount = 0;

    if (root)
        generateCode(root);

    displayTAC();
    removeRedundantTemporaries();
    displayOptimizedTAC();
}
