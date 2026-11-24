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
    cout << "        ASCIIZ TEST SUITE\n";
    cout << "====================================\n\n";


    // =========================================
    // ASSEMBLE
    // =========================================

    bool assembled =
        assembler.assembleFile(
            "Assembly_Codes/AsciizTest.asm",
            mem
        );

    check(
        "Assembly completed successfully",
        assembled
    );

    if (!assembled) {
        return 1;
    }


    // =========================================
    // FIRST STRING: "Hello MIPS!"
    // =========================================

    const string expectedMessage =
        "Hello MIPS!";

    bool messageCorrect = true;

    for (
        size_t i = 0;
        i < expectedMessage.size();
        i++
    ) {

        uint8_t actual =
            mem.memReadByte(
                0x1000 +
                static_cast<uint32_t>(i)
            );

        if (
            actual !=
            static_cast<uint8_t>(
                expectedMessage[i]
            )
        ) {
            messageCorrect = false;
            break;
        }
    }

    check(
        "\"Hello MIPS!\" stored correctly",
        messageCorrect
    );


    // =========================================
    // NULL TERMINATOR
    // =========================================

    check(
        "First string has null terminator",
        mem.memReadByte(0x100B) == 0
    );


    // =========================================
    // SECOND STRING
    // =========================================

    check(
        "Second string begins at correct address",
        mem.memReadByte(0x100C) == 'A'
    );

    check(
        "Second string second character",
        mem.memReadByte(0x100D) == 'B'
    );

    check(
        "Second string third character",
        mem.memReadByte(0x100E) == 'C'
    );

    check(
        "Second string has null terminator",
        mem.memReadByte(0x100F) == 0
    );


    // =========================================
    // .WORD AFTER STRINGS
    // =========================================

    check(
        ".word after strings stored at correct address",
        mem.memRead32(0x1010) == 42
    );


    // =========================================
    // TEXT SECTION
    // =========================================

    check(
        "Exit setup instruction at text address 0",
        mem.memRead32(0x0000) == 0x2002000A
    );

    check(
        "SYSCALL at text address 4",
        mem.memRead32(0x0004) == 0x0000000C
    );


    // =========================================
    // RUN PROGRAM
    // =========================================

    int instructionCount = 0;
    const int MAX_INSTRUCTIONS = 100;

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

    check(
        "Program halted",
        cpu.halted
    );

    check(
        "Instruction limit not reached",
        instructionCount < MAX_INSTRUCTIONS
    );


    // =========================================
    // MEMORY DUMP
    // =========================================

    cout << "\n--- DATA MEMORY ---\n";

    cout << hex
         << uppercase
         << setfill('0');

    for (
        uint32_t address = 0x1000;
        address <= 0x1013;
        address++
    ) {

        cout
            << "0x"
            << setw(4)
            << address
            << " -> 0x"
            << setw(2)
            << static_cast<int>(
                mem.memReadByte(address)
            )
            << '\n';
    }


    // =========================================
    // SUMMARY
    // =========================================

    cout << dec;

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
        cout << "\nALL ASCIIZ TESTS PASSED!\n";
        return 0;
    }

    cout << "\nSOME ASCIIZ TESTS FAILED!\n";
    return 1;
}