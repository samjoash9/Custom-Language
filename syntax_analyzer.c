// syntax_analyzer.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "headers/syntax_analyzer.h"

// === GLOBALS FROM LEXICAL ANALYZER ===
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
    node->value = value ? strdup(value) : strdup("");
    if (!node->value)
    {
        fprintf(stderr, "Out of memory duplicating node value\n");
        exit(1);
    }
    return node;
}

// === PROGRAM ===
// S → STATEMENT_LIST
ASTNode *parse_program()
{
    return create_node(NODE_START, "START", parse_statement_list(), NULL);
}

// === STATEMENT LIST ===
// STATEMENT_LIST → STATEMENT STATEMENT_LIST | ε
ASTNode *parse_statement_list()
{
    ASTNode *head = NULL; // holds the first statement list
    ASTNode *tail = NULL; // most recent node

    // loop continues reading tokens until no valid statements are left.
    while (1)
    {
        TOKEN *tok = peek();
        if (!tok)
            break;

        // Determine if token can start a statement. If not, stop. (mimic epsilon)
        if (!(tok->type == TOK_DATATYPE ||
              tok->type == TOK_IDENTIFIER ||
              tok->type == TOK_INT_LITERAL ||
              tok->type == TOK_CHAR_LITERAL ||
              (tok->type == TOK_PARENTHESIS && strcmp(tok->lexeme, "(") == 0) ||
              (tok->type == TOK_OPERATOR &&
               (strcmp(tok->lexeme, "+") == 0 || strcmp(tok->lexeme, "-") == 0 ||
                strcmp(tok->lexeme, "++") == 0 || strcmp(tok->lexeme, "--") == 0))))
        {
            break;
        }

        // STATEMENT → DECLARATION ';' | ASSIGNMENT ';' | EXPRESSION ';'
        ASTNode *stmt = parse_statement();
        if (!stmt)
            break;

        // Wrap statement in NODE_STATEMENT_LIST node
        ASTNode *stmt_list_node = create_node(NODE_STATEMENT_LIST, "STATEMENT_LIST", stmt, NULL);

        // Link Into the List
        if (!head)
            head = tail = stmt_list_node; // set as head and tail
        else
        {
            tail->right = stmt_list_node; // attach to the right of the previous tail
            tail = stmt_list_node;        // add statement list to list
        }
    }

    return head;
}

// === STATEMENT ===
// STATEMENT → DECLARATION ';' | ASSIGNMENT ';' | EXPRESSION ';'
ASTNode *parse_statement()
{
    TOKEN *tok = peek();
    if (!tok)
        return NULL;

    ASTNode *stmt_node = NULL;

    // determine of declaration
    if (tok->type == TOK_DATATYPE)
    {
        stmt_node = parse_declaration();
    }
    // determine if assignment
    else if (tok->type == TOK_IDENTIFIER)
    {
        // Lookahead to see if this is an assignment
        TOKEN *next = (current_token + 1 < token_count) ? &tokens[current_token + 1] : NULL;

        if (next && next->type == TOK_OPERATOR &&
            (strcmp(next->lexeme, "=") == 0 || strcmp(next->lexeme, "+=") == 0 ||
             strcmp(next->lexeme, "-=") == 0 || strcmp(next->lexeme, "*=") == 0 ||
             strcmp(next->lexeme, "/=") == 0))
        {
            stmt_node = parse_assignment();
        }
        else
        {
            stmt_node = parse_expression();
        }
    }
    // determine if expression only
    else
    {
        stmt_node = parse_expression();
    }

    if (!match(";"))
        error("Missing ';' after statement");

    return create_node(NODE_STATEMENT, "STATEMENT", stmt_node, NULL);
}

