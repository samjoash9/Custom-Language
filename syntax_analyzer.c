/* ======================================================================
   syntax_analyzer.c  (Corrected to perfectly follow your grammar)
   ====================================================================== */

#include "headers/syntax_analyzer.h"

extern TOKEN tokens[];
extern int token_count;

int current_token = 0;
int syntax_error = 0;

ASTNode *syntax_tree = NULL;

/* --------------------------------------------------------------
   TOKEN HELPERS
   -------------------------------------------------------------- */

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

int match_lex(const char *lex)
{
    TOKEN *t = peek();
    if (t && strcmp(t->lexeme, lex) == 0)
    {
        consume();
        return 1;
    }
    return 0;
}

int match_type(int type)
{
    TOKEN *t = peek();
    if (t && t->type == type)
    {
        consume();
        return 1;
    }
    return 0;
}

void error(const char *msg)
{
    if (!syntax_error)
    {
        TOKEN *t = peek();
        printf("Syntax Error: %s (near '%s')\n",
               msg, t ? t->lexeme : "EOF");
    }
    syntax_error = 1;
}

void print_indent(int level)
{
    for (int i = 0; i < level; i++)
        printf("  ");
}

void print_ast(ASTNode *node, int level)
{
    if (!node)
        return;

    print_indent(level);
    printf("(%d) %s\n", node->type, node->value ? node->value : "NULL");

    if (node->left)
        print_ast(node->left, level + 1);

    if (node->right)
        print_ast(node->right, level + 1);
}

/* --------------------------------------------------------------
   AST NODE CREATION
   -------------------------------------------------------------- */

ASTNode *new_node(NodeType type, const char *val, ASTNode *l, ASTNode *r)
{
    ASTNode *n = malloc(sizeof(ASTNode));
    n->type = type;
    n->left = l;
    n->right = r;
    n->value = strdup(val ? val : "");
    return n;
}

/* --------------------------------------------------------------
   FORWARD DECLARATIONS
   -------------------------------------------------------------- */

ASTNode *parse_program();
ASTNode *parse_statement_list();
ASTNode *parse_statement();
ASTNode *parse_printing();
ASTNode *parse_print_list();
ASTNode *parse_print_list_prime(ASTNode *left_head);
ASTNode *parse_print_item();

ASTNode *parse_declaration();
char *parse_declarator();
ASTNode *parse_init_declarator_list();

ASTNode *parse_assignment();
ASTNode *parse_simple_expr();
ASTNode *parse_add_expr();
ASTNode *parse_term();
ASTNode *parse_factor();
ASTNode *parse_unary();
ASTNode *parse_postfix();
ASTNode *parse_primary();

/* --------------------------------------------------------------
   <program> ::= <statement_list>
   -------------------------------------------------------------- */

ASTNode *parse_program()
{
    return new_node(NODE_START, "START", parse_statement_list(), NULL);
}

/* --------------------------------------------------------------
   <statement_list> ::= <statement> <statement_list> | ε
   -------------------------------------------------------------- */

int is_statement_start(int type)
{
    return (type == ENTEGER || type == CHAROT || type == KUAN ||
            type == PRENT ||
            type == IDENTIFIER ||
            type == LPAREN ||
            type == EXCLAM ||
            type == INT_LITERAL || type == CHAR_LITERAL);
}

ASTNode *parse_statement_list()
{
    TOKEN *t = peek();
    if (!t || !is_statement_start(t->type))
        return NULL; // epsilon

    ASTNode *st = parse_statement();
    ASTNode *rest = parse_statement_list(); // recursion OK (no infinite loop because t must change)

    return new_node(NODE_STATEMENT_LIST, "STMT_LIST", st, rest);
}

/* --------------------------------------------------------------
   <statement> ::= <declaration> "!"
                 | <assignment> "!"
                 | <simple_expr> "!"
                 | <printing> "!"
                 | "!"
   -------------------------------------------------------------- */

