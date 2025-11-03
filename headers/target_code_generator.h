#ifndef TARGET_CODE_GENERATOR_H
#define TARGET_CODE_GENERATOR_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "intermediate_code_generator.h"
#include "symbol_table.h"
#include <stdarg.h>

#define MAX_DATA_LENGTH 50
#define MAX_DATA 256
#define MAX_REGISTER_NAME_LENGTH 10
#define MAX_REGISTERS 30
#define MAX_TAC 256
#define MAX_ASSEMBLY_CODE 2048
#define MAX_ASSEMBLY_LINE 128

// 2D array for storage of Data section
typedef struct
{
    char data[MAX_DATA_LENGTH];
} Data;

typedef struct
{
    char name[MAX_REGISTER_NAME_LENGTH];
    int used;
    char assigned_temp[MAX_REGISTER_NAME_LENGTH];
} Register;

// Struct to hold the generated assembly output
typedef struct
{
    char assembly[MAX_ASSEMBLY_LINE];
} ASSEMBLY;

extern int assembly_code_count;
extern ASSEMBLY assembly_code[MAX_ASSEMBLY_CODE];

void initialize_registers();
void generate_target_code();

#endif
