// syntax_analyzer.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "headers/syntax_analyzer.h"

// === GLOBALS FROM LEXICAL ANALYZER ===
extern TOKEN tokens[MAX_TOKENS];
extern int token_count;
int current_token = 0;
int syntax_error = 0;
ASTNode *syntax_tree = NULL;

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
    node->left = left;
    node->right = right;
    if (value)
        node->value = strdup(value); // deep copy
    else
        node->value = strdup(""); // keep non-NULL for printing convenience
    if (!node->value)
    {
        fprintf(stderr, "Out of memory duplicating node value\n");
        exit(1);
    }
    return node;
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
            stmt_node = parse_expression();
        }
    }
    else
    {
        stmt_node = parse_expression();
    }

    if (!stmt_node)
        return NULL;

    if (!match(";"))
        error("Missing ';' after statement");

    return create_node(NODE_STATEMENT, "STATEMENT", stmt_node, NULL);
}

// === DECLARATION ===
ASTNode *parse_declaration()
{
    TOKEN *datatype = consume(); // int or char
    ASTNode *decl_list = NULL, *last = NULL;

    while (1)
    {
        TOKEN *id = peek();
        if (!id || id->type != TOK_IDENTIFIER)
        {
            error("Expected identifier in declaration");
            break;
        }
        consume(); // consume identifier

        ASTNode *rhs_node = NULL;
        int initialized = 0;
        char literal_value[MAX_VALUE_LEN] = "";

        if (match("="))
        {
            rhs_node = parse_expression(); // build AST for RHS

            // If RHS is literal, store value in symbol table
            if (rhs_node && rhs_node->type == NODE_FACTOR &&
                (isdigit((unsigned char)rhs_node->value[0]) || rhs_node->value[0] == '\''))
            {
                strncpy(literal_value, rhs_node->value, sizeof(literal_value) - 1);
                literal_value[sizeof(literal_value) - 1] = '\0';
                initialized = 1;
            }
            else
            {
                initialized = 1; // variable is initialized, but RHS is non-literal
            }
        }

        add_symbol(id->lexeme, datatype->lexeme, literal_value, initialized);

        ASTNode *decl_node = create_node(NODE_DECLARATION, id->lexeme, rhs_node, NULL);
        if (!decl_list)
            decl_list = decl_node;
        else
            last->right = decl_node;
        last = decl_node;

        if (!match(","))
            break;
    }

    return create_node(NODE_DECLARATION, datatype->lexeme, decl_list, NULL);
}

// === ASSIGNMENT ===
ASTNode *parse_assignment()
{
    ASTNode *node = NULL;
    node = parse_assignment_element();

    while (match(","))
    {
        ASTNode *right = parse_assignment_element();
        node = create_node(NODE_EXPRESSION, ",", node, right);
    }

    return node;
}

// === ASSIGNMENT ELEMENT ===
ASTNode *parse_assignment_element()
{
    TOKEN *id = peek();
    if (!id || id->type != TOK_IDENTIFIER)
    {
        error("Expected identifier in assignment");
        return NULL;
    }
    consume();

    TOKEN *op = peek();
    if (!op || op->type != TOK_OPERATOR)
    {
        error("Expected operator in assignment");
        return NULL;
    }
    TOKEN op_token = *op;
    consume();

    ASTNode *rhs_node = parse_expression(); // build AST for RHS

    int idx = find_symbol(id->lexeme);
    if (idx != -1)
    {
        // If RHS is literal, update value
        if (rhs_node && rhs_node->type == NODE_FACTOR &&
            (isdigit((unsigned char)rhs_node->value[0]) || rhs_node->value[0] == '\''))
        {
            update_symbol_value(id->lexeme, symbol_table[idx].datatype, rhs_node->value);
        }
        else
        {
            // Mark initialized, but do not store raw expression
            symbol_table[idx].initialized = 1;
        }
    }
    else
    {
        // Variable undeclared, add with empty datatype
        add_symbol(id->lexeme, "", "", 0);
    }

    return create_node(NODE_ASSIGNMENT, op_token.lexeme,
                       create_node(NODE_FACTOR, id->lexeme, NULL, NULL),
                       rhs_node);
}

// === EXPRESSION / TERM / FACTOR ===
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

ASTNode *parse_factor()
{
    TOKEN *tok = peek();
    if (!tok)
        return NULL;

    // Prefix unary operators
    if (tok->type == TOK_OPERATOR &&
        (strcmp(tok->lexeme, "+") == 0 || strcmp(tok->lexeme, "-") == 0 ||
         strcmp(tok->lexeme, "++") == 0 || strcmp(tok->lexeme, "--") == 0))
    {
        TOKEN op_token = *tok;
        consume();
        ASTNode *factor_node = parse_factor();
        return create_node(NODE_UNARY_OP, op_token.lexeme, factor_node, NULL);
    }

    ASTNode *node = NULL;

    // Parentheses
    if (tok->type == TOK_PARENTHESIS && strcmp(tok->lexeme, "(") == 0)
    {
        consume();
        node = parse_expression();
        if (!match(")"))
            error("Missing ')'");
    }
    // Identifiers or literals
    else if (tok->type == TOK_IDENTIFIER || tok->type == TOK_INT_LITERAL || tok->type == TOK_CHAR_LITERAL)
    {
        consume();
        node = create_node(NODE_FACTOR, tok->lexeme, NULL, NULL);

        // Check symbol table for identifiers
        if (tok->type == TOK_IDENTIFIER)
        {
            int idx = find_symbol(tok->lexeme);
            if (idx == -1)
                add_symbol(tok->lexeme, "", "", 0);
        }
    }
    else
    {
        error("Unexpected token in factor");
        consume();
        return NULL;
    }

    // Postfix operators
    tok = peek();
    while (tok && tok->type == TOK_OPERATOR &&
           (strcmp(tok->lexeme, "++") == 0 || strcmp(tok->lexeme, "--") == 0))
    {
        TOKEN op_token = *tok;
        consume();
        node = create_node(NODE_POSTFIX_OP, op_token.lexeme, node, NULL);

        // Optional: update symbol table for identifier
        if (node->left == NULL && node->type == NODE_FACTOR)
        {
            int idx = find_symbol(node->value);
            if (idx != -1)
            {
                char expr[MAX_VALUE_LEN];
                snprintf(expr, MAX_VALUE_LEN, "%s%s", node->value, op_token.lexeme);
                update_symbol_value(node->value, symbol_table[idx].datatype, expr);
            }
        }

        tok = peek();
    }

    return node;
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
    printf("(%s: %s)\n", type_str, node->value ? node->value : "(null)");
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
    if (node->value)
        free(node->value);
    free(node);
}

// === ENTRY ===
void syntax_analyzer()
{
    printf("\n===== SYNTAX ANALYSIS START =====\n");
    current_token = 0;
    syntax_error = 0;

    syntax_tree = parse_program();

    print_ast(syntax_tree, 0);

    if (syntax_error == 0 && current_token == token_count)
        printf("\nSyntax Accepted!\n");
    else
        printf("\nSyntax Rejected (Error found)\n");

    printf("===== SYNTAX ANALYSIS END =====\n\n");

    // DO NOT free syntax_tree here — main or driver will call free_ast(syntax_tree)
}
