#include "EmulatorSession.h"

using namespace std;


// ============================================================
// CONSTRUCTOR
// ============================================================

EmulatorSession::EmulatorSession()
    : programLoaded(false),
      pausedOnBreakpoint(false)
{
}


// ============================================================
// RESET CPU STATE
// ============================================================

void EmulatorSession::resetCPUState() {

    for (int i = 0; i < 32; i++) {
        cpu.registers[i] = 0;
    }

    cpu.pc = 0;

    cpu.hi = 0;

    cpu.lo = 0;

    cpu.halted = false;

    pausedOnBreakpoint = false;
}


// ============================================================
// LOAD PROGRAM
// ============================================================

bool EmulatorSession::loadProgram(
    const string& filename
) {

    return assembleProgram(
        filename
    );
}


// ============================================================
// ASSEMBLE PROGRAM
// ============================================================

bool EmulatorSession::assembleProgram(
    const string& filename
) {

    resetCPUState();


    if (!assembler.assembleFile(
            filename,
            memory
        )) {

        programLoaded = false;

        return false;
    }


    programFile =
        filename;


    programLoaded =
        true;


    return true;
}


// ============================================================
// RESET PROGRAM
// ============================================================

bool EmulatorSession::reset() {

    if (!programLoaded) {
        return false;
    }


    resetCPUState();


    if (!assembler.assembleFile(
            programFile,
            memory
        )) {

        return false;
    }


    return true;
}


// ============================================================
// STEP
// ============================================================

bool EmulatorSession::step() {

    if (!programLoaded) {
        return false;
    }


    if (cpu.halted) {
        return false;
    }


    // A manual step should always be allowed even when the
    // current instruction has a breakpoint.
    pausedOnBreakpoint = false;


    cpu.executeInstruction(
        memory,
        control,
        alu
    );


    return true;
}


// ============================================================
// RUN
// ============================================================

EmulatorSession::RunResult
EmulatorSession::run(
    int maxInstructions
) {

    RunResult result;


    if (!programLoaded) {
        return result;
    }


    // --------------------------------------------------------
    // Continue-from-breakpoint behavior
    // --------------------------------------------------------
    //
    // If Run previously stopped AT this breakpoint, pressing
    // Run again should execute that instruction once rather
    // than stopping at it forever.
    // --------------------------------------------------------

    if (
        pausedOnBreakpoint &&
        !cpu.halted
    ) {

        cpu.executeInstruction(
            memory,
            control,
            alu
        );


        result.instructionsExecuted++;

        pausedOnBreakpoint = false;
    }


    // --------------------------------------------------------
    // Normal execution
    // --------------------------------------------------------

    while (
        !cpu.halted &&
        result.instructionsExecuted
            < maxInstructions
    ) {

        // Stop BEFORE the breakpoint instruction executes.
        if (
            breakpoints.find(cpu.pc)
            != breakpoints.end()
        ) {

            result.breakpointHit =
                true;


            result.stopAddress =
                cpu.pc;


            pausedOnBreakpoint =
                true;


            break;
        }


        cpu.executeInstruction(
            memory,
            control,
            alu
        );


        result.instructionsExecuted++;
    }


    result.halted =
        cpu.halted;


    if (
        !cpu.halted &&
        !result.breakpointHit &&
        result.instructionsExecuted
            >= maxInstructions
    ) {

        result.instructionLimitReached =
            true;
    }


    if (!result.breakpointHit) {

        result.stopAddress =
            cpu.pc;
    }


    return result;
}


// ============================================================
// BREAKPOINTS
// ============================================================

void EmulatorSession::addBreakpoint(
    uint32_t address
) {

    breakpoints.insert(
        address
    );
}


void EmulatorSession::removeBreakpoint(
    uint32_t address
) {

    breakpoints.erase(
        address
    );
}


void EmulatorSession::clearBreakpoints() {

    breakpoints.clear();

    pausedOnBreakpoint = false;
}


bool EmulatorSession::hasBreakpoint(
    uint32_t address
) const {

    return
        breakpoints.find(address)
        != breakpoints.end();
}


const set<uint32_t>&
EmulatorSession::getBreakpoints() const {

    return breakpoints;
}


// ============================================================
// ACCESSORS
// ============================================================

CPU& EmulatorSession::getCPU() {
    return cpu;
}


const CPU&
EmulatorSession::getCPU() const {
    return cpu;
}


Memory&
EmulatorSession::getMemory() {
    return memory;
}


const Memory&
EmulatorSession::getMemory() const {
    return memory;
}


Assembler&
EmulatorSession::getAssembler() {
    return assembler;
}


const Assembler&
EmulatorSession::getAssembler() const {
    return assembler;
}


const string&
EmulatorSession::getProgramFile() const {

    return programFile;
}


bool EmulatorSession::hasProgram() const {

    return programLoaded;
}