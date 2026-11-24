#include <iostream>
#include <string>

#include "Assembler.h"
#include "DataMemory.h"

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

bool expectAssemblyFailure(
    const string& filename
) {
    Memory mem;
    Assembler assembler;

    bool result =
        assembler.assembleFile(
            filename,
            mem
        );

    return result == false;
}

int main() {

    cout << "====================================\n";
    cout << "      ASSEMBLER ERROR TEST SUITE\n";
    cout << "====================================\n\n";

    check(
        "Reject invalid register",
        expectAssemblyFailure(
            "Assembly_Codes/Error_InvalidRegister.asm"
        )
    );

    check(
        "Reject missing operand",
        expectAssemblyFailure(
            "Assembly_Codes/Error_MissingOperand.asm"
        )
    );

    check(
        "Reject too many operands",
        expectAssemblyFailure(
            "Assembly_Codes/Error_TooManyOperands.asm"
        )
    );

    check(
        "Reject invalid immediate",
        expectAssemblyFailure(
            "Assembly_Codes/Error_InvalidImmediate.asm"
        )
    );

    check(
        "Reject immediate out of range",
        expectAssemblyFailure(
            "Assembly_Codes/Error_ImmediateRange.asm"
        )
    );

    check(
        "Reject invalid shift amount",
        expectAssemblyFailure(
            "Assembly_Codes/Error_ShiftAmount.asm"
        )
    );

    check(
        "Reject unknown instruction",
        expectAssemblyFailure(
            "Assembly_Codes/Error_UnknownInstruction.asm"
        )
    );

    check(
        "Reject invalid LW offset",
        expectAssemblyFailure(
            "Assembly_Codes/Error_LWOffset.asm"
        )
    );

    check(
        "Reject invalid jump target",
        expectAssemblyFailure(
            "Assembly_Codes/Error_JumpTarget.asm"
        )
    );

    cout << dec;

    cout << "\n====================================\n";
    cout << "              SUMMARY\n";
    cout << "====================================\n";

    cout << "Passed: " << passed << '\n';
    cout << "Failed: " << failed << '\n';
    cout << "Total:  " << passed + failed << '\n';

    if (failed == 0) {
        cout << "\nALL ASSEMBLER ERROR TESTS PASSED!\n";
        return 0;
    }

    cout << "\nSOME ASSEMBLER ERROR TESTS FAILED!\n";
    return 1;
}