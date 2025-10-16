// syntax_analyzer.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "headers/syntax_analyzer.h"
#include "headers/symbol_table.h"

// === GLOBALS FROM LEXICAL ANALYZER ===
extern TOKEN tokens[MAX_TOKENS];
extern int token_count;
int current_token = 0;
int syntax_error = 0;

// === UTILITY FUNCTIONS ===
TOKEN *peek()
{
    if (current_token < token_count)
        return &tokens[current_token];
    return NULL;
}

TOKEN *consume()
{
    if (current_token < token_count)
        return &tokens[current_token++];
    return NULL;
}

int match(const char *lexeme)
{
    TOKEN *tok = peek();
    if (tok && strcmp(tok->lexeme, lexeme) == 0)
    {
        consume();
        return 1;
    }
    return 0;
}

void error(const char *message)
{
    printf("Syntax Error: %s (near token '%s')\n",
           message,
           current_token < token_count ? tokens[current_token].lexeme : "EOF");
    syntax_error = 1;
}

// === AST CREATION ===
ASTNode *create_node(NodeType type, const char *value, ASTNode *left, ASTNode *right)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
    {
        fprintf(stderr, "Out of memory creating AST node\n");
        exit(1);
    }
    node->type = type;
    strcpy(node->value, value ? value : "");
    node->left = left;
    node->right = right;
    return node;
}

// === EVALUATION HELPERS ===
// Evaluate AST node and optionally update symbol table for assignments.
// Returns 1 on success (value computed), places value in *out.
// Supports: integer literals, identifiers (reads symbol_table), unary +/-,
// binary + - * / (nodes of type EXPRESSION/TERM), assignment nodes (NODE_ASSIGNMENT),
// and comma operator represented by node->value == "," (NODE_EXPRESSION).
int eval_node(ASTNode *node, int *out)
{
    if (!node)
        return 0;

    // Factor leaf: integer literal or identifier or char literal
    if (node->type == NODE_FACTOR && node->left == NULL && node->right == NULL)
    {
        const char *s = node->value;
        if (s[0] == '\0')
            return 0;

        // integer literal (optionally signed)
        int i = 0;
        if (s[0] == '+' || s[0] == '-')
            i = 1;
        int all_digits = 1;
        for (int j = i; s[j] != '\0'; j++)
            if (!isdigit((unsigned char)s[j]))
            {
                all_digits = 0;
                break;
            }
        if (all_digits)
        {
            *out = atoi(s);
            return 1;
        }

        // identifier: read from symbol table if exists and is int
        int idx = find_symbol(s);
        if (idx == -1)
            return 0;
        if (strcmp(symbol_table[idx].datatype, "int") == 0)
        {
            *out = symbol_table[idx].value.integer_val;
            return 1;
        }
        return 0;
    }

    // Unary operator stored as NODE_FACTOR with right child (created by parse_factor)
    if (node->type == NODE_FACTOR && node->right != NULL && node->left == NULL &&
        (strcmp(node->value, "+") == 0 || strcmp(node->value, "-") == 0))
    {
        int val;
        if (!eval_node(node->right, &val))
            return 0;
        *out = (strcmp(node->value, "-") == 0) ? -val : val;
        return 1;
    }

    // Comma operator: we used NODE_EXPRESSION with value ","
    if (node->type == NODE_EXPRESSION && strcmp(node->value, ",") == 0)
    {
        int tmp;
        // evaluate left for side-effects, ignore its return
        eval_node(node->left, &tmp);
        // evaluate right and return its value
        return eval_node(node->right, out);
    }

    // Assignment node: left is a factor (identifier), right is expression
    if (node->type == NODE_ASSIGNMENT)
    {
        // left should be a factor with identifier
        if (!node->left || node->left->type != NODE_FACTOR)
            return 0;
        const char *idname = node->left->value;
        if (idname[0] == '\0')
            return 0;

        int rhs;
        if (!eval_node(node->right, &rhs))
            return 0; // cannot evaluate RHS

        int idx = find_symbol(idname);
        if (idx == -1)
        {
            printf("Semantic Error: Undeclared variable '%s'\n", idname);
            return 0;
        }

        // handle operator in node->value: "=", "+=", "-=", "*=", "/="
        if (strcmp(node->value, "=") == 0)
        {
            if (strcmp(symbol_table[idx].datatype, "int") == 0)
            {
                symbol_table[idx].value.integer_val = rhs;
                *out = rhs;
                return 1;
            }
            return 0;
        }
        else if (strcmp(node->value, "+=") == 0)
        {
            if (strcmp(symbol_table[idx].datatype, "int") == 0)
            {
                symbol_table[idx].value.integer_val += rhs;
                *out = symbol_table[idx].value.integer_val;
                return 1;
            }
            return 0;
        }
        else if (strcmp(node->value, "-=") == 0)
        {
            if (strcmp(symbol_table[idx].datatype, "int") == 0)
            {
                symbol_table[idx].value.integer_val -= rhs;
                *out = symbol_table[idx].value.integer_val;
                return 1;
            }
            return 0;
        }
        else if (strcmp(node->value, "*=") == 0)
        {
            if (strcmp(symbol_table[idx].datatype, "int") == 0)
            {
                symbol_table[idx].value.integer_val *= rhs;
                *out = symbol_table[idx].value.integer_val;
                return 1;
            }
            return 0;
        }
        else if (strcmp(node->value, "/=") == 0)
        {
            if (strcmp(symbol_table[idx].datatype, "int") == 0)
            {
                if (rhs == 0)
                {
                    printf("Runtime Error: Division by zero in assignment to '%s'\n", idname);
                    return 0;
                }
                symbol_table[idx].value.integer_val /= rhs;
                *out = symbol_table[idx].value.integer_val;
                return 1;
            }
            return 0;
        }

        // unknown assignment operator
        return 0;
    }

    // Binary arithmetic nodes: NODE_EXPRESSION or NODE_TERM depending on how created
    if ((node->type == NODE_EXPRESSION || node->type == NODE_TERM) && node->left && node->right)
    {
        int l, r;
        if (!eval_node(node->left, &l) || !eval_node(node->right, &r))
            return 0;

        if (strcmp(node->value, "+") == 0)
        {
            *out = l + r;
            return 1;
        }
        if (strcmp(node->value, "-") == 0)
        {
            *out = l - r;
            return 1;
        }
        if (strcmp(node->value, "*") == 0)
        {
            *out = l * r;
            return 1;
        }
        if (strcmp(node->value, "/") == 0)
        {
            if (r == 0)
                return 0;
            *out = l / r;
            return 1;
        }
    }

    return 0;
}

