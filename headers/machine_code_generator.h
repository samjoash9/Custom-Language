#ifndef MACHINE_CODE_GENERATOR_H
#define MACHINE_CODE_GENERATOR_H
#define MAX_MACHINE_CODE 1024

#include <stdio.h>
#include <string.h>
#include "target_code_generator.h"

typedef struct{
    char code[33]; // 32 bits + null terminator
} MACHINE; 

// Function prototype
void generate_machine_code(void);

#endif // MACHINE_CODE_GENERERATOR_H
