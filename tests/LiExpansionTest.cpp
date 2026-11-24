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
    cout << "       LI EXPANSION TEST SUITE\n";
    cout << "====================================\n\n";


    // =========================================
    // ASSEMBLE
    // =========================================

    bool assembled =
        assembler.assembleFile(
            "Assembly_Codes/LiExpansionTest.asm",
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
    // MACHINE CODE LAYOUT CHECK
    // =========================================

    cout << "\n--- MACHINE CODE ---\n";

    cout << hex
         << uppercase
         << setfill('0');

    for (uint32_t address = 0;
         address <= 0x20;
         address += 4) {

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
    // EXECUTE PROGRAM
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
    // GENERAL CHECKS
    // =========================================

    check(
        "Program halted",
        cpu.halted
    );

    check(
        "Instruction limit not reached",
        instructionCount < MAX_INSTRUCTIONS
    );


    // =========================================
    // LI $t0, 10
    // =========================================

    check(
        "16-bit LI loaded 10 into $t0",
        cpu.registers[8] == 10
    );


    // =========================================
    // LI $t1, 0x12345678
    // =========================================

    check(
        "32-bit LI loaded 0x12345678 into $t1",
        cpu.registers[9] == 0x12345678
    );


    // =========================================
    // LI $t2, -50000
    // =========================================

    check(
        "32-bit negative LI loaded -50000 into $t2",
        static_cast<int32_t>(
            cpu.registers[10]
        ) == -50000
    );

    check(
        "32-bit negative LI bit pattern correct",
        cpu.registers[10] == 0xFFFF3CB0
    );


    // =========================================
    // MOVE
    // =========================================

    check(
        "MOVE copied $t1 into $s0",
        cpu.registers[16] == 0x12345678
    );


    // =========================================
    // NOP
    // =========================================

    // NOP should not modify $zero
    check(
        "NOP leaves $zero unchanged",
        cpu.registers[0] == 0
    );


    // =========================================
    // LUI + ORI EXPANSION CHECK
    // =========================================

    /*
        Expected rough layout:

        0x0000 -> li $t0, 10          (1 instruction)

        0x0004 -> LUI $t1, 0x1234
        0x0008 -> ORI $t1, $t1, 0x5678

        0x000C -> LUI for -50000
        0x0010 -> ORI for -50000

        0x0014 -> move $s0, $t1
        0x0018 -> nop
        0x001C -> addi $v0, $zero, 10
        0x0020 -> syscall
    */

    uint32_t t1Lui =
        mem.memRead32(0x0004);

    uint32_t t1Ori =
        mem.memRead32(0x0008);

    uint8_t luiOpcode =
        static_cast<uint8_t>(
            (t1Lui >> 26) & 0x3F
        );

    uint8_t oriOpcode =
        static_cast<uint8_t>(
            (t1Ori >> 26) & 0x3F
        );

    check(
        "32-bit LI first instruction is LUI",
        luiOpcode == 15
    );

    check(
        "32-bit LI second instruction is ORI",
        oriOpcode == 13
    );


    // =========================================
    // ADDRESS / EXPANSION CHECK
    // =========================================

    check(
        "MOVE appears after expanded LI instructions",
        mem.memRead32(0x0014) != 0
    );

    check(
        "NOP encoded as 0x00000000",
        mem.memRead32(0x0018) == 0x00000000
    );


    // =========================================
    // FINAL STATE
    // =========================================

    cout << dec;

    cout << "\n--- FINAL REGISTER STATE ---\n";

    cout << "$t0 = "
         << cpu.registers[8]
         << '\n';

    cout << "$t1 = 0x"
         << hex
         << uppercase
         << cpu.registers[9]
         << '\n';

    cout << dec;

    cout << "$t2 = "
         << static_cast<int32_t>(
                cpu.registers[10]
            )
         << '\n';

    cout << "$s0 = 0x"
         << hex
         << uppercase
         << cpu.registers[16]
         << '\n';

    cout << dec;

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
        cout << "\nALL LI EXPANSION TESTS PASSED!\n";
        return 0;
    }

    cout << "\nSOME LI EXPANSION TESTS FAILED!\n";

    return 1;
}