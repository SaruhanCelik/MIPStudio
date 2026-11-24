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

    cout << "====================================\n";
    cout << "     MIPS FULL INTEGRATION TEST\n";
    cout << "====================================\n\n";


    // =========================================
    // ASSEMBLE
    // =========================================

    bool assembled = assembler.assembleFile(
        "Assembly_Codes/IntegrationTest.asm",
        mem
    );

    check(
        "Assembler completed successfully",
        assembled
    );

    if (!assembled) {
        cout << "\nAssembly failed. Cannot continue.\n";
        return 1;
    }


    // =========================================
    // RUN PROGRAM
    // =========================================

    const int MAX_INSTRUCTIONS = 1000;

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
    // GENERAL EXECUTION
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
    // ARITHMETIC
    // =========================================

    // Note: t0/t1 are overwritten later by MULT test,
    // so don't validate their original 10/20 values here.

    check(
        "ADD result stored in $t2",
        cpu.registers[10] == 30
    );

    check(
        "SUB result stored in $t3",
        cpu.registers[11] == 20
    );

    check(
        "ADDU result stored in $t4",
        cpu.registers[12] == 30
    );

    check(
        "SUBU result stored in $t5",
        cpu.registers[13] == 10
    );


    // =========================================
    // LOGICAL
    // =========================================

    // Later MULT/MFLO overwrites $s0
    // and MFHI overwrites $s1,
    // so validate logical results that survive.

    check(
        "XOR result in $s2",
        cpu.registers[18] == (0x0A ^ 0x14)
    );

    check(
        "NOR result in $s3",
        cpu.registers[19] == ~(10u | 20u)
    );

    check(
        "ANDI result in $s4",
        cpu.registers[20] == (20u & 0xFu)
    );

    check(
        "ORI result in $s5",
        cpu.registers[21] == 0xFF
    );

    check(
        "XORI result in $s6",
        cpu.registers[22] == (0xFFu ^ 0xFu)
    );


    // =========================================
    // COMPARISONS
    // =========================================

    check(
        "SLT result",
        cpu.registers[23] == 1
    );

    check(
        "SLTI result",
        cpu.registers[14] == 1
    );

    check(
        "SLTU result",
        cpu.registers[15] == 1
    );

    check(
        "SLTIU result",
        cpu.registers[24] == 1
    );


    // =========================================
    // SHIFTS
    // =========================================

    check(
        "SLL result",
        cpu.registers[25] == 40
    );

    check(
        "SRL result",
        cpu.registers[26] == 20
    );

    check(
        "SRA result",
        cpu.registers[27] == 10
    );


    // =========================================
    // MEMORY
    // =========================================

    check(
        "SW wrote value to memory",
        mem.memRead32(100) == 30
    );

    // $a0 later becomes 42 for syscall print,
    // so don't validate LW through $a0 here.
    // Memory result above already confirms SW path.


    // =========================================
    // BRANCHES
    // =========================================

    check(
        "BEQ took correct path",
        cpu.registers[3] == 1
    );

    check(
        "BNE took correct path",
        cpu.registers[5] == 2
    );


    // =========================================
    // JUMP
    // =========================================

    check(
        "J skipped unwanted instruction",
        cpu.registers[6] == 3
    );


    // =========================================
    // JAL / JR
    // =========================================

    // function sets $a3 = 40,
    // then returns and following instruction makes it 4

    check(
        "JAL/JR returned correctly",
        cpu.registers[7] == 4
    );

    check(
        "$ra contains a return address",
        cpu.registers[31] != 0
    );


    // =========================================
    // MULT / HI / LO
    // =========================================

    check(
        "MULT result in LO",
        cpu.lo == 42
    );

    check(
        "MULT high result in HI",
        cpu.hi == 0
    );

    check(
        "MFLO copied LO",
        cpu.registers[16] == 42
    );

    check(
        "MFHI copied HI",
        cpu.registers[17] == 0
    );


    // =========================================
    // SYSCALL
    // =========================================

    check(
        "Exit syscall code left in $v0",
        cpu.registers[2] == 10
    );

    check(
        "Print argument left in $a0",
        cpu.registers[4] == 42
    );


// =========================================
// FINAL SUMMARY
// =========================================

cout << dec;  // switch cout back to decimal

cout << "\n====================================\n";
cout << "              SUMMARY\n";
cout << "====================================\n";

cout << "Passed: " << passed << '\n';
cout << "Failed: " << failed << '\n';
cout << "Total:  " << passed + failed << '\n';

cout << "Instructions executed: "
     << instructionCount
     << '\n';

if (failed == 0) {
    cout << "\nALL INTEGRATION TESTS PASSED!\n";
    return 0;
}

cout << "\nSOME INTEGRATION TESTS FAILED!\n";
return 1;
}
