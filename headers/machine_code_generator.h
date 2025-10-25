#ifndef MACHINE_CODE_GENERATOR_H
#define MACHINE_CODE_GENERATOR_H
#define MAX_MACHINE_CODE 1024

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "target_code_generator.h"

#define R_TYPE_COUNT 5
#define I_TYPE_COUNT 3

typedef struct
{
    char code[33]; // 32 bits + null terminator
} MACHINE;

// Function prototype
void generate_machine_code(void);

#endif // MACHINE_CODE_GENERERATOR_H