// === DECLARATION ===
/*
DECLARATION             → DATATYPE INIT_DECLARATOR_LIST
INIT_DECLARATOR_LIST    → INIT_DECLARATOR  | INIT_DECLARATOR ',' INIT_DECLARATOR_LIST
INIT_DECLARATOR         → IDENTIFIER  | IDENTIFIER '=' EXPRESSION
DATATYPE                → 'char' | 'int'
*/
ASTNode *parse_declaration()
{
    // DECLARATION → DATATYPE INIT_DECLARATOR_LIST
    //    DATATYPE → 'char' | 'int'
    TOKEN *datatype = consume();

    ASTNode *decl_list = NULL;
    ASTNode *last = NULL;

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
            // initializer may be an assignment expression
            rhs_node = parse_expression();
            initialized = 1;
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
// Implements: ASSIGN_EXPR -> IDENTIFIER ASSIGN_OP ASSIGN_EXPR | ADD_EXPR
ASTNode *parse_assignment()
{
    TOKEN *tok = peek();
    if (!tok)
        return NULL;

    // Save index to backtrack if this is not an assignment
    int save_index = current_token;

    if (tok->type == TOK_IDENTIFIER)
    {
        // Tentatively consume identifier
        TOKEN id_tok = *consume(); // copy token

        TOKEN *op = peek();
        if (op && op->type == TOK_OPERATOR &&
            (strcmp(op->lexeme, "=") == 0 || strcmp(op->lexeme, "+=") == 0 ||
             strcmp(op->lexeme, "-=") == 0 || strcmp(op->lexeme, "*=") == 0 ||
             strcmp(op->lexeme, "/=") == 0))
        {
            // It's an assignment operator: consume it
            TOKEN op_tok = *consume();

            ASTNode *lhs_node = create_node(NODE_FACTOR, id_tok.lexeme, NULL, NULL);

            // Right-hand side: try to parse another assignment (right-recursive)
            ASTNode *rhs_node = NULL;

            // If next token is identifier and could start another assignment, attempt recursive
            TOKEN *next = peek();
            if (next && next->type == TOK_IDENTIFIER)
            {
                rhs_node = parse_assignment();
                // If parse_assignment failed (returned NULL), fall back to additive
                if (!rhs_node)
                {
                    rhs_node = parse_additive();
                }
            }
            else
            {
                rhs_node = parse_additive();
            }

            return create_node(NODE_ASSIGNMENT, op_tok.lexeme, lhs_node, rhs_node);
        }
        else
        {
            // Not an assignment; backtrack and parse additive expression
            current_token = save_index;
            return parse_additive();
        }
    }

    // Not identifier: parse additive expression
    return parse_additive();
}

// === EXPRESSION ENTRY POINT ===
ASTNode *parse_expression()
{
    // Expression entry is assignment (per grammar)
    return parse_assignment();
}

// === ADDITIVE / TERM / FACTOR ===
ASTNode *parse_additive()
{
    ASTNode *node = parse_term();
    TOKEN *tok = peek();

    while (tok && tok->type == TOK_OPERATOR &&
           (strcmp(tok->lexeme, "+") == 0 || strcmp(tok->lexeme, "-") == 0))
    {
        TOKEN op = *consume();
        ASTNode *right = parse_term();
        node = create_node(NODE_EXPRESSION, op.lexeme, node, right);
        tok = peek();
    }
    return node;
}

ASTNode *parse_term()
{
    ASTNode *node = parse_factor();
    TOKEN *tok = peek();

    while (tok && tok->type == TOK_OPERATOR &&
           (strcmp(tok->lexeme, "*") == 0 || strcmp(tok->lexeme, "/") == 0))
    {
        TOKEN op = *consume();
        ASTNode *right = parse_factor();
        node = create_node(NODE_TERM, op.lexeme, node, right);
        tok = peek();
    }
    return node;
}

ASTNode *parse_factor()
{
    TOKEN *tok = peek();
    if (!tok)
        return NULL;

    // Unary prefix
    if (tok->type == TOK_OPERATOR &&
        (strcmp(tok->lexeme, "+") == 0 || strcmp(tok->lexeme, "-") == 0 ||
         strcmp(tok->lexeme, "++") == 0 || strcmp(tok->lexeme, "--") == 0))
    {
        TOKEN op_token = *consume();
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
    // Identifier or literal
    else if (tok->type == TOK_IDENTIFIER || tok->type == TOK_INT_LITERAL || tok->type == TOK_CHAR_LITERAL)
    {
        TOKEN literal = *consume();
        node = create_node(NODE_FACTOR, literal.lexeme, NULL, NULL);
    }
    else
    {
        error("Unexpected token in factor");
        consume();
        return NULL;
    }

    // Postfix (e.g., i++)
    tok = peek();
    while (tok && tok->type == TOK_OPERATOR &&
           (strcmp(tok->lexeme, "++") == 0 || strcmp(tok->lexeme, "--") == 0))
    {
        TOKEN op_token = *consume();
        node = create_node(NODE_POSTFIX_OP, op_token.lexeme, node, NULL);
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
        node->type == NODE_START ? "START" : node->type == NODE_STATEMENT_LIST ? "STMT_LIST"
                                         : node->type == NODE_STATEMENT        ? "STMT"
                                         : node->type == NODE_DECLARATION      ? "DECL"
                                         : node->type == NODE_ASSIGNMENT       ? "ASSIGN"
                                         : node->type == NODE_EXPRESSION       ? "EXPR"
                                         : node->type == NODE_TERM             ? "TERM"
                                         : node->type == NODE_UNARY_OP         ? "UNARY_OP"
                                         : node->type == NODE_POSTFIX_OP       ? "POSTFIX_OP"
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

// === ENTRY POINT ===
int syntax_analyzer()
{
    current_token = 0;
    syntax_error = 0;

    syntax_tree = parse_program();
    print_ast(syntax_tree, 0);

    if (syntax_error == 0 && current_token == token_count)
        printf("\nSyntax Accepted!\n");
    else
        printf("\nSyntax Rejected (Error found)\n");

    printf("===== SYNTAX ANALYSIS END =====\n\n");
    return syntax_error;
}
