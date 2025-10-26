// intermediate_code_generator.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "headers/intermediate_code_generator.h"
#include "headers/symbol_table.h"

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

// return 1 if name is an integer literal (optionally signed), 0 otherwise
static int isIntegerLiteral(const char *s)
{
    if (!s || !*s)
        return 0;
    const char *p = s;
    if (*p == '+' || *p == '-')
        p++;
    if (!*p)
        return 0;
    while (*p)
    {
        if (!isdigit((unsigned char)*p))
            return 0;
        p++;
    }
    return 1;
}

// try to get value of a symbol (or temp) or literal; returns 1 if found and sets out_value
static int get_value_if_known(const char *token, int *out_value)
{
    if (!token)
        return 0;

    // literal?
    if (isIntegerLiteral(token))
    {
        *out_value = atoi(token);
        return 1;
    }

    // symbol table
    int idx = find_symbol(token);
    if (idx >= 0 && symbol_table[idx].initialized)
    {
        *out_value = atoi(symbol_table[idx].value_str);
        return 1;
    }

    return 0;
}

// Try constant fold two operands. If folding possible, returns a newly malloc'd string with result.
// If division by zero detected during folding, prints error and exits the compiler.
// If folding not possible, returns NULL.
static char *tryConstantFold(const char *a, const char *op, const char *b)
{
    int va, vb;
    if (!op)
        return NULL;

    if (get_value_if_known(a, &va) && get_value_if_known(b, &vb))
    {
        long long r = 0;
        if (strcmp(op, "+") == 0)
            r = (long long)va + (long long)vb;
        else if (strcmp(op, "-") == 0)
            r = (long long)va - (long long)vb;
        else if (strcmp(op, "*") == 0)
            r = (long long)va * (long long)vb;
        else if (strcmp(op, "/") == 0)
        {
            if (vb == 0)
            {
                fprintf(stderr, "[FATAL] Division by zero detected at compile time: (%s %s %s)\n", a, op, b);
                exit(1);
            }
            r = va / vb;
        }
        else
            return NULL;

        char *buf = malloc(32);
        if (!buf)
        {
            fprintf(stderr, "Out of memory during constant folding\n");
            exit(1);
        }
        snprintf(buf, 32, "%lld", r);
        return buf;
    }
    return NULL;
}

// Add or update a symbol value (used for temporaries and variables when value known)
static void record_symbol_value(const char *name, const char *datatype, const char *value_str)
{
    if (!name)
        return;
    int idx = find_symbol(name);
    if (idx == -1)
    {
        add_symbol(name, datatype ? datatype : "int", value_str ? value_str : "", 1);
    }
    else
    {
        update_symbol_value(name, datatype ? datatype : symbol_table[idx].datatype, value_str ? value_str : "");
    }
}

