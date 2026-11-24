#ifndef CONTROLUNIT_H
#define CONTROLUNIT_H

#include "Instruction.h"
#include "ALU.h"

struct ControlSignals{
    bool RegDst;
    bool Branch;
    bool MemRead;
    bool MemtoReg;
    bool MemWrite;
    bool ALUSrc;
    bool RegWrite;

    uint8_t ALUOp;
};

struct ControlUnit {

    ControlSignals generateSignals(instructionType instruction);

    ALUOperation getALUOperation(instructionType instruction);
};

#endif