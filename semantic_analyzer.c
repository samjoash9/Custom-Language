#include "headers/semantic_analyzer.h"

static SEM_TEMP *sem_temps = NULL;
static size_t sem_temps_capacity = 0;
static size_t sem_temps_count = 0;
static int sem_next_temp_id = 1;

static KnownVar *known_vars_head = NULL;
static int sem_errors = 0;

/* ----------------- Forward declarations ----------------- */
static SEM_TEMP evaluate_expression(ASTNode *node);
static void analyze_statement_list(ASTNode *stmt_list);

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

static void sem_record_warning(ASTNode *node, const char *fmt, ...)
{
    fprintf(stderr, "Semantic Warning: ");
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
    ensure_temp_capacity();
    SEM_TEMP t;
    t.id = sem_next_temp_id++;
    t.type = type;
    t.is_constant = is_const;
    t.int_value = val;
    t.node = node;
    sem_temps[sem_temps_count++] = t;
    return t;
}

/* ----------------- Known variable management ----------------- */

static KnownVar *find_known_var(const char *name)
{
    for (KnownVar *k = known_vars_head; k; k = k->next)
        if (strcmp(k->name, name) == 0)
            return k;
    return NULL;
}

/* set_known_var stores a semantic-only value for a variable.
   Does NOT modify the original symbol_table. Initializes used=0 unless it exists already. */
static void set_known_var(const char *name, SEM_TEMP t, int initialized)
{
    KnownVar *k = find_known_var(name);
    if (k)
    {
        k->temp = t;
        k->initialized = initialized;
        /* do not reset 'used' because prior uses are still relevant */
        return;
    }
    k = malloc(sizeof(KnownVar));
    if (!k)
    {
        fprintf(stderr, "Out of memory in set_known_var\n");
        exit(1);
    }
    k->name = strdup(name);
    k->temp = t;
    k->initialized = initialized;
    k->used = 0;
    k->next = known_vars_head;
    known_vars_head = k;
}

/* remove semantic-known entry (e.g., on non-constant assignment) */
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

