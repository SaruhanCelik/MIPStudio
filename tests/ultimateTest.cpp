//////////////           ULTIMATE TEST SUITE          //////////////////
#include <iostream>
#include <iomanip>
#include <string>

#include "DataMemory.h"
#include "CPU.h"
#include "ControlUnit.h"
#include "ALU.h"

using std::cout;
using std::string;


// ============================================================
// TEST UTILITIES
// ============================================================

int testsPassed = 0;
int testsFailed = 0;

void check(const string& testName, bool condition) {
    if (condition) {
        cout << "[PASS] " << testName << '\n';
        testsPassed++;
    }
    else {
        cout << "[FAIL] " << testName << '\n';
        testsFailed++;
    }
}


// ============================================================
// MEMORY TESTS
// ============================================================

void testMemoryByte() {
    Memory mem;

    mem.memWriteByte(100, 0xAB);

    check(
        "Memory byte read/write",
        mem.memReadByte(100) == 0xAB
    );
}


void testMemory32() {
    Memory mem;

    mem.memWrite32(100, 0x12345678);

    check(
        "Memory 32-bit read/write",
        mem.memRead32(100) == 0x12345678
    );
}


// ============================================================
// REGISTER TESTS
// ============================================================

void testZeroRegister() {
    CPU cpu;

    cpu.regWrite(0, 12345);

    check(
        "$zero remains zero",
        cpu.registers[0] == 0
    );
}


void testNormalRegisterWrite() {
    CPU cpu;

    cpu.regWrite(8, 42);

    check(
        "Normal register write",
        cpu.registers[8] == 42
    );
}


// ============================================================
// ADDI TEST
// ============================================================

void testADDI() {
    Memory mem;
    CPU cpu;
    ALU alu;
    ControlUnit control;

    // addi $t0, $zero, 10
    // machine code = 0x2008000A

    mem.memWrite32(0, 0x2008000A);

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    check(
        "ADDI result: $t0 = 10",
        cpu.registers[8] == 10
    );

    check(
        "ADDI advances PC by 4",
        cpu.pc == 4
    );
}


// ============================================================
// ADD TEST
// ============================================================

void testADD() {
    Memory mem;
    CPU cpu;
    ALU alu;
    ControlUnit control;

    cpu.regWrite(8, 10); // $t0
    cpu.regWrite(9, 20); // $t1

    // add $t2, $t0, $t1
    // machine code = 0x01095020

    mem.memWrite32(0, 0x01095020);

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    check(
        "ADD result: $t2 = 30",
        cpu.registers[10] == 30
    );

    check(
        "ADD preserves $t0",
        cpu.registers[8] == 10
    );

    check(
        "ADD preserves $t1",
        cpu.registers[9] == 20
    );

    check(
        "ADD advances PC by 4",
        cpu.pc == 4
    );
}


// ============================================================
// LW TEST
// ============================================================

void testLW() {
    Memory mem;
    CPU cpu;
    ALU alu;
    ControlUnit control;

    cpu.regWrite(9, 96); // $t1

    mem.memWrite32(
        100,
        0x12345678
    );

    // lw $t0, 4($t1)
    // machine code = 0x8D280004

    mem.memWrite32(
        0,
        0x8D280004
    );

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    check(
        "LW loads correct value",
        cpu.registers[8] == 0x12345678
    );

    check(
        "LW advances PC by 4",
        cpu.pc == 4
    );
}


// ============================================================
// SW TEST
// ============================================================

void testSW() {
    Memory mem;
    CPU cpu;
    ALU alu;
    ControlUnit control;

    cpu.regWrite(9, 96);           // $t1
    cpu.regWrite(8, 0xCAFEBABE);   // $t0

    // sw $t0, 4($t1)
    // machine code = 0xAD280004

    mem.memWrite32(
        0,
        0xAD280004
    );

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    check(
        "SW stores correct value",
        mem.memRead32(100) == 0xCAFEBABE
    );

    check(
        "SW advances PC by 4",
        cpu.pc == 4
    );
}


