// semantic_analyzer.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "headers/semantic_analyzer.h"
#include "headers/symbol_table.h"    /* your provided symbol table header */
#include "headers/syntax_analyzer.h" /* ASTNode and syntax_tree global */

/* ----------------- Internal storage ----------------- */

/* Temps array (for bookkeeping, you can inspect if needed) */
static SEM_TEMP *sem_temps = NULL;
static size_t sem_temps_capacity = 0;
static size_t sem_temps_count = 0;
static int sem_next_temp_id = 1;

/* Known-vars linked list (semantic-only) */
static KnownVar *known_vars_head = NULL;

/* error tracking */
static int sem_errors = 0;

/* ----------------- Helpers ----------------- */

static void sem_record_error(ASTNode *node, const char *fmt, ...)
{
    sem_errors++;
    fprintf(stderr, "Semantic Error: ");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (node && node->value)
        fprintf(stderr, " (node: '%s')", node->value);
    fprintf(stderr, "\n");
}

static int ensure_temp_capacity(void)
{
    if (sem_temps_count + 1 > sem_temps_capacity)
    {
        size_t newcap = sem_temps_capacity == 0 ? 256 : sem_temps_capacity * 2;
        SEM_TEMP *newbuf = realloc(sem_temps, newcap * sizeof(SEM_TEMP));
        if (!newbuf)
        {
            fprintf(stderr, "Out of memory allocating semantic temps\n");
            return 0;
        }
        sem_temps = newbuf;
        sem_temps_capacity = newcap;
    }
    return 1;
}

static SEM_TEMP make_temp(SEM_TYPE type, int is_const, long val, ASTNode *node)
{
    if (!ensure_temp_capacity())
    {
        /* fatal */
        exit(1);
    }
    SEM_TEMP t;
    t.id = sem_next_temp_id++;
    t.type = type;
    t.is_constant = is_const;
    t.int_value = val;
    t.node = node;
    sem_temps[sem_temps_count++] = t;
    return t;
}

/* Known var helpers */
static KnownVar *find_known_var(const char *name)
{
    KnownVar *k = known_vars_head;
    while (k)
    {
        if (strcmp(k->name, name) == 0)
            return k;
        k = k->next;
    }
    return NULL;
}

static void set_known_var(const char *name, SEM_TEMP t)
{
    KnownVar *k = find_known_var(name);
    if (k)
    {
        k->temp = t;
        return;
    }
    k = malloc(sizeof(KnownVar));
    k->name = strdup(name);
    k->temp = t;
    k->next = known_vars_head;
    known_vars_head = k;
}

static void remove_known_var(const char *name)
{
    KnownVar **pp = &known_vars_head;
    while (*pp)
    {
        if (strcmp((*pp)->name, name) == 0)
        {
            KnownVar *rem = *pp;
            *pp = rem->next;
            free(rem->name);
            free(rem);
            return;
        }
        pp = &(*pp)->next;
    }
}

/* convert symbol_table datatype to SEM_TYPE */
static SEM_TYPE datatype_to_semtype(const char *dt)
{
    if (!dt)
        return SEM_TYPE_UNKNOWN;
    if (strcmp(dt, "int") == 0)
        return SEM_TYPE_INT;
    if (strcmp(dt, "char") == 0)
        return SEM_TYPE_CHAR;
    return SEM_TYPE_UNKNOWN;
}

/* try parse int literal string; returns 1 if succeeded */
static int try_parse_int(const char *s, long *out)
{
    if (!s)
        return 0;
    char *end;
    long v = strtol(s, &end, 10);
    if (end != s && *end == '\0')
    {
        *out = v;
        return 1;
    }
    return 0;
}

/* lookup variable: first check known_vars (semantic-only), else check symbol_table
   return 1 if variable exists, and fills out SEM_TEMP (is_constant if known), 0 otherwise */