/* mark variable as used (for warnings) */
static void mark_known_var_used(const char *name)
{
    KnownVar *k = find_known_var(name);
    if (k)
        k->used = 1;
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

/* ----------------- Try evaluate subtree as constant -----------------
   Returns 1 if the subtree is compile-time-evaluable to an integer, with value in *out.
   Uses:
     - integer literals
     - char literals
     - semantic-known variables (known_vars list)
     - symbol_table entries if initialized and value_str parsable
     - recursively evaluates +, -, *, / when operands are constant
   Conservative: returns 0 if any part is unknown or division by zero would occur.
   --------------------------------------------------------------- */
static int try_eval_constant(ASTNode *node, long *out)
{
    if (!node)
        return 0;

    if (node->type == NODE_FACTOR)
    {
        const char *lex = node->value;
        if (!lex)
            return 0;

        long v;
        if (try_parse_int(lex, &v))
        {
            *out = v;
            return 1;
        }

        /* char literal like 'a' */
        if (lex[0] == '\'' && lex[2] == '\'' && lex[3] == '\0')
        {
            *out = (unsigned char)lex[1];
            return 1;
        }

        /* identifier: check semantic-known first, then symbol table */
        if (isalpha((unsigned char)lex[0]) || lex[0] == '_')
        {
            KnownVar *k = find_known_var(lex);
            if (k)
            {
                if (k->initialized && k->temp.is_constant)
                {
                    *out = k->temp.int_value;
                    return 1;
                }
                return 0;
            }
            int idx = find_symbol(lex);
            if (idx != -1 && symbol_table[idx].initialized && symbol_table[idx].value_str[0] != '\0')
            {
                long vv;
                if (try_parse_int(symbol_table[idx].value_str, &vv))
                {
                    *out = vv;
                    return 1;
                }
            }
            return 0;
        }
        return 0;
    }

    if (node->type == NODE_TERM)
    {
        long L, R;
        if (!try_eval_constant(node->left, &L))
            return 0;
        if (!try_eval_constant(node->right, &R))
            return 0;
        const char *op = node->value ? node->value : "";
        if (strcmp(op, "*") == 0)
        {
            *out = L * R;
            return 1;
        }
        if (strcmp(op, "/") == 0)
        {
            if (R == 0)
                return 0; /* avoid division by zero here; caller handles detection as error */
            *out = L / R;
            return 1;
        }
        return 0;
    }

    if (node->type == NODE_EXPRESSION)
    {
        long L, R;
        if (!try_eval_constant(node->left, &L))
            return 0;
        if (!try_eval_constant(node->right, &R))
            return 0;
        const char *op = node->value ? node->value : "";
        if (strcmp(op, "+") == 0)
        {
            *out = L + R;
            return 1;
        }
        if (strcmp(op, "-") == 0)
        {
            *out = L - R;
            return 1;
        }
        return 0;
    }

    if (node->type == NODE_UNARY_OP)
    {
        long v;
        if (!try_eval_constant(node->left, &v))
            return 0;
        const char *op = node->value ? node->value : "";
        if (strcmp(op, "+") == 0)
        {
            *out = v;
            return 1;
        }
        if (strcmp(op, "-") == 0)
        {
            *out = -v;
            return 1;
        }
        return 0;
    }

    return 0;
}

/* ----------------- Expression evaluation ----------------- */

/* evaluate factor (literals, identifiers, parentheses handled by parser) */
static SEM_TEMP eval_factor(ASTNode *node)
{
    if (!node)
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);

    if (node->type == NODE_FACTOR)
    {
        const char *lex = node->value;
        if (!lex)
            return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);

        long v;
        if (try_parse_int(lex, &v))
            return make_temp(SEM_TYPE_INT, 1, v, node);

        if (lex[0] == '\'' && lex[1] != '\0' && lex[2] == '\'' && lex[3] == '\0')
        {
            long cv = (unsigned char)lex[1];
            return make_temp(SEM_TYPE_CHAR, 1, cv, node);
        }

        if (isalpha((unsigned char)lex[0]) || lex[0] == '_')
        {
            /* check semantic-known entry first */
            KnownVar *k = find_known_var(lex);
            if (k)
            {
                /* mark used for later "declared but never used" detection */
                k->used = 1;

                /* If variable is not initialized, warn (but do not error) */
                if (!k->initialized)
                {
                    sem_record_warning(node, "Use of uninitialized variable '%s'", lex);
                }

                SEM_TEMP tv = k->temp;
                tv.node = node;
                return tv;
            }

            /* fallback to symbol table */
            int idx = find_symbol(lex);
            if (idx == -1)
            {
                sem_record_error(node, "Undeclared identifier '%s'", lex);
                return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
            }

            /* mark usage for later warnings: create or update known var placeholder so we can track used status */
            SEM_TEMP placeholder = make_temp(datatype_to_semtype(symbol_table[idx].datatype), 0, 0, node);
            set_known_var(lex, placeholder, symbol_table[idx].initialized);
            KnownVar *newk = find_known_var(lex);
            if (newk)
                newk->used = 1;

            /* If symbol_table says not initialized, warn (but don't error) */
            if (!symbol_table[idx].initialized)
            {
                sem_record_warning(node, "Use of uninitialized variable '%s'", lex);
            }
            else
            {
                /* if symbol table has a parseable initializer, return constant temp */
                long vv;
                if (symbol_table[idx].value_str[0] != '\0' && try_parse_int(symbol_table[idx].value_str, &vv))
                {
                    return make_temp(datatype_to_semtype(symbol_table[idx].datatype), 1, vv, node);
                }
            }

            return make_temp(datatype_to_semtype(symbol_table[idx].datatype), 0, 0, node);
        }

        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
    }

    /* parentheses / other shapes: delegate */
    return evaluate_expression(node);
}

/* evaluate term nodes (handles * and /) */
static SEM_TEMP eval_term(ASTNode *node)
{
    if (!node)
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
    if (node->type != NODE_TERM)
        return eval_factor(node);

    SEM_TEMP L = eval_term(node->left);
    SEM_TEMP R = eval_factor(node->right);
    const char *op = node->value ? node->value : "";

    if (strcmp(op, "/") == 0)
    {
        /* if right is constant and zero -> error */
        if (R.is_constant && R.int_value == 0)
        {
            sem_record_error(node, "Division by zero detected at compile time");
            return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
        }

        /* else, attempt to resolve subtree to constant and check */
        long denom;
        if (try_eval_constant(node->right, &denom))
        {
            if (denom == 0)
            {
                sem_record_error(node, "Division by zero detected at compile time");
                return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
            }
        }
    }

    /* constant folding */
    if (L.is_constant && R.is_constant)
    {
        if (strcmp(op, "*") == 0)
        {
            long val = L.int_value * R.int_value;
            return make_temp(SEM_TYPE_INT, 1, val, node);
        }
        else if (strcmp(op, "/") == 0)
        {
            if (R.int_value == 0)
                return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
            long val = L.int_value / R.int_value;
            return make_temp(SEM_TYPE_INT, 1, val, node);
        }
    }

    return make_temp(SEM_TYPE_INT, 0, 0, node);
}

/* evaluate additive / expression nodes (handles + and -) */
static SEM_TEMP eval_additive(ASTNode *node)
{
    if (!node)
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
    if (node->type != NODE_EXPRESSION)
        return eval_term(node);

    SEM_TEMP L = eval_additive(node->left);
    SEM_TEMP R = eval_term(node->right);
    const char *op = node->value ? node->value : "";

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

    return make_temp(SEM_TYPE_INT, 0, 0, node);
}