ASTNode *parse_statement()
{
    TOKEN *t = peek();
    if (!t)
        return NULL;

    ASTNode *node = NULL;

    if (t->type == ENTEGER || t->type == CHAROT || t->type == KUAN)
    {
        node = parse_declaration();
    }
    else if (t->type == PRENT)
    {
        node = parse_printing();
    }
    else if (t->type == IDENTIFIER)
    {
        /* Check ahead for assign_op */
        TOKEN *nxt = (current_token + 1 < token_count)
                         ? &tokens[current_token + 1]
                         : NULL;

        if (nxt && (nxt->type == EQUAL || nxt->type == PLUS_EQUAL ||
                    nxt->type == MINUS_EQUAL || nxt->type == MUL_EQUAL ||
                    nxt->type == DIV_EQUAL))
            node = parse_assignment();
        else
            node = parse_simple_expr();
    }
    else if (t->type == EXCLAM)
    {
        consume();
        return new_node(NODE_STATEMENT, "EMPTY!", NULL, NULL);
    }
    else
    {
        node = parse_simple_expr();
    }

    if (!match_lex("!"))
        error("Missing '!' after statement");

    return new_node(NODE_STATEMENT, "STATEMENT", node, NULL);
}

/* --------------------------------------------------------------
   PRINTING
   -------------------------------------------------------------- */

ASTNode *parse_printing()
{
    if (!match_type(PRENT))
    {
        error("Expected PRENT");
        return NULL;
    }

    ASTNode *list = parse_print_list();
    return new_node(NODE_PRINTING, "PRINT", list, NULL);
}

ASTNode *parse_print_list()
{
    TOKEN *t = peek();
    if (!t || t->type == EXCLAM)
        return NULL; // ε

    ASTNode *item = parse_print_item();
    return parse_print_list_prime(item);
}

ASTNode *parse_print_list_prime(ASTNode *left_head)
{
    if (match_lex(","))
    {
        ASTNode *itm = parse_print_item();
        ASTNode *rest = parse_print_list_prime(itm);
        return new_node(NODE_PRINT_ITEM, "PRINT_ITEM", left_head, rest);
    }
    return left_head; // ε
}

ASTNode *parse_print_item()
{
    TOKEN *t = peek();
    if (t->type == STRING_LITERAL)
    {
        TOKEN lit = *consume();
        return new_node(NODE_PRINT_ITEM, lit.lexeme, NULL, NULL);
    }
    return parse_simple_expr();
}

/* --------------------------------------------------------------
   DECLARATION
   -------------------------------------------------------------- */

ASTNode *parse_declaration()
{
    TOKEN *dt = consume();
    if (!dt)
    {
        error("Expected datatype");
        return NULL;
    }

    ASTNode *decls = parse_init_declarator_list();
    return new_node(NODE_DECLARATION, dt->lexeme, decls, NULL);
}

ASTNode *parse_init_declarator_list()
{
    char *name = parse_declarator();
    if (!name)
        return NULL;

    ASTNode *rhs = NULL;

    if (match_lex("="))
    {
        rhs = parse_simple_expr();
    }

    ASTNode *decl = new_node(NODE_DECLARATION, name, rhs, NULL);
    free(name);

    if (match_lex(","))
    {
        ASTNode *rest = parse_init_declarator_list();
        return new_node(NODE_DECLARATION, "DECL", decl, rest);
    }

    return decl;
}

char *parse_declarator()
{
    TOKEN *t = peek();
    if (!t)
        return NULL;

    if (match_type(IDENTIFIER))
    {
        return strdup(tokens[current_token - 1].lexeme);
    }

    if (match_lex("("))
    {
        char *inner = parse_declarator();
        if (!match_lex(")"))
            error("Missing ')'");
        return inner;
    }

    error("Invalid declarator");
    return NULL;
}

/* --------------------------------------------------------------
   ASSIGNMENT
   -------------------------------------------------------------- */

ASTNode *parse_assignment()
{
    if (!match_type(IDENTIFIER))
        return NULL;
    TOKEN idt = tokens[current_token - 1];

    TOKEN *op = peek();
    if (!op ||
        !(op->type == EQUAL || op->type == PLUS_EQUAL ||
          op->type == MINUS_EQUAL || op->type == MUL_EQUAL ||
          op->type == DIV_EQUAL))
    {
        error("Expected assignment operator");
        return NULL;
    }

    TOKEN opt = *consume();

    /* two productions:
         id op assignment
         id op simple_expr
    */

    TOKEN *nxt = peek();
    ASTNode *rhs;

    if (nxt && nxt->type == IDENTIFIER)
    {
        /* look ahead further to distinguish assign vs expr */
        TOKEN *nxt2 = (current_token + 1 < token_count)
                          ? &tokens[current_token + 1]
                          : NULL;

        if (nxt2 && (nxt2->type == EQUAL || nxt2->type == PLUS_EQUAL ||
                     nxt2->type == MINUS_EQUAL || nxt2->type == MUL_EQUAL ||
                     nxt2->type == DIV_EQUAL))
        {
            rhs = parse_assignment();
        }
        else
        {
            rhs = parse_simple_expr();
        }
    }
    else
    {
        rhs = parse_simple_expr();
    }

    ASTNode *lhs = new_node(NODE_FACTOR, idt.lexeme, NULL, NULL);
    return new_node(NODE_ASSIGNMENT, opt.lexeme, lhs, rhs);
}

