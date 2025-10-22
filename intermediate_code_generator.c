// intermediate_code_generator.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "headers/syntax_analyzer.h"
#include "headers/symbol_table.h"
#include "headers/intermediate_code_generator.h"

typedef struct
{
    char result[64];
    char arg1[64];
    char op[16];
    char arg2[64];
} TACInstruction;

static TACInstruction *code = NULL;
static TACInstruction *optimizedCode = NULL;
static int codeCount = 0;
static int optimizedCount = 0;
static int tempCount = 0;

// === Utility ===
static char *newTemp()
{
    char buf[32];
    snprintf(buf, sizeof(buf), "t%d", tempCount++);
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

    // Handle postfix operators (++ / --)
    if (node->type == NODE_POSTFIX_OP && node->left)
    {
        // For postfix used in expression contexts you'd need to return original value;
        // for standalone statement it's fine to just update x.
        // We'll produce: t0 = x + 1 ; x = t0  (postfix value lost, but works for standalone)
        // If you need exact postfix semantics when used inside larger expressions,
        // you'd emit original value into a temp and then increment.
        if (strcmp(node->value, "++") == 0)
        {
            char *tmp = newTemp();                   // t0
            emit(tmp, node->left->value, "+", "1");  // t0 = x + 1
            emit(node->left->value, tmp, "=", NULL); // x = t0
            free(tmp);
            return strdup(node->left->value);
        }
        else if (strcmp(node->value, "--") == 0)
        {
            char *tmp = newTemp();
            emit(tmp, node->left->value, "-", "1");
            emit(node->left->value, tmp, "=", NULL);
            free(tmp);
            return strdup(node->left->value);
        }
    }

    // Handle prefix operators (++ / --) (NODE_UNARY_OP)
    if (node->type == NODE_UNARY_OP && node->left)
    {
        if (strcmp(node->value, "++") == 0)
        {
            // produce: t0 = x + 1 ; x = t0
            char *tmp = newTemp();
            emit(tmp, node->left->value, "+", "1");
            emit(node->left->value, tmp, "=", NULL);
            free(tmp);
            return strdup(node->left->value);
        }
        else if (strcmp(node->value, "--") == 0)
        {
            char *tmp = newTemp();
            emit(tmp, node->left->value, "-", "1");
            emit(node->left->value, tmp, "=", NULL);
            free(tmp);
            return strdup(node->left->value);
        }
        else if (strcmp(node->value, "-") == 0)
        {
            // unary minus: produce temp = 0 - rhs
            char *rhs = generateExpression(node->left);
            char *tmp = newTemp();
            emit(tmp, "0", "-", rhs);
            free(rhs);
            return tmp;
        }
        else if (strcmp(node->value, "+") == 0)
        {
            return generateExpression(node->left);
        }
    }

    // Handle binary operations (+, -, *, /, etc.)
    if (node->left && node->right)
    {
        char *left = generateExpression(node->left);
        char *right = generateExpression(node->right);
        char *temp = newTemp();
        emit(temp, left, node->value, right);
        free(left);
        free(right);
        return temp;
    }

    // Handle assignment (=, +=, -=, etc.)
    if (node->type == NODE_ASSIGNMENT && strcmp(node->value, "=") == 0 && node->left && node->right)
    {
        char *rhs = generateExpression(node->right);
        emit(node->left->value, rhs, "=", NULL);
        free(rhs);
        return strdup(node->left->value);
    }
    else if (node->type == NODE_ASSIGNMENT &&
             (strcmp(node->value, "+=") == 0 ||
              strcmp(node->value, "-=") == 0 ||
              strcmp(node->value, "*=") == 0 ||
              strcmp(node->value, "/=") == 0) &&
             node->left && node->right)
    {
        char op[2] = {node->value[0], '\0'}; // "+=" -> '+'
        char *rhs = generateExpression(node->right);
        emit(node->left->value, node->left->value, op, rhs);
        free(rhs);
        return strdup(node->left->value);
    }

    // Default fallback
    return strdup(node->value ? node->value : "");
}

// === Code Generator ===
static void generateCode(ASTNode *node)
{
    if (!node)
        return;

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
    case NODE_UNARY_OP: // <-- **ADDED** so prefix unary ops get emitted
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

// === Optimization ===
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
    {
        if (strlen(optimizedCode[i].result) > 0)
            optimizedCode[j++] = optimizedCode[i];
    }
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
    printf("===== OPTIMIZED CODE END =====\n");
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

    if (code)
        free(code);
    if (optimizedCode)
        free(optimizedCode);
}