// === FORWARD DECLARATIONS ===
ASTNode *parse_expression();
ASTNode *parse_term();
ASTNode *parse_factor();
ASTNode *parse_assignment_element();

// === PROGRAM ===
ASTNode *parse_program()
{
    return create_node(NODE_PROGRAM, "PROGRAM", parse_statement_list(), NULL);
}

// === STATEMENT_LIST ===
ASTNode *parse_statement_list()
{
    TOKEN *tok = peek();
    if (!tok)
        return NULL;

    ASTNode *stmt = parse_statement();
    if (!stmt)
        return NULL;

    if (!peek())
        return create_node(NODE_STATEMENT_LIST, "STATEMENT_LIST", stmt, NULL);

    ASTNode *next = parse_statement_list();
    return create_node(NODE_STATEMENT_LIST, "STATEMENT_LIST", stmt, next);
}

// === STATEMENT ===
ASTNode *parse_statement()
{
    TOKEN *tok = peek();
    if (!tok)
        return NULL;

    ASTNode *stmt_node = NULL;

    if (tok->type == TOK_DATATYPE)
    {
        stmt_node = parse_declaration();
    }
    else if (tok->type == TOK_IDENTIFIER)
    {
        // Peek next token to decide assignment vs expression
        TOKEN *next = (current_token + 1 < token_count) ? &tokens[current_token + 1] : NULL;

        if (next && next->type == TOK_OPERATOR &&
            (strcmp(next->lexeme, "=") == 0 ||
             strcmp(next->lexeme, "+=") == 0 ||
             strcmp(next->lexeme, "-=") == 0 ||
             strcmp(next->lexeme, "*=") == 0 ||
             strcmp(next->lexeme, "/=") == 0))
        {
            stmt_node = parse_assignment();
        }
        else
        {
            // it's an expression statement starting with identifier
            stmt_node = parse_expression();
        }
    }
    else
    {
        // expression can start with INT_LITERAL, CHAR_LITERAL, '(' or unary +/- or operator
        stmt_node = parse_expression();
    }

    if (!stmt_node)
        return NULL;

    // Require semicolon after any statement form
    if (!match(";"))
    {
        error("Missing ';' after statement");
    }

    return create_node(NODE_STATEMENT, "STATEMENT", stmt_node, NULL);
}