/* --------------------------------------------------------------
   SIMPLE EXPR
   -------------------------------------------------------------- */

ASTNode *parse_simple_expr()
{
    return parse_add_expr();
}

/* --------------------------------------------------------------
   ADD_EXPR (no left recursion)
   -------------------------------------------------------------- */

ASTNode *parse_add_expr()
{
    ASTNode *node = parse_term();

    while (1)
    {
        TOKEN *t = peek();
        if (!t)
            break;
        if (t->type == PLUS || t->type == MINUS)
        {
            TOKEN op = *consume();
            ASTNode *right = parse_term();
            node = new_node(NODE_EXPRESSION, op.lexeme, node, right);
        }
        else
            break;
    }
    return node;
}

/* --------------------------------------------------------------
   TERM
   -------------------------------------------------------------- */

ASTNode *parse_term()
{
    ASTNode *node = parse_factor();

    while (1)
    {
        TOKEN *t = peek();
        if (!t)
            break;
        if (t->type == MUL || t->type == DIV)
        {
            TOKEN op = *consume();
            ASTNode *right = parse_factor();
            node = new_node(NODE_TERM, op.lexeme, node, right);
        }
        else
            break;
    }
    return node;
}

/* --------------------------------------------------------------
   FACTOR
   -------------------------------------------------------------- */

ASTNode *parse_factor()
{
    TOKEN *t = peek();
    if (!t)
        return NULL;

    /* unary */
    if (t->type == PLUS || t->type == MINUS)
        return parse_unary();

    if (t->type == PLUSPLUS || t->type == MINUSMINUS)
        return parse_unary();

    /* postfix */
    return parse_postfix();
}

/* --------------------------------------------------------------
   UNARY
   -------------------------------------------------------------- */

ASTNode *parse_unary()
{
    TOKEN *t = consume();

    ASTNode *right = NULL;

    if (t->type == PLUS || t->type == MINUS)
    {
        right = parse_factor();
        return new_node(NODE_UNARY_OP, t->lexeme, right, NULL);
    }

    if (t->type == PLUSPLUS || t->type == MINUSMINUS)
    {
        ASTNode *post = parse_postfix();
        return new_node(NODE_UNARY_OP, t->lexeme, post, NULL);
    }

    error("Invalid unary operator");
    return NULL;
}

/* --------------------------------------------------------------
   POSTFIX
   -------------------------------------------------------------- */

ASTNode *parse_postfix()
{
    ASTNode *p = parse_primary();
    if (!p)
        return NULL;

    TOKEN *t = peek();
    if (t && (t->type == PLUSPLUS || t->type == MINUSMINUS))
    {
        TOKEN op = *consume();
        return new_node(NODE_POSTFIX_OP, op.lexeme, p, NULL);
    }
    return p;
}

/* --------------------------------------------------------------
   PRIMARY
   -------------------------------------------------------------- */

ASTNode *parse_primary()
{
    TOKEN *t = peek();
    if (!t)
        return NULL;

    if (match_lex("("))
    {
        ASTNode *x = parse_simple_expr();
        if (!match_lex(")"))
            error("Missing ')'");
        return x;
    }

    if (match_type(IDENTIFIER) ||
        match_type(INT_LITERAL) ||
        match_type(CHAR_LITERAL))
    {
        TOKEN lit = tokens[current_token - 1];
        return new_node(NODE_FACTOR, lit.lexeme, NULL, NULL);
    }

    error("Invalid primary");
    consume();
    return NULL;
}

/* --------------------------------------------------------------
   ENTRY POINT
   -------------------------------------------------------------- */

int syntax_analyzer()
{
    current_token = 0;
    syntax_error = 0;

    syntax_tree = parse_program();

    print_ast(syntax_tree, 0);

    if (!syntax_error && current_token == token_count)
        printf("\nSyntax Accepted!\n");
    else
        printf("\nSyntax Rejected.\n");

    return syntax_error;
}