/* evaluate assignment nodes */
static SEM_TEMP eval_assignment(ASTNode *node)
{
    if (!node)
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);

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

    SEM_TEMP rhs_temp;
    if (rhs && rhs->type == NODE_ASSIGNMENT)
        rhs_temp = eval_assignment(rhs);
    else
        rhs_temp = evaluate_expression(rhs);

    int idx = find_symbol(varname);
    if (idx == -1)
    {
        sem_record_error(node, "Assignment to undeclared variable '%s'", varname);
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
    }

    if (rhs_temp.is_constant)
    {
        SEM_TEMP store_temp = make_temp(datatype_to_semtype(symbol_table[idx].datatype), 1, rhs_temp.int_value, node);
        set_known_var(varname, store_temp, 1);
    }
    else
    {
        /* We don't know the value at compile-time; mark as declared but not semantically-initialized */
        SEM_TEMP placeholder = make_temp(datatype_to_semtype(symbol_table[idx].datatype), 0, 0, node);
        set_known_var(varname, placeholder, 0);
        /* remove_known_var(varname);  // we keep placeholder so we can warn about unused later */
    }

    return make_temp(datatype_to_semtype(symbol_table[idx].datatype), rhs_temp.is_constant, rhs_temp.int_value, node);
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
        SEM_TEMP t = evaluate_expression(node->left);
        if (t.is_constant)
        {
            if (strcmp(node->value, "+") == 0)
                return t;
            if (strcmp(node->value, "-") == 0)
                return make_temp(t.type, 1, -t.int_value, node);
        }
        return make_temp(t.type, 0, 0, node);
    }
    case NODE_POSTFIX_OP:
        return make_temp(SEM_TYPE_INT, 0, 0, node);
    default:
        if (node->left)
            evaluate_expression(node->left);
        if (node->right)
            evaluate_expression(node->right);
        return make_temp(SEM_TYPE_UNKNOWN, 0, 0, node);
    }
}

/* ----------------- Statement analysis ----------------- */

static void analyze_statement_list(ASTNode *stmt_list)
{
    for (ASTNode *cur = stmt_list; cur; cur = cur->right)
    {
        if (!cur)
            break;

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
                    ASTNode *decls = stmt->left;
                    while (decls)
                    {
                        const char *idname = decls->value;
                        ASTNode *initializer = decls->left;
                        if (initializer)
                        {
                            SEM_TEMP val = evaluate_expression(initializer);
                            if (val.is_constant)
                            {
                                SEM_TEMP store_temp = make_temp(datatype_to_semtype(stmt->value), 1, val.int_value, decls);
                                set_known_var(idname, store_temp, 1);
                            }
                            else
                            {
                                SEM_TEMP placeholder = make_temp(datatype_to_semtype(stmt->value), 0, 0, decls);
                                set_known_var(idname, placeholder, 0);
                            }
                        }
                        else
                        {
                            SEM_TEMP placeholder = make_temp(datatype_to_semtype(stmt->value), 0, 0, decls);
                            set_known_var(idname, placeholder, 0);
                        }

                        decls = decls->right;
                    }
                }
                else
                {
                    evaluate_expression(stmt);
                }
            }
            else
            {
                if (cur->left)
                    evaluate_expression(cur->left);
            }
        }
        else
        {
            if (cur->left)
                evaluate_expression(cur->left);
            if (cur->right)
                evaluate_expression(cur->right);
        }
    }

    /* After pass: produce warnings for declared-but-never-initialized-or-used variables */
    for (int i = 0; i < symbol_count; ++i)
    {
        const char *name = symbol_table[i].name;
        int sym_init = symbol_table[i].initialized;

        KnownVar *k = find_known_var(name);
        if (!k)
        {
            /* if symbol declared and not initialized in symbol table, warn */
            if (!sym_init)
            {
                sem_record_warning(NULL, "Variable '%s' declared but never initialized or used", name);
            }
            continue;
        }

        if (!sym_init)
        {
            /* if we have known-var placeholder and it was never used and not initialized semantically -> warn */
            if (!k->initialized && !k->used)
            {
                sem_record_warning(k->temp.node, "Variable '%s' declared but never initialized or used", name);
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

    ASTNode *stmts = syntax_tree->left;
    if (!stmts)
        return 0;

    analyze_statement_list(stmts);

    if (sem_errors == 0)
        printf("Semantic Analysis: no errors found.\n");
    else
        printf("Semantic Analysis: %d error(s) detected.\n", sem_errors);

    return sem_errors;
}

int semantic_error_count(void) { return sem_errors; }