// === DECLARATION ===
ASTNode *parse_declaration()
{
    TOKEN *datatype = consume();
    ASTNode *decl_list = NULL, *last = NULL;

    while (1)
    {
        TOKEN *id = peek();
        if (!id || id->type != TOK_IDENTIFIER)
        {
            error("Expected identifier in declaration");
            break;
        }
        consume();

        ASTNode *init = NULL;
        int initialized = 0, int_val = 0;
        char char_val = '\0';

        if (match("="))
        {
            init = parse_expression();
            if (strcmp(datatype->lexeme, "int") == 0 && eval_node(init, &int_val))
                initialized = 1;
            else if (strcmp(datatype->lexeme, "char") == 0 &&
                     init->type == NODE_FACTOR && strlen(init->value) == 1)
            {
                char_val = init->value[0];
                initialized = 1;
            }
        }

        if (find_symbol(id->lexeme) != -1)
            printf("Semantic Error: Redeclaration of '%s'\n", id->lexeme);
        else
        {
            if (strcmp(datatype->lexeme, "int") == 0)
                add_symbol(id->lexeme, "int", int_val, initialized);
            else
                add_symbol(id->lexeme, "char", char_val, initialized);
        }

        ASTNode *decl = create_node(NODE_DECLARATION, id->lexeme, init, NULL);
        if (!decl_list)
            decl_list = decl;
        else
            last->right = decl;
        last = decl;

        if (match(","))
            continue;
        break;
    }
    return create_node(NODE_DECLARATION, datatype->lexeme, decl_list, NULL);
}

// === ASSIGNMENT ===
// Handles comma-separated list where elements can be assignments or expressions
ASTNode *parse_assignment()
{
    ASTNode *node = NULL;

    // Parse first element
    node = parse_assignment_element();

    // If commas follow, chain them using COMMA expression nodes (value = ",")
    while (match(","))
    {
        ASTNode *right = parse_assignment_element();
        node = create_node(NODE_EXPRESSION, ",", node, right);
    }

    return node;
}

// Helper: parse either assignment or expression element
ASTNode *parse_assignment_element()
{
    TOKEN *tok = peek();
    if (!tok)
        return NULL;

    // Check if it's an identifier followed by assignment operator
    if (tok->type == TOK_IDENTIFIER)
    {
        TOKEN *next = (current_token + 1 < token_count) ? &tokens[current_token + 1] : NULL;
        if (next && next->type == TOK_OPERATOR &&
            (strcmp(next->lexeme, "=") == 0 ||
             strcmp(next->lexeme, "+=") == 0 ||
             strcmp(next->lexeme, "-=") == 0 ||
             strcmp(next->lexeme, "*=") == 0 ||
             strcmp(next->lexeme, "/=") == 0))
        {
            TOKEN *id = consume();
            TOKEN *op = consume();
            ASTNode *expr = parse_expression();

            int idx = find_symbol(id->lexeme);
            if (idx == -1)
                printf("Semantic Error: Undeclared variable '%s'\n", id->lexeme);

            return create_node(NODE_ASSIGNMENT, op->lexeme,
                               create_node(NODE_FACTOR, id->lexeme, NULL, NULL),
                               expr);
        }
    }

    // Otherwise it's just a standalone expression (like b, c, d)
    return parse_expression();
}

// === EXPRESSION ===
ASTNode *parse_expression()
{
    ASTNode *node = parse_term();

    TOKEN *tok = peek();
    while (tok && (strcmp(tok->lexeme, "+") == 0 || strcmp(tok->lexeme, "-") == 0))
    {
        TOKEN *op = consume();
        ASTNode *right = parse_term();
        node = create_node(NODE_EXPRESSION, op->lexeme, node, right);
        tok = peek();
    }
    return node;
}

