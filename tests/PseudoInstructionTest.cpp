#include <iostream>
#include <string>

#include "CPU.h"
#include "ALU.h"
#include "ControlUnit.h"
#include "DataMemory.h"
#include "Assembler.h"

using namespace std;

int passed = 0;
int failed = 0;

void check(const string& name, bool condition) {
    if (condition) {
        cout << "[PASS] " << name << '\n';
        passed++;
    }
    else {
        cout << "[FAIL] " << name << '\n';
        failed++;
    }
}

int main() {
    Memory mem;
    CPU cpu;
    ALU alu;
    ControlUnit control;
    Assembler assembler;

    bool assembled = assembler.assembleFile(
        "Assembly_Codes/PseudoInstructionTest.asm",
        mem
    );

    check(
        "Pseudo-instruction program assembled",
        assembled
    );

    if (!assembled) {
        return 1;
    }

    int instructionCount = 0;
    const int MAX_INSTRUCTIONS = 100;

    while (!cpu.halted &&
           instructionCount < MAX_INSTRUCTIONS) {

        cpu.executeInstruction(
            mem,
            control,
            alu
        );

        instructionCount++;
    }

    check(
        "Program halted",
        cpu.halted
    );

    check(
        "LI loaded 10 into $t0",
        cpu.registers[8] == 10
    );

    check(
        "LI loaded 20 into $t1",
        cpu.registers[9] == 20
    );

    check(
        "MOVE copied $t0 into $s0",
        cpu.registers[16] == 10
    );

    check(
        "ADD produced 30 in $t2",
        cpu.registers[10] == 30
    );

    check(
        "Instruction limit not reached",
        instructionCount < MAX_INSTRUCTIONS
    );

    cout << dec;

    cout << "\n============================\n";
    cout << "          SUMMARY\n";
    cout << "============================\n";

    cout << "Passed: " << passed << '\n';
    cout << "Failed: " << failed << '\n';
    cout << "Total:  " << passed + failed << '\n';

    if (failed == 0) {
        cout << "\nALL PSEUDO-INSTRUCTION TESTS PASSED!\n";
        return 0;
    }

    cout << "\nSOME PSEUDO-INSTRUCTION TESTS FAILED!\n";
    return 1;
}