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
    node->value = value ? strdup(value) : strdup("");
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
ASTNode *parse_assignment();

// === PROGRAM ===
ASTNode *parse_program()
{
    return create_node(NODE_START, "START", parse_statement_list(), NULL);
}

// === STATEMENT LIST ===
ASTNode *parse_statement_list()
{
    TOKEN *tok = peek();
    if (!tok)
        return NULL;

    ASTNode *stmt = parse_statement();
    if (!stmt)
        return NULL;

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
    else
    {
        stmt_node = parse_expression();
    }

    if (!match(";"))
        error("Missing ';' after statement");

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

        ASTNode *rhs_node = NULL;
        int initialized = 0;
        char literal_value[MAX_VALUE_LEN] = "";

        if (match("="))
        {
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
ASTNode *parse_assignment()
{
    TOKEN *id = peek();
    if (!id || id->type != TOK_IDENTIFIER)
    {
        error("Expected identifier in assignment");
        return NULL;
    }

    consume();
    ASTNode *lhs_node = create_node(NODE_FACTOR, id->lexeme, NULL, NULL);

    TOKEN *op = peek();
    if (!op || op->type != TOK_OPERATOR ||
        (strcmp(op->lexeme, "=") != 0 && strcmp(op->lexeme, "+=") != 0 &&
         strcmp(op->lexeme, "-=") != 0 && strcmp(op->lexeme, "*=") != 0 &&
         strcmp(op->lexeme, "/=") != 0))
    {
        error("Expected assignment operator");
        return lhs_node;
    }

    consume(); // consume operator

    ASTNode *rhs_node = NULL;
    // Support chained '=' assignments
    if (strcmp(op->lexeme, "=") == 0 && peek() && peek()->type == TOK_IDENTIFIER)
    {
        TOKEN *next_op = (current_token + 1 < token_count) ? &tokens[current_token + 1] : NULL;
        if (next_op && next_op->type == TOK_OPERATOR && strcmp(next_op->lexeme, "=") == 0)
        {
            rhs_node = parse_assignment(); // recursive chaining
        }
        else
        {
            rhs_node = parse_expression();
        }
    }
    else
    {
        rhs_node = parse_expression();
    }

    return create_node(NODE_ASSIGNMENT, op->lexeme, lhs_node, rhs_node);
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

    // Unary prefix
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
    // Identifier or literal
    else if (tok->type == TOK_IDENTIFIER || tok->type == TOK_INT_LITERAL || tok->type == TOK_CHAR_LITERAL)
    {
        consume();
        node = create_node(NODE_FACTOR, tok->lexeme, NULL, NULL);
    }
    else
    {
        error("Unexpected token in factor");
        consume();
        return NULL;
    }

    // Postfix
    tok = peek();
    while (tok && tok->type == TOK_OPERATOR &&
           (strcmp(tok->lexeme, "++") == 0 || strcmp(tok->lexeme, "--") == 0))
    {
        TOKEN op_token = *tok;
        consume();
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