static int lookup_variable_as_temp(const char *name, SEM_TEMP *out)
{
    if (!name)
        return 0;

    /* check semantic-known values first */
    KnownVar *k = find_known_var(name);
    if (k)
    {
        *out = k->temp;
        return 1;
    }

    /* check symbol table: find_symbol returns index or -1 */
    int idx = find_symbol(name);
    if (idx == -1)
        return 0; /* undeclared */

    /* Use datatype from symbol_table, and if symbol has initialized==1 and value_str non-empty,
       and the value_str parses as integer, treat as compile-time known. */
    SEM_TYPE t = datatype_to_semtype(symbol_table[idx].datatype);

    if (symbol_table[idx].initialized && symbol_table[idx].value_str[0] != '\0')
    {
        long v;
        if (try_parse_int(symbol_table[idx].value_str, &v))
        {
            *out = make_temp(t, 1, v, NULL);
            return 1;
        }
    }

    /* not compile-time-known, return non-constant temp (type from symbol) */
    *out = make_temp(t, 0, 0, NULL);
    return 1;
}

/* ----------------- Expression evaluation -----------------
   The evaluator returns a SEM_TEMP describing:
     - type (SEM_TYPE)
     - is_constant (1 if compile-time-known)
     - int_value (valid only if is_constant and numeric)
   It also emits semantic errors (undeclared var, division by zero, assignment to undeclared).
   --------------------------------------------------------- */

static SEM_TEMP evaluate_expression(ASTNode *node); /* forward */

/* evaluate factor (literals, identifiers, parenthesis handled by tree shape) */
static SEM_TEMP eval_factor(ASTNode *node)
{
    if (!node)
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);

    if (node->type == NODE_FACTOR)
    {
        const char *lex = node->value;
        if (!lex)
            return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);

        /* integer literal */
        long v;
        if (try_parse_int(lex, &v))
        {
            return make_temp(SEM_TYPE_INT, 1, v, node);
        }

        /* char literal like 'a' (simple detection) */
        if (lex[0] == '\'' && lex[1] != '\0' && lex[2] == '\'' && lex[3] == '\0')
        {
            long cv = (unsigned char)lex[1];
            return make_temp(SEM_TYPE_CHAR, 1, cv, node);
        }

        /* identifier */
        if (isalpha((unsigned char)lex[0]) || lex[0] == '_')
        {
            SEM_TEMP tv;
            if (!lookup_variable_as_temp(lex, &tv))
            {
                sem_record_error(node, "Undeclared identifier '%s'", lex);
                return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
            }
            /* attach node for better diagnostics if needed */
            tv.node = node;
            return tv;
        }

        /* fallback */
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
    }

    /* if not exactly NODE_FACTOR, evaluate as general expression (handles parentheses) */
    return evaluate_expression(node);
}

/* evaluate term nodes (handles * and /) */
static SEM_TEMP eval_term(ASTNode *node)
{
    if (!node)
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);

    /* If parser produced nested terms left-associatively, left may be TERM and right is FACTOR */
    if (node->type == NODE_TERM)
    {
        SEM_TEMP L = eval_term(node->left);
        SEM_TEMP R = eval_factor(node->right);

        /* If type mismatches matter: we treat char as int for arithmetic */
        if ((L.type != SEM_TYPE_INT && L.type != SEM_TYPE_CHAR && L.type != SEM_TYPE_UNKNOWN) ||
            (R.type != SEM_TYPE_INT && R.type != SEM_TYPE_CHAR && R.type != SEM_TYPE_UNKNOWN))
        {
            /* we don't support other types currently, but we won't crash */
        }

        const char *op = node->value ? node->value : "";

        /* Division by zero detection: only if right is constant numeric and equals 0 */
        if (strcmp(op, "/") == 0)
        {
            if (R.is_constant)
            {
                if (R.int_value == 0)
                {
                    sem_record_error(node, "Division by zero detected at compile time");
                    return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
                }
            }
            /* otherwise cannot detect at compile-time */
        }

        /* Constant folding */
        if (L.is_constant && R.is_constant)
        {
            if (strcmp(op, "*") == 0)
            {
                long val = L.int_value * R.int_value;
                return make_temp(SEM_TYPE_INT, 1, val, node);
            }
            else if (strcmp(op, "/") == 0)
            {
                /* we already checked R.int_value != 0 above */
                if (R.int_value == 0)
                {
                    return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
                }
                long val = L.int_value / R.int_value;
                return make_temp(SEM_TYPE_INT, 1, val, node);
            }
        }

        /* unknown or partially-known result => type int non-constant */
        return make_temp(SEM_TYPE_INT, 0, 0, node);
    }

    /* fallback */
    return eval_factor(node);
}

