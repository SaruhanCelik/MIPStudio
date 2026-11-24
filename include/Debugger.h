#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <cstdint>
#include <set>
#include <string>

struct CPU;
struct Memory;
struct ALU;
struct ControlUnit;
struct Assembler;

class Debugger {
public:
    Debugger(
        CPU& cpu,
        Memory& mem,
        ALU& alu,
        ControlUnit& control,
        Assembler& assembler,
        const std::string& programFile
    );

    void start();

private:
    CPU& cpu;
    Memory& mem;
    ALU& alu;
    ControlUnit& control;
    Assembler& assembler;

    std::string programFile;

    std::set<uint32_t> breakpoints;

    void step();
    void run();
    void reset();

    void printRegisters();

    void printMemory(
        uint32_t address,
        uint32_t count
    );

    void addBreakpoint(uint32_t address);
    void removeBreakpoint(uint32_t address);
    void printBreakpoints();

    void printHelp();

    std::string disassemble(
        uint32_t instruction,
        uint32_t address
    );

    void disassembleMemory(
        uint32_t address,
        uint32_t count
    );

    bool parseAddress(
        const std::string& text,
        uint32_t& address
    );
};

#endif