// ============================================================
// BEQ TAKEN
// ============================================================

void testBEQTaken() {
    Memory mem;
    CPU cpu;
    ALU alu;
    ControlUnit control;

    cpu.regWrite(8, 10);
    cpu.regWrite(9, 10);

    // beq $t0, $t1, 2
    // machine code = 0x11090002

    mem.memWrite32(
        0,
        0x11090002
    );

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    check(
        "BEQ taken",
        cpu.pc == 12
    );
}


// ============================================================
// BEQ NOT TAKEN
// ============================================================

void testBEQNotTaken() {
    Memory mem;
    CPU cpu;
    ALU alu;
    ControlUnit control;

    cpu.regWrite(8, 10);
    cpu.regWrite(9, 20);

    mem.memWrite32(
        0,
        0x11090002
    );

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    check(
        "BEQ not taken",
        cpu.pc == 4
    );
}


// ============================================================
// NEGATIVE BEQ OFFSET
// ============================================================

void testBEQNegativeOffset() {
    Memory mem;
    CPU cpu;
    ALU alu;
    ControlUnit control;

    /*
        Instruction is at address 8.

        beq $t0, $t1, -2

        fetch:
            PC = 12

        offset:
            -2 << 2 = -8

        new PC:
            12 - 8 = 4
    */

    cpu.pc = 8;

    cpu.regWrite(8, 100);
    cpu.regWrite(9, 100);

    // beq $t0, $t1, -2
    // machine code = 0x1109FFFE

    mem.memWrite32(
        8,
        0x1109FFFE
    );

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    check(
        "BEQ negative offset / loop backwards",
        cpu.pc == 4
    );
}


// ============================================================
// JUMP TEST
// ============================================================

void testJump() {
    Memory mem;
    CPU cpu;
    ALU alu;
    ControlUnit control;

    // j 4
    // target = 4 << 2 = 16
    // machine code = 0x08000004

    mem.memWrite32(
        0,
        0x08000004
    );

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    check(
        "J sets PC to correct target",
        cpu.pc == 16
    );
}


// ============================================================
// MULTIPLE INSTRUCTION TEST
// ============================================================

void testSmallProgram() {
    Memory mem;
    CPU cpu;
    ALU alu;
    ControlUnit control;

    /*
        addi $t0, $zero, 10
        addi $t1, $zero, 20
        add  $t2, $t0, $t1

        Expected:

        $t0 = 10
        $t1 = 20
        $t2 = 30
        PC  = 12
    */

    mem.memWrite32(
        0,
        0x2008000A
    );

    mem.memWrite32(
        4,
        0x20090014
    );

    mem.memWrite32(
        8,
        0x01095020
    );

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

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    check(
        "Program: $t0 = 10",
        cpu.registers[8] == 10
    );

    check(
        "Program: $t1 = 20",
        cpu.registers[9] == 20
    );

    check(
        "Program: $t2 = 30",
        cpu.registers[10] == 30
    );

    check(
        "Program: PC = 12",
        cpu.pc == 12
    );
}


// ============================================================
// MAIN
// ============================================================

int main() {

    cout << "========================================\n";
    cout << "        MIPS EMULATOR TEST SUITE        \n";
    cout << "========================================\n\n";

    testMemoryByte();
    testMemory32();

    testZeroRegister();
    testNormalRegisterWrite();

    testADDI();
    testADD();

    testLW();
    testSW();

    testBEQTaken();
    testBEQNotTaken();
    testBEQNegativeOffset();

    testJump();

    testSmallProgram();

    cout << "\n========================================\n";
    cout << "              TEST SUMMARY              \n";
    cout << "========================================\n";

    cout << "Passed: " << testsPassed << '\n';
    cout << "Failed: " << testsFailed << '\n';
    cout << "Total:  "
         << testsPassed + testsFailed
         << '\n';

    if (testsFailed == 0) {
        cout << "\nALL TESTS PASSED!\n";
        return 0;
    }

    cout << "\nSOME TESTS FAILED!\n";

    return 1;
}



