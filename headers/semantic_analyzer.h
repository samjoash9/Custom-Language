#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "syntax_analyzer.h"
#include "symbol_table.h"

#define SEM_MAX_TEMPS 2048
typedef enum
{
    SEM_TYPE_UNKNOWN = 0,
    SEM_TYPE_INT,
    SEM_TYPE_CHAR
} SEM_TYPE;

typedef struct
{
    int id;
    SEM_TYPE type;
    int is_constant;
    long int_value;
    ASTNode *node;
} SEM_TEMP;

typedef struct KnownVar
{
    char *name;
    SEM_TEMP temp;
    int initialized;
    int used;
    struct KnownVar *next;
} KnownVar;

/* Public API */
int semantic_analyzer(void);
int semantic_error_count(void);

#endif
