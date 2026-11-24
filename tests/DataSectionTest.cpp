#include <iostream>
#include <iomanip>
#include <string>

#include "CPU.h"
#include "ALU.h"
#include "ControlUnit.h"
#include "DataMemory.h"
#include "Assembler.h"

using namespace std;

int passed = 0;
int failed = 0;

void check(
    const string& name,
    bool condition
) {
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

    cout << "====================================\n";
    cout << "       DATA SECTION TEST SUITE\n";
    cout << "====================================\n\n";


    // =========================================
    // ASSEMBLE
    // =========================================

    bool assembled =
        assembler.assembleFile(
            "Assembly_Codes/DataSectionTest.asm",
            mem
        );

    check(
        "Assembly completed successfully",
        assembled
    );

    if (!assembled) {
        cout << "\nAssembly failed.\n";
        return 1;
    }


    // =========================================
    // DATA MEMORY CHECKS
    // =========================================

    check(
        ".word 42 stored at 0x1000",
        mem.memRead32(0x1000) == 42
    );

    check(
        ".word -100 stored correctly at 0x1004",
        mem.memRead32(0x1004) == 0xFFFFFF9C
    );

    check(
        ".word 0x12345678 stored at 0x1008",
        mem.memRead32(0x1008) == 0x12345678
    );


    // =========================================
    // TEXT MEMORY CHECKS
    // =========================================

    /*
        addi $t0, $zero, 10

        opcode 8
        rt = 8
        imm = 10

        machine code:
        0x2008000A
    */

    check(
        ".text instruction begins at address 0",
        mem.memRead32(0x0000) == 0x2008000A
    );

    /*
        addi $v0, $zero, 10
        machine code = 0x2002000A
    */

    check(
        "Exit setup instruction stored at 0x0004",
        mem.memRead32(0x0004) == 0x2002000A
    );

    /*
        syscall
        machine code = 0x0000000C
    */

    check(
        "SYSCALL stored at 0x0008",
        mem.memRead32(0x0008) == 0x0000000C
    );


    // =========================================
    // VERIFY TEXT/DATA DO NOT OVERLAP
    // =========================================

    check(
        "Text and data regions are separate",
        mem.memRead32(0x1000) !=
        mem.memRead32(0x0000)
    );


    // =========================================
    // EXECUTE TEXT PROGRAM
    // =========================================

    const int MAX_INSTRUCTIONS = 100;

    int instructionCount = 0;

    while (
        !cpu.halted &&
        instructionCount < MAX_INSTRUCTIONS
    ) {

        cpu.executeInstruction(
            mem,
            control,
            alu
        );

        instructionCount++;
    }


    // =========================================
    // CPU RESULT CHECKS
    // =========================================

    check(
        "Program halted",
        cpu.halted
    );

    check(
        "Instruction limit not reached",
        instructionCount < MAX_INSTRUCTIONS
    );

    check(
        "$t0 contains 10",
        cpu.registers[8] == 10
    );

    check(
        "$v0 contains exit syscall code 10",
        cpu.registers[2] == 10
    );


    // =========================================
    // DATA MUST SURVIVE CPU EXECUTION
    // =========================================

    check(
        "Data value 42 survives execution",
        mem.memRead32(0x1000) == 42
    );

    check(
        "Data value -100 survives execution",
        mem.memRead32(0x1004) == 0xFFFFFF9C
    );

    check(
        "Large data value survives execution",
        mem.memRead32(0x1008) == 0x12345678
    );


    // =========================================
    // DISPLAY MEMORY
    // =========================================

    cout << "\n--- TEXT MEMORY ---\n";

    cout << hex
         << uppercase
         << setfill('0');

    for (
        uint32_t address = 0;
        address <= 0x0008;
        address += 4
    ) {

        cout
            << "0x"
            << setw(4)
            << address
            << " -> 0x"
            << setw(8)
            << mem.memRead32(address)
            << '\n';
    }


    cout << "\n--- DATA MEMORY ---\n";

    for (
        uint32_t address = 0x1000;
        address <= 0x1008;
        address += 4
    ) {

        cout
            << "0x"
            << setw(4)
            << address
            << " -> 0x"
            << setw(8)
            << mem.memRead32(address)
            << '\n';
    }


    // =========================================
    // FINAL STATE
    // =========================================

    cout << dec;

    cout << "\n--- FINAL STATE ---\n";

    cout << "$t0 = "
         << cpu.registers[8]
         << '\n';

    cout << "$v0 = "
         << cpu.registers[2]
         << '\n';

    cout << "PC = "
         << cpu.pc
         << '\n';

    cout << "Instructions executed = "
         << instructionCount
         << '\n';


    // =========================================
    // SUMMARY
    // =========================================

    cout << "\n====================================\n";
    cout << "              SUMMARY\n";
    cout << "====================================\n";

    cout << "Passed: "
         << passed
         << '\n';

    cout << "Failed: "
         << failed
         << '\n';

    cout << "Total:  "
         << passed + failed
         << '\n';

    if (failed == 0) {
        cout << "\nALL DATA SECTION TESTS PASSED!\n";
        return 0;
    }

    cout << "\nSOME DATA SECTION TESTS FAILED!\n";

    return 1;
}