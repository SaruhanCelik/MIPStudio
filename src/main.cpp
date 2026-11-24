#include <iostream>

#include "CPU.h"
#include "ALU.h"
#include "ControlUnit.h"
#include "DataMemory.h"
#include "Assembler.h"
#include "Debugger.h"

using namespace std;

int main(
    int argc,
    char* argv[]
) {

    if (argc < 2) {

        cout
            << "Usage: mips.exe <program.asm>\n";

        return 1;
    }

    Memory mem;
    CPU cpu;
    ALU alu;
    ControlUnit control;
    Assembler assembler;


    // =========================================
    // ASSEMBLE PROGRAM
    // =========================================

    if (!assembler.assembleFile(
            argv[1],
            mem
        )) {

        cout << "Assembly failed.\n";
        return 1;
    }


    // =========================================
    // START DEBUGGER
    // =========================================

    Debugger debugger(
        cpu,
        mem,
        alu,
        control,
        assembler,
        argv[1]
    );

    debugger.start();

    return 0;
}