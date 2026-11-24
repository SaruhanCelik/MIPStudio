#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include "Instruction.h"

struct Memory;
struct Instruction;
enum class instructionType;
struct ControlUnit;
struct ALU;


struct CPU{
    uint32_t registers[32];
    uint32_t pc;
    uint32_t hi;

    uint32_t lo;

    uint32_t ir;  // Instruction Register

    bool halted;

    public:
        CPU();

        void fetch(Memory& memory);
        void regWrite(int regIndex,uint32_t value);
        void printRegisterValue(int regIndex);
        uint8_t getOpcode();
        uint8_t getrs();
        uint8_t getrt();
        uint8_t getrd();
        uint8_t getshamt();
        uint8_t getfunct();
        uint16_t getimm();
        uint32_t getJaddr();
        void testgetimm();
        
        Instruction decode();
        instructionType decodeInstruction();
        void executeInstruction(Memory& memory, ControlUnit& controlUnit, ALU& alu);

        uint32_t signExtend16(uint16_t value);

};

#endif