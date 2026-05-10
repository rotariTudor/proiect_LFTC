#pragma once

typedef struct Instr{
    int opcode;
    struct Instr *next;
}Instr;