/* evaluate additive / expression nodes (handles + and -). Parser uses NODE_EXPRESSION for + and - */
static SEM_TEMP eval_additive(ASTNode *node)
{
    if (!node)
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);

    if (node->type == NODE_EXPRESSION)
    {
        SEM_TEMP L = eval_additive(node->left);
        SEM_TEMP R = eval_term(node->right);

        const char *op = node->value ? node->value : "";

        /* constant folding when possible */
        if (L.is_constant && R.is_constant)
        {
            if (strcmp(op, "+") == 0)
            {
                long val = L.int_value + R.int_value;
                return make_temp(SEM_TYPE_INT, 1, val, node);
            }
            else if (strcmp(op, "-") == 0)
            {
                long val = L.int_value - R.int_value;
                return make_temp(SEM_TYPE_INT, 1, val, node);
            }
        }

        /* otherwise return non-const int result */
        return make_temp(SEM_TYPE_INT, 0, 0, node);
    }

    return eval_term(node);
}

/* evaluate assignment nodes */
static SEM_TEMP eval_assignment(ASTNode *node)
{
    if (!node)
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);

    /* node->left should be a NODE_FACTOR describing the identifier */
    ASTNode *lhs = node->left;
    ASTNode *rhs = node->right;
    if (!lhs || lhs->type != NODE_FACTOR)
    {
        sem_record_error(node, "Invalid assignment LHS");
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
    }
    const char *varname = lhs->value;
    if (!varname)
    {
        sem_record_error(node, "Invalid identifier on LHS");
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
    }

    /* Evaluate RHS - parser may create nested assignment (right-recursive) */
    SEM_TEMP rhs_temp;
    if (rhs && rhs->type == NODE_ASSIGNMENT)
    {
        rhs_temp = eval_assignment(rhs);
    }
    else
    {
        rhs_temp = evaluate_expression(rhs);
    }

    /* check var declared in symbol table */
    int idx = find_symbol(varname);
    if (idx == -1)
    {
        sem_record_error(node, "Assignment to undeclared variable '%s'", varname);
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
    }

    /* basic type compatibility check (we only support int/char) */
    SEM_TYPE lhs_type = datatype_to_semtype(symbol_table[idx].datatype);
    if (lhs_type == SEM_TYPE_UNKNOWN)
    {
        sem_record_error(node, "Unsupported LHS type for '%s'", varname);
    }
    else
    {
        /* If RHS is constant, we may record compile-time known value in semantic store */
        if (rhs_temp.is_constant)
        {
            SEM_TEMP store_temp = make_temp(lhs_type, 1, rhs_temp.int_value, node);
            set_known_var(varname, store_temp);
        }
        else
        {
            /* RHS not constant: remove known_var entry (we are not allowed to change symbol_table) */
            remove_known_var(varname);
        }

        /* optionally check assignment type mismatch: (here we are permissive: char <- int allowed) */
    }

    /* Return temp representing the assigned value (if language allows assignment expressions) */
    return make_temp(lhs_type, rhs_temp.is_constant, rhs_temp.int_value, node);
}

