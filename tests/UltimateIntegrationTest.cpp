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

    cout << "========================================\n";
    cout << "      ULTIMATE MIPS INTEGRATION TEST\n";
    cout << "========================================\n\n";


    // ========================================
    // ASSEMBLE
    // ========================================

    bool assembled =
        assembler.assembleFile(
            "Assembly_Codes/UltimateIntegration.asm",
            mem
        );

    check(
        "Assembler completed successfully",
        assembled
    );

    if (!assembled) {
        return 1;
    }


    // ========================================
    // DATA SECTION
    // ========================================

    const string expectedMessage =
        "Ultimate MIPS Test";

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
        ".asciiz stored correctly",
        messageCorrect
    );

    check(
        ".asciiz null terminator stored",
        mem.memReadByte(
            0x1000 +
            static_cast<uint32_t>(
                expectedMessage.size()
            )
        ) == 0
    );


    // ========================================
    // FIND WHERE .WORD VALUES LANDED
    // ========================================

    uint32_t numberAddress =
        0x1000 +
        static_cast<uint32_t>(
            expectedMessage.size() + 1
        );

    uint32_t negativeAddress =
        numberAddress + 4;

    check(
        ".word 1234 stored correctly",
        mem.memRead32(numberAddress) == 1234
    );

    check(
        ".word -77 stored correctly",
        mem.memRead32(negativeAddress)
            == static_cast<uint32_t>(-77)
    );


    // ========================================
    // EXECUTE PROGRAM
    // ========================================

    int instructionCount = 0;

    const int MAX_INSTRUCTIONS = 1000;

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


    // ========================================
    // GENERAL CPU
    // ========================================

    check(
        "Program halted",
        cpu.halted
    );

    check(
        "Instruction limit not reached",
        instructionCount < MAX_INSTRUCTIONS
    );

    check(
        "$zero remains zero",
        cpu.registers[0] == 0
    );


    // ========================================
    // ARITHMETIC RESULTS
    // ========================================

    check(
        "ADD result",
        cpu.registers[11] == 30
    );

    check(
        "SUB result",
        cpu.registers[12] == 20
    );

    check(
        "ADDU result",
        cpu.registers[13] == 30
    );

    check(
        "SUBU result",
        cpu.registers[14] == 10
    );


    // ========================================
    // LOGICAL RESULTS
    // ========================================

    check(
        "XOR result",
        cpu.registers[19] == (0x0Au ^ 0x14u)
    );

    check(
        "NOR result",
        cpu.registers[20] == ~(10u | 20u)
    );

    check(
        "ANDI result",
        cpu.registers[21] == (20u & 0xFu)
    );

    check(
        "ORI result",
        cpu.registers[22] == 0xFF
    );

    check(
        "XORI result",
        cpu.registers[23] == (0xFFu ^ 0xFu)
    );


    // ========================================
    // COMPARISONS
    // ========================================

    check(
        "SLT result",
        cpu.registers[15] == 1
    );

    check(
        "SLTI result",
        cpu.registers[24] == 1
    );

    check(
        "SLTU result",
        cpu.registers[25] == 1
    );

    check(
        "SLTIU result",
        cpu.registers[26] == 1
    );


    // ========================================
    // SHIFTS
    // ========================================

    check(
        "SLL result",
        cpu.registers[27] == 40
    );

    check(
        "SRL result",
        cpu.registers[28] == 20
    );

    check(
        "SRA result",
        cpu.registers[29] == 10
    );


    // ========================================
    // MEMORY
    // ========================================

    check(
        "SW wrote correct value",
        mem.memRead32(200) == 30
    );

    check(
        "LW loaded correct value",
        cpu.registers[30] == 30
    );


    // ========================================
    // BRANCH / JUMP
    // ========================================

    check(
        "BEQ took correct branch",
        cpu.registers[3] == 1
    );

    check(
        "BNE took correct branch",
        cpu.registers[5] == 2
    );

    check(
        "J skipped unwanted instruction",
        cpu.registers[6] == 3
    );

    check(
        "JAL/JR returned correctly",
        cpu.registers[7] == 4
    );

    check(
        "$ra contains return address",
        cpu.registers[31] != 0
    );


    // ========================================
    // MULT / HI / LO
    // ========================================

    check(
        "MULT LO == 42",
        cpu.lo == 42
    );

    check(
        "MULT HI == 0",
        cpu.hi == 0
    );

    check(
        "MFLO result",
        cpu.registers[16] == 42
    );

    check(
        "MFHI result",
        cpu.registers[17] == 0
    );


    // ========================================
    // LA + DATA LOAD
    // ========================================

    check(
        "LA + LW loaded .word 1234",
        cpu.registers[9] == 1234
    );


    // ========================================
    // EXIT
    // ========================================

    check(
        "$v0 contains exit syscall code",
        cpu.registers[2] == 10
    );


    // ========================================
    // FINAL INFO
    // ========================================

    cout << dec;

    cout << "\n--- FINAL STATE ---\n";

    cout << "Instructions executed: "
         << instructionCount
         << '\n';

    cout << "PC: "
         << cpu.pc
         << '\n';

    cout << "HI: "
         << cpu.hi
         << '\n';

    cout << "LO: "
         << cpu.lo
         << '\n';


    // ========================================
    // SUMMARY
    // ========================================

    cout << "\n========================================\n";
    cout << "                 SUMMARY\n";
    cout << "========================================\n";

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
        cout
            << "\nALL ULTIMATE INTEGRATION TESTS PASSED!\n";

        return 0;
    }

    cout
        << "\nSOME ULTIMATE INTEGRATION TESTS FAILED!\n";

    return 1;
}