static void emit(const char *result, const char *arg1, const char *op, const char *arg2)
{
    // Pre-check: if op=='/' attempt to detect division by zero using known values (including temporaries)
    if (op && strcmp(op, "/") == 0 && arg2)
    {
        int val;
        if (get_value_if_known(arg2, &val))
        {
            if (val == 0)
            {
                fprintf(stderr, "[FATAL] Division by zero detected at compile time before emitting: (%s / %s)\n", arg1 ? arg1 : "(null)", arg2);
                exit(1);
            }
        }
        // otherwise unknown divisor -> cannot determine at compile time; still emit TAC
    }

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

            // If RHS can be folded to a known constant (it should already be if generateExpression returned a literal),
            // update symbol table for lhs
            if (isIntegerLiteral(rhs))
            {
                record_symbol_value(lhs, "int", rhs);
            }

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
            char op[2] = {node->value[0], '\0'};
            char *rhs = generateExpression(node->right);

            // Attempt to constant fold lhs (if known) op rhs (if known)
            char *lhs_val = NULL;
            int known_lhs = 0;
            int vlhs;
            if (get_value_if_known(lhs, &vlhs))
            {
                known_lhs = 1;
                lhs_val = malloc(32);
                snprintf(lhs_val, 32, "%d", vlhs);
            }

            char *folded = NULL;
            if (known_lhs)
                folded = tryConstantFold(lhs_val, op, rhs);

            if (folded)
            {
                emit(lhs, folded, "=", NULL);
                record_symbol_value(lhs, "int", folded);
                free(folded);
                if (lhs_val)
                    free(lhs_val);
                free(rhs);
                return strdup(lhs);
            }
            else
            {
                emit(lhs, lhs, op, rhs);
                // If rhs is literal and lhs known, update value by computing (best-effort)
                int v1, v2;
                if (get_value_if_known(lhs, &v1) && get_value_if_known(rhs, &v2))
                {
                    long long r = 0;
                    if (op[0] == '+')
                        r = v1 + v2;
                    else if (op[0] == '-')
                        r = v1 - v2;
                    else if (op[0] == '*')
                        r = (long long)v1 * v2;
                    else if (op[0] == '/')
                    {
                        if (v2 == 0)
                        {
                            fprintf(stderr, "[FATAL] Division by zero during compound assignment\n");
                            exit(1);
                        }
                        r = v1 / v2;
                    }
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%lld", r);
                    record_symbol_value(lhs, "int", buf);
                }
                if (lhs_val)
                    free(lhs_val);
                free(rhs);
                return strdup(lhs);
            }
        }
    }

    // Postfix operations (++ / --)
    if (node->type == NODE_POSTFIX_OP && node->left)
    {
        char *var = generateExpression(node->left);
        char *tmp = newTemp();

        // If var has known value, record temp = var value then update var
        int v;
        if (get_value_if_known(var, &v))
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", v);
            // temp = var
            emit(tmp, buf, "=", NULL);
            record_symbol_value(tmp, "int", buf);
            // var = var +/- 1
            if (strcmp(node->value, "++") == 0)
            {
                emit(var, var, "+", "1");
                char buf2[32];
                snprintf(buf2, sizeof(buf2), "%d", v + 1);
                record_symbol_value(var, "int", buf2);
            }
            else
            {
                emit(var, var, "-", "1");
                char buf2[32];
                snprintf(buf2, sizeof(buf2), "%d", v - 1);
                record_symbol_value(var, "int", buf2);
            }
        }
        else
        {
            // no known value -> emit using runtime temp semantics
            emit(tmp, var, "=", NULL);
            if (strcmp(node->value, "++") == 0)
                emit(var, var, "+", "1");
            else
                emit(var, var, "-", "1");
            // can't record updated var value
        }

        free(var);
        return tmp;
    }

    // Unary operators (++ / -- / + / -)
    if (node->type == NODE_UNARY_OP && node->left)
    {
        char *lhs = generateExpression(node->left);
        if (strcmp(node->value, "++") == 0)
        {
            // pre-increment: update variable then return its name
            emit(lhs, lhs, "+", "1");
            int val;
            if (get_value_if_known(lhs, &val))
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "%d", val + 1);
                record_symbol_value(lhs, "int", buf);
            }
            return lhs;
        }
        else if (strcmp(node->value, "--") == 0)
        {
            emit(lhs, lhs, "-", "1");
            int val;
            if (get_value_if_known(lhs, &val))
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "%d", val - 1);
                record_symbol_value(lhs, "int", buf);
            }
            return lhs;
        }
        else if (strcmp(node->value, "-") == 0)
        {
            char *tmp = newTemp();
            // constant fold if possible
            char *folded = tryConstantFold("0", "-", lhs);
            if (folded)
            {
                // we can return folded literal and free lhs
                free(lhs);
                return folded;
            }
            emit(tmp, "0", "-", lhs);
            free(lhs);
            // attempt to record tmp value if lhs known
            int vlhs;
            if (get_value_if_known(lhs, &vlhs))
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "%d", -vlhs);
                record_symbol_value(tmp, "int", buf);
            }
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
        // Generate subexpressions first
        char *left = generateExpression(node->left);
        char *right = generateExpression(node->right);

        // attempt constant folding using known values or literals
        char *folded = tryConstantFold(left, node->value, right);
        if (folded)
        {
            free(left);
            free(right);
            return folded; // a literal string (malloc'd) representing the folded value
        }

        // Otherwise emit TAC
        char *tmp = newTemp();
        emit(tmp, left, node->value, right);

        // If left and right are known now (maybe they were literals or known symbols),
        // compute value for tmp and record it in symbol table
        int vl, vr;
        if (get_value_if_known(left, &vl) && get_value_if_known(right, &vr))
        {
            long long r = 0;
            if (strcmp(node->value, "+") == 0)
                r = (long long)vl + vr;
            else if (strcmp(node->value, "-") == 0)
                r = (long long)vl - vr;
            else if (strcmp(node->value, "*") == 0)
                r = (long long)vl * vr;
            else if (strcmp(node->value, "/") == 0)
            {
                if (vr == 0)
                {
                    fprintf(stderr, "[FATAL] Division by zero detected at compile time: (%d / %d)\n", vl, vr);
                    exit(1);
                }
                r = vl / vr;
            }
            char buf[32];
            snprintf(buf, sizeof(buf), "%lld", r);
            record_symbol_value(tmp, "int", buf);
        }

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
                    // If RHS is a known literal, record variable value
                    if (isIntegerLiteral(rhs))
                        record_symbol_value(cur->value, "int", rhs);

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

        if (cur->result[0] == 't' && strcmp(next->arg1, cur->result) == 0 && strcmp(next->op, "=") == 0)
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
