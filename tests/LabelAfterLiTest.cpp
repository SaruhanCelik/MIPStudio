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
    cout << "      LABEL AFTER LI TEST SUITE\n";
    cout << "====================================\n\n";

    bool assembled =
        assembler.assembleFile(
            "Assembly_Codes/LabelAfterLiTest.asm",
            mem
        );

    check(
        "Assembly completed successfully",
        assembled
    );

    if (!assembled) {
        return 1;
    }

    cout << "\n--- MACHINE CODE ---\n";

    cout << hex
         << uppercase
         << setfill('0');

    for (uint32_t address = 0;
         address <= 0x18;
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

    // -----------------------------------------
    // Check jump encoding
    // -----------------------------------------

    uint32_t jumpInstruction =
        mem.memRead32(0x0008);

    uint8_t jumpOpcode =
        static_cast<uint8_t>(
            (jumpInstruction >> 26) & 0x3F
        );

    uint32_t jumpField =
        jumpInstruction & 0x03FFFFFF;

    uint32_t jumpTarget =
        jumpField << 2;

    check(
        "Instruction at 0x0008 is J",
        jumpOpcode == 2
    );

    check(
        "J target resolves to 0x0010",
        jumpTarget == 0x0010
    );

    // -----------------------------------------
    // Execute program
    // -----------------------------------------

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

    check(
        "32-bit LI loaded correct value into $t0",
        cpu.registers[8] == 0x12345678
    );

    check(
        "Jump skipped $t1 assignment",
        cpu.registers[9] == 0
    );

    check(
        "Execution reached target label",
        cpu.registers[10] == 42
    );

    cout << dec;

    cout << "\n--- FINAL STATE ---\n";

    cout << "$t0 = 0x"
         << hex
         << uppercase
         << cpu.registers[8]
         << '\n';

    cout << dec;

    cout << "$t1 = "
         << cpu.registers[9]
         << '\n';

    cout << "$t2 = "
         << cpu.registers[10]
         << '\n';

    cout << "PC = "
         << cpu.pc
         << '\n';

    cout << "Instructions executed = "
         << instructionCount
         << '\n';

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
        cout << "\nALL LABEL-AFTER-LI TESTS PASSED!\n";
        return 0;
    }

    cout << "\nSOME LABEL-AFTER-LI TESTS FAILED!\n";
    return 1;
}