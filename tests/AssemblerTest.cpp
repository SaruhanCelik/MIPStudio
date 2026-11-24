#include <iostream>
#include <iomanip>

#include "DataMemory.h"
#include "CPU.h"
#include "ALU.h"
#include "ControlUnit.h"
#include "Assembler.h"

int passed = 0;
int failed = 0;
using namespace std;

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
    cout << "   ASSEMBLER + CPU INTEGRATION TEST\n";
    cout << "====================================\n\n";


    // ============================================================
    // 1. ASSEMBLE deneme.asm DIRECTLY INTO MEMORY
    // ============================================================

    bool assembled = assembler.assembleFile(
        "Assembly_Codes/deneme.asm",
        mem
    );

    check(
        "Assembly file loaded successfully",
        assembled
    );

    if (!assembled) {
        cout << "\nAssembly failed. Stopping tests.\n";
        return 1;
    }


    // ============================================================
    // 2. VERIFY GENERATED MACHINE CODE
    // ============================================================

    cout << "\n--- MACHINE CODE CHECK ---\n";

    cout << hex
              << uppercase
              << setfill('0');

    cout << "Memory[0x0000] = 0x"
              << setw(8)
              << mem.memRead32(0)
              << '\n';

    cout << "Memory[0x0004] = 0x"
              << setw(8)
              << mem.memRead32(4)
              << '\n';

    cout << "Memory[0x0008] = 0x"
              << setw(8)
              << mem.memRead32(8)
              << '\n';

    cout << "Memory[0x000C] = 0x"
              << setw(8)
              << mem.memRead32(12)
              << '\n';


    check(
        "ADDI $t0 assembled correctly",
        mem.memRead32(0) == 0x2008000A
    );

    check(
        "ADDI $t1 assembled correctly",
        mem.memRead32(4) == 0x20090014
    );

    check(
        "ADD $t2 assembled correctly",
        mem.memRead32(8) == 0x01095020
    );

    check(
        "SUB $t3 assembled correctly",
        mem.memRead32(12) == 0x01485822
    );


    // ============================================================
    // 3. EXECUTE INSTRUCTION 1
    // addi $t0, $zero, 10
    // ============================================================

    cout << "\n--- EXECUTION ---\n";

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    check(
        "Instruction 1: $t0 == 10",
        cpu.registers[8] == 10
    );

    check(
        "Instruction 1: PC == 4",
        cpu.pc == 4
    );


    // ============================================================
    // 4. EXECUTE INSTRUCTION 2
    // addi $t1, $zero, 20
    // ============================================================

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    check(
        "Instruction 2: $t1 == 20",
        cpu.registers[9] == 20
    );

    check(
        "Instruction 2: PC == 8",
        cpu.pc == 8
    );


    // ============================================================
    // 5. EXECUTE INSTRUCTION 3
    // add $t2, $t0, $t1
    // ============================================================

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    check(
        "Instruction 3: $t2 == 30",
        cpu.registers[10] == 30
    );

    check(
        "Instruction 3: PC == 12",
        cpu.pc == 12
    );


    // ============================================================
    // 6. EXECUTE INSTRUCTION 4
    // sub $t3, $t2, $t0
    // ============================================================

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    check(
        "Instruction 4: $t3 == 20",
        cpu.registers[11] == 20
    );

    check(
        "Instruction 4: PC == 16",
        cpu.pc == 16
    );


    // ============================================================
    // 7. FINAL REGISTER STATE
    // ============================================================

    cout << "\n--- FINAL REGISTER STATE ---\n";

    cout << dec;

    cout << "$t0 = "
              << cpu.registers[8]
              << '\n';

    cout << "$t1 = "
              << cpu.registers[9]
              << '\n';

    cout << "$t2 = "
              << cpu.registers[10]
              << '\n';

    cout << "$t3 = "
              << cpu.registers[11]
              << '\n';

    cout << "PC  = "
              << cpu.pc
              << '\n';


    check(
        "Final $t0 == 10",
        cpu.registers[8] == 10
    );

    check(
        "Final $t1 == 20",
        cpu.registers[9] == 20
    );

    check(
        "Final $t2 == 30",
        cpu.registers[10] == 30
    );

    check(
        "Final $t3 == 20",
        cpu.registers[11] == 20
    );


    // ============================================================
    // SUMMARY
    // ============================================================

    cout << "\n====================================\n";
    cout << "             SUMMARY\n";
    cout << "====================================\n";

    cout << "Passed: " << passed << '\n';
    cout << "Failed: " << failed << '\n';
    cout << "Total:  " << passed + failed << '\n';

    if (failed == 0) {
        cout << "\nALL TESTS PASSED!\n";
        return 0;
    }

    cout << "\nSOME TESTS FAILED!\n";
    return 1;
}