#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

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
    cout << "      PRINT STRING TEST SUITE\n";
    cout << "====================================\n\n";


    // =========================================
    // ASSEMBLE
    // =========================================

    bool assembled =
        assembler.assembleFile(
            "Assembly_Codes/PrintStringTest.asm",
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
    // CHECK STRING IN DATA MEMORY
    // =========================================

    const string expected =
        "Hello MIPS!";

    bool stringStoredCorrectly = true;

    for (
        size_t i = 0;
        i < expected.size();
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
                expected[i]
            )
        ) {
            stringStoredCorrectly = false;
            break;
        }
    }

    check(
        "String stored correctly in data memory",
        stringStoredCorrectly
    );

    check(
        "String null terminator exists",
        mem.memReadByte(0x100B) == 0
    );


    // =========================================
    // CHECK LA EXPANSION
    // =========================================

    /*
        la $a0, message

        message = 0x1000

        Expected expansion:

        lui $a0, 0x0000
        ori $a0, $a0, 0x1000
    */

    uint32_t luiInstruction =
        mem.memRead32(0x0000);

    uint32_t oriInstruction =
        mem.memRead32(0x0004);

    uint8_t luiOpcode =
        static_cast<uint8_t>(
            (luiInstruction >> 26) & 0x3F
        );

    uint8_t oriOpcode =
        static_cast<uint8_t>(
            (oriInstruction >> 26) & 0x3F
        );

    check(
        "LA first instruction is LUI",
        luiOpcode == 15
    );

    check(
        "LA second instruction is ORI",
        oriOpcode == 13
    );


    // =========================================
    // EXECUTE ONLY LA FIRST
    // =========================================

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    check(
        "LA loaded 0x1000 into $a0",
        cpu.registers[4] == 0x1000
    );


    // =========================================
    // CONTINUE PROGRAM
    // =========================================

    int instructionCount = 2;

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


    // =========================================
    // FINAL CPU CHECKS
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
        "$a0 still points to message",
        cpu.registers[4] == 0x1000
    );

    check(
        "$v0 contains exit syscall code",
        cpu.registers[2] == 10
    );


    // =========================================
    // MEMORY DUMP
    // =========================================

    cout << "\n--- TEXT MEMORY ---\n";

    cout << hex
         << uppercase
         << setfill('0');

    for (
        uint32_t address = 0;
        address <= 0x0018;
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
        address <= 0x100B;
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
    // FINAL STATE
    // =========================================

    cout << dec;

    cout << "\n--- FINAL STATE ---\n";

    cout << "$a0 = 0x"
         << hex
         << uppercase
         << cpu.registers[4]
         << '\n';

    cout << dec;

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
        cout << "\nALL PRINT STRING TESTS PASSED!\n";
        return 0;
    }

    cout << "\nSOME PRINT STRING TESTS FAILED!\n";
    return 1;
}