// === TERM ===
ASTNode *parse_term()
{
    ASTNode *node = parse_factor();

    TOKEN *tok = peek();
    while (tok && (strcmp(tok->lexeme, "*") == 0 || strcmp(tok->lexeme, "/") == 0))
    {
        TOKEN *op = consume();
        ASTNode *right = parse_factor();
        node = create_node(NODE_TERM, op->lexeme, node, right);
        tok = peek();
    }
    return node;
}

// === FACTOR ===
// Handles unary + and -
ASTNode *parse_factor()
{
    TOKEN *tok = peek();
    if (!tok)
        return NULL;

    if (strcmp(tok->lexeme, "+") == 0 || strcmp(tok->lexeme, "-") == 0)
    {
        TOKEN *op = consume();
        ASTNode *right = parse_factor();
        return create_node(NODE_FACTOR, op->lexeme, NULL, right);
    }

    if (tok->type == TOK_IDENTIFIER || tok->type == TOK_INT_LITERAL || tok->type == TOK_CHAR_LITERAL)
    {
        consume();
        if (tok->type == TOK_IDENTIFIER && find_symbol(tok->lexeme) == -1)
            printf("Semantic Error: Undeclared '%s'\n", tok->lexeme);
        return create_node(NODE_FACTOR, tok->lexeme, NULL, NULL);
    }

    if (tok->type == TOK_PARENTHESIS && strcmp(tok->lexeme, "(") == 0)
    {
        consume();
        ASTNode *expr = parse_expression();
        if (!match(")"))
            error("Missing ')'");
        return expr;
    }

    error("Unexpected token in factor");
    consume();
    return NULL;
}

// === EXECUTION: Walk the AST and evaluate non-declaration statements ===
void execute_statements(ASTNode *stmt_list)
{
    if (!stmt_list)
        return;

    // stmt_list is NODE_STATEMENT_LIST: left is statement, right is next
    ASTNode *cur = stmt_list;
    while (cur)
    {
        ASTNode *stmt = cur->left; // STATEMENT node
        if (stmt && stmt->left)    // stmt->left is the inner node
        {
            ASTNode *inner = stmt->left;
            // If it's a declaration node, skip - declarations were handled during parse_declaration()
            if (inner->type != NODE_DECLARATION)
            {
                int val;
                // evaluate the inner expression/assignment/comma-expression to apply assignments / side-effects
                eval_node(inner, &val);
                // we ignore the numeric result for statement, but eval_node will apply assignments
            }
        }
        cur = cur->right; // next statement in list
    }
}

// === AST PRINTER ===
void print_ast(ASTNode *node, int depth)
{
    if (!node)
        return;
    for (int i = 0; i < depth; i++)
        printf("  ");
    const char *type_str =
        node->type == NODE_PROGRAM ? "PROGRAM" : node->type == NODE_STATEMENT_LIST ? "STMT_LIST"
                                             : node->type == NODE_STATEMENT        ? "STMT"
                                             : node->type == NODE_DECLARATION      ? "DECL"
                                             : node->type == NODE_ASSIGNMENT       ? "ASSIGN"
                                             : node->type == NODE_EXPRESSION       ? "EXPR"
                                             : node->type == NODE_TERM             ? "TERM"
                                                                                   : "FACTOR";
    printf("(%s: %s)\n", type_str, node->value);
    print_ast(node->left, depth + 1);
    print_ast(node->right, depth + 1);
}

// === FREE AST ===
void free_ast(ASTNode *node)
{
    if (!node)
        return;
    free_ast(node->left);
    free_ast(node->right);
    free(node);
}

// === ENTRY ===
void syntax_analyzer()
{
    printf("\n===== SYNTAX ANALYSIS START =====\n");
    current_token = 0;
    syntax_error = 0;

    ASTNode *tree = parse_program();
    print_ast(tree, 0);

    // Execute non-declaration statements to update symbol table (assignments, comma expressions, etc.)
    if (tree && tree->left)
        execute_statements(tree->left);

    if (syntax_error == 0 && current_token == token_count)
        printf("\nSyntax Accepted!\n");
    else
        printf("\nSyntax Rejected (Error found)\n");

    printf("===== SYNTAX ANALYSIS END =====\n");

    free_ast(tree);
}
