#ifndef EMULATOR_SESSION_H
#define EMULATOR_SESSION_H

#include <cstdint>
#include <set>
#include <string>

#include "CPU.h"
#include "ALU.h"
#include "ControlUnit.h"
#include "DataMemory.h"
#include "Assembler.h"


// ============================================================
// EmulatorSession
// ------------------------------------------------------------
// UI/CLI-facing wrapper around the emulator core.
//
// CPU, Memory, Assembler, etc. remain completely independent
// from Qt.
//
// Breakpoints live here because they are execution-session
// state, not CPU architecture state.
// ============================================================

class EmulatorSession {
public:

    struct RunResult {
        int instructionsExecuted = 0;

        bool halted = false;

        bool breakpointHit = false;

        bool instructionLimitReached = false;

        uint32_t stopAddress = 0;
    };


    EmulatorSession();


    // ========================================================
    // PROGRAM MANAGEMENT
    // ========================================================

    bool loadProgram(
        const std::string& filename
    );

    bool assembleProgram(
        const std::string& filename
    );

    bool reset();


    // ========================================================
    // EXECUTION
    // ========================================================

    bool step();

    RunResult run(
        int maxInstructions = 100000
    );


    // ========================================================
    // BREAKPOINTS
    // ========================================================

    void addBreakpoint(
        uint32_t address
    );

    void removeBreakpoint(
        uint32_t address
    );

    void clearBreakpoints();

    bool hasBreakpoint(
        uint32_t address
    ) const;

    const std::set<uint32_t>&
    getBreakpoints() const;


    // ========================================================
    // CORE ACCESS
    // ========================================================

    CPU& getCPU();

    const CPU& getCPU() const;


    Memory& getMemory();

    const Memory& getMemory() const;


    Assembler& getAssembler();

    const Assembler& getAssembler() const;


    // ========================================================
    // PROGRAM INFORMATION
    // ========================================================

    const std::string&
    getProgramFile() const;

    bool hasProgram() const;


private:

    CPU cpu;
    Memory memory;
    ALU alu;
    ControlUnit control;
    Assembler assembler;

    std::string programFile;

    bool programLoaded;


    // Breakpoint addresses in machine-code space.
    std::set<uint32_t> breakpoints;


    // True when Run() previously stopped BEFORE executing
    // the current breakpoint instruction.
    //
    // Calling Run() again will execute that instruction once
    // and continue instead of getting stuck forever.
    bool pausedOnBreakpoint;


    void resetCPUState();
};

#endif