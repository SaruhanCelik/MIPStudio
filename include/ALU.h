#ifndef ALU_H
#define ALU_H

#include <cstdint>

enum class ALUOperation {
    ADD,
    ADDU,

    SUB,
    SUBU,

    AND,
    OR,
    XOR,
    NOR,

    SLT,
    SLTU,

    SLL,
    SRL,
    SRA,

    MUL,

    NONE
};

struct ALUResult {
    uint32_t value;
    bool zero;
    bool overflow;
};

struct ALU {
    ALUResult execute(
        uint32_t op1,
        uint32_t op2,
        ALUOperation operation
    );
};

#endif