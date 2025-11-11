#include "headers/syntax_analyzer.h"

// === GLOBALS FROM LEXICAL ANALYZER ===
int current_token = 0;
int syntax_error = 0;
ASTNode *syntax_tree = NULL;

// === UTILITY FUNCTIONS ===
// Looks at the current token without consuming it.
TOKEN *peek()
{
    if (current_token < token_count)
        return &tokens[current_token];

    return NULL;
}

// Moves to the next token (consumes one).
TOKEN *consume()
{
    if (current_token < token_count)
        return &tokens[current_token++];

    return NULL;
}

// If current token matches the given string, consume it and return 1; otherwise 0.
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

// Prints a syntax error and sets syntax_error = 1.
void error(const char *message)
{
    if (!syntax_error) // avoid spamming same error
    {
        printf("Syntax Error: %s (near token '%s')\n",
               message,
               current_token < token_count ? tokens[current_token].lexeme : "EOF");
    }
    syntax_error = 1;
}

// === AST CREATION ===
ASTNode *create_node(NodeType type, const char *value, ASTNode *left, ASTNode *right)
{
    if (syntax_error)
        return NULL; // stop creating nodes if syntax already failed

    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
    {
        error("Memory allocation failed while creating AST node");
        return NULL;
    }

    node->type = type;
    node->left = left;
    node->right = right;

    if (value)
    {
        node->value = strdup(value);
        if (!node->value)
        {
            error("Memory allocation failed while duplicating node value");
            free(node);
            return NULL;
        }
    }
    else
    {
        node->value = strdup("");
        if (!node->value)
        {
            error("Memory allocation failed while duplicating empty node value");
            free(node);
            return NULL;
        }
    }

    return node;
}

// === PROGRAM ===
// S → STATEMENT_LIST
ASTNode *parse_program()
{
    if (syntax_error)
        return NULL;
    return create_node(NODE_START, "START", parse_statement_list(), NULL);
}

// === STATEMENT LIST ===
// STATEMENT_LIST → STATEMENT STATEMENT_LIST | ε
ASTNode *parse_statement_list()
{
    ASTNode *head = NULL;
    ASTNode *tail = NULL;

    while (!syntax_error)
    {
        TOKEN *tok = peek();
        if (!tok)
            break;

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

        ASTNode *stmt = parse_statement();
        if (!stmt)
            break;

        ASTNode *stmt_list_node = create_node(NODE_STATEMENT_LIST, "STATEMENT_LIST", stmt, NULL);
        if (!stmt_list_node)
            return head;

        if (!head)
            head = tail = stmt_list_node;
        else
        {
            tail->right = stmt_list_node;
            tail = stmt_list_node;
        }
    }

    return head;
}

// === STATEMENT ===
ASTNode *parse_statement()
{
    if (syntax_error)
        return NULL;

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
char *parse_declarator()
{
    TOKEN *tok = peek();
    if (!tok)
    {
        error("Unexpected end of declarator");
        return NULL;
    }

    if (tok->type == TOK_IDENTIFIER)
    {
        char *name = strdup(tok->lexeme);
        consume();
        return name;
    }

    if (tok->type == TOK_PARENTHESIS && strcmp(tok->lexeme, "(") == 0)
    {
        consume();
        char *inner = parse_declarator();
        if (!match(")"))
            error("Missing ')' in declarator");
        return inner;
    }

    error("Expected identifier or '(' in declarator");
    return NULL;
}

ASTNode *parse_declaration()
{
    TOKEN *datatype = consume();
    ASTNode *decl_list = NULL;
    ASTNode *last = NULL;

    while (!syntax_error)
    {
        TOKEN *tok = peek();
        if (!tok)
        {
            error("Unexpected end of declaration");
            break;
        }

        char *identifier_name = parse_declarator();
        if (!identifier_name)
            break;

        ASTNode *rhs_node = NULL;
        int initialized = 0;
        char literal_value[MAX_VALUE_LEN] = "";

        if (match("="))
        {
            rhs_node = parse_expression();
            initialized = 1;
        }

        int as_res = add_symbol(identifier_name, datatype->lexeme, literal_value, initialized);

        if (as_res == 0)
        {
            syntax_error = 1;
            free(identifier_name);
            break;
        }

        ASTNode *decl_node = create_node(NODE_DECLARATION, identifier_name, rhs_node, NULL);
        if (!decl_list)
            decl_list = decl_node;
        else
            last->right = decl_node;

        last = decl_node;

        free(identifier_name);

        if (!match(","))
            break;
    }

    return create_node(NODE_DECLARATION, datatype->lexeme, decl_list, NULL);
}

// === ASSIGNMENT ===
ASTNode *parse_assignment()
{
    TOKEN *tok = peek();
    if (!tok)
        return NULL;

    int save_index = current_token;

    if (tok->type == TOK_IDENTIFIER)
    {
        TOKEN id_tok = *consume();
        TOKEN *op = peek();

        if (op && op->type == TOK_OPERATOR &&
            (strcmp(op->lexeme, "=") == 0 || strcmp(op->lexeme, "+=") == 0 ||
             strcmp(op->lexeme, "-=") == 0 || strcmp(op->lexeme, "*=") == 0 ||
             strcmp(op->lexeme, "/=") == 0))
        {
            TOKEN op_tok = *consume();
            ASTNode *lhs_node = create_node(NODE_FACTOR, id_tok.lexeme, NULL, NULL);
            ASTNode *rhs_node = NULL;

            TOKEN *next = peek();
            if (next && next->type == TOK_IDENTIFIER)
            {
                rhs_node = parse_assignment();
                if (!rhs_node)
                    rhs_node = parse_additive();
            }
            else
            {
                rhs_node = parse_additive();
            }

            return create_node(NODE_ASSIGNMENT, op_tok.lexeme, lhs_node, rhs_node);
        }
        else
        {
            current_token = save_index;
            return parse_additive();
        }
    }

    return parse_additive();
}

// === EXPRESSIONS ===
ASTNode *parse_expression() { return parse_assignment(); }

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

    if (tok->type == TOK_OPERATOR &&
        (strcmp(tok->lexeme, "+") == 0 || strcmp(tok->lexeme, "-") == 0 ||
         strcmp(tok->lexeme, "++") == 0 || strcmp(tok->lexeme, "--") == 0))
    {
        TOKEN op_token = *consume();
        ASTNode *factor_node = parse_factor();
        return create_node(NODE_UNARY_OP, op_token.lexeme, factor_node, NULL);
    }

    ASTNode *node = NULL;

    if (tok->type == TOK_PARENTHESIS && strcmp(tok->lexeme, "(") == 0)
    {
        consume();
        node = parse_expression();
        if (!match(")"))
            error("Missing ')'");
    }
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

// === AST PRINTER & CLEANUP ===
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