/* Generic dispatcher */
static SEM_TEMP evaluate_expression(ASTNode *node)
{
    if (!node)
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);

    switch (node->type)
    {
    case NODE_ASSIGNMENT:
        return eval_assignment(node);
    case NODE_EXPRESSION:
        return eval_additive(node);
    case NODE_TERM:
        return eval_term(node);
    case NODE_FACTOR:
        return eval_factor(node);
    case NODE_UNARY_OP:
    {
        /* handle +, -, ++, -- (conservatively) */
        ASTNode *operand = node->left;
        SEM_TEMP t = evaluate_expression(operand);
        if (t.is_constant)
        {
            if (strcmp(node->value, "+") == 0)
                return t;
            if (strcmp(node->value, "-") == 0)
                return make_temp(t.type, 1, -t.int_value, node);
            /* ++/-- make value non-constant and may modify variable (we conservatively drop constness) */
            if (strcmp(node->value, "++") == 0 || strcmp(node->value, "--") == 0)
            {
                return make_temp(t.type, 0, 0, node);
            }
        }
        else
        {
            return make_temp(t.type, 0, 0, node);
        }
        break;
    }
    case NODE_POSTFIX_OP:
    {
        /* i++/i-- produce non-const */
        return make_temp(SEM_TYPE_INT, 0, 0, node);
    }
    default:
        /* For Node types not explicitly handled, evaluate children if present */
        if (node->left)
            evaluate_expression(node->left);
        if (node->right)
            evaluate_expression(node->right);
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
    }
    return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
}

/* Walk statement list created by parser and analyze each statement */
static void analyze_statement_list(ASTNode *stmt_list)
{
    for (ASTNode *cur = stmt_list; cur; cur = cur->right)
    {
        if (!cur)
            break;

        /* Our parser wraps statements in NODE_STATEMENT_LIST where left points to NODE_STATEMENT */
        if (cur->type == NODE_STATEMENT_LIST)
        {
            ASTNode *stmt_wrapper = cur->left;
            if (!stmt_wrapper)
                continue;
            if (stmt_wrapper->type == NODE_STATEMENT && stmt_wrapper->left)
            {
                ASTNode *stmt = stmt_wrapper->left;
                if (!stmt)
                    continue;

                if (stmt->type == NODE_DECLARATION)
                {
                    /* declaration: node->value holds datatype, left points to the linked list of declarators */
                    ASTNode *decls = stmt->left;
                    for (ASTNode *d = decls; d; d = d->right)
                    {
                        if (!d)
                            break;
                        /* parser created decl nodes where d->value is identifier and d->left is initializer subtree */
                        const char *idname = d->value;
                        ASTNode *initializer = d->left;
                        if (initializer)
                        {
                            SEM_TEMP val = evaluate_expression(initializer);
                            if (val.is_constant)
                            {
                                /* record compile-time known initializer in semantic-only store */
                                SEM_TEMP store_temp = make_temp(datatype_to_semtype(stmt->value), 1, val.int_value, d);
                                set_known_var(idname, store_temp);
                            }
                        }
                    }
                }
                else
                {
                    /* assignment or expression */
                    evaluate_expression(stmt);
                }
            }
            else
            {
                /* unexpected shape: still try to evaluate children */
                if (cur->left)
                    evaluate_expression(cur->left);
            }
        }
        else
        {
            /* If parser produced bare statement nodes (defensive) */
            if (cur->type == NODE_STATEMENT && cur->left)
            {
                evaluate_expression(cur->left);
            }
            else
            {
                /* fallback evaluate */
                if (cur->left)
                    evaluate_expression(cur->left);
                if (cur->right)
                    evaluate_expression(cur->right);
            }
        }
    }
}

/* ----------------- Public API ----------------- */

int semantic_analyzer(void)
{
    /* reset state */
    sem_errors = 0;
    sem_temps_count = 0;
    sem_next_temp_id = 1;

    /* free known_vars if any */
    while (known_vars_head)
    {
        KnownVar *n = known_vars_head;
        known_vars_head = n->next;
        free(n->name);
        free(n);
    }

    if (!syntax_tree)
    {
        fprintf(stderr, "Semantic Analyzer: no syntax tree available\n");
        return 1;
    }

    /* the parser sets syntax_tree to NODE_START whose left is statement_list */
    ASTNode *stmts = syntax_tree->left;
    if (!stmts)
    {
        /* nothing to analyze */
        return 0;
    }

    analyze_statement_list(stmts);

    if (sem_errors == 0)
    {
        printf("Semantic Analysis: no errors found.\n");
    }
    else
    {
        printf("Semantic Analysis: %d error(s) detected.\n", sem_errors);
    }

    return sem_errors;
}

int semantic_error_count(void) { return sem_errors; }
