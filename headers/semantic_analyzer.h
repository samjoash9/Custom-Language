#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "syntax_analyzer.h"
#include "symbol_table.h"

#define SEM_MAX_TEMPS 2048

/* semantic type (currently int/char/unknown; arithmetic treated as int) */
typedef enum
{
    SEM_TYPE_UNKNOWN = 0,
    SEM_TYPE_INT,
    SEM_TYPE_CHAR
} SEM_TYPE;

/* Temp container used by semantic analysis only */
typedef struct
{
    int id; /* unique id */
    SEM_TYPE type;
    int is_constant; /* 1 if compile-time-known */
    long int_value;  /* valid iff is_constant and numeric */
    ASTNode *node;   /* pointer to AST node (for diagnostics) */
} SEM_TEMP;

/* known compile-time variable value (semantic-only store) */
typedef struct KnownVar
{
    char *name;
    SEM_TEMP temp;
    struct KnownVar *next;
} KnownVar;

/* Public API */
int semantic_analyzer(void);    /* run semantic analysis on global syntax_tree */
int semantic_error_count(void); /* return number of semantic errors found */

#endif
