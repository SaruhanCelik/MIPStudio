#include "Debugger.h"

#include "CPU.h"
#include "DataMemory.h"
#include "ALU.h"
#include "ControlUnit.h"
#include "Assembler.h"

#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;


// ============================================================
// REGISTER NAMES
// ============================================================

static const char* REGISTER_NAMES[32] = {
    "$zero", "$at",
    "$v0", "$v1",
    "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3",
    "$t4", "$t5", "$t6", "$t7",
    "$s0", "$s1", "$s2", "$s3",
    "$s4", "$s5", "$s6", "$s7",
    "$t8", "$t9",
    "$k0", "$k1",
    "$gp", "$sp", "$fp", "$ra"
};


// ============================================================
// CONSTRUCTOR
// ============================================================

Debugger::Debugger(
    CPU& cpu,
    Memory& mem,
    ALU& alu,
    ControlUnit& control,
    Assembler& assembler,
    const string& programFile
)
    : cpu(cpu),
      mem(mem),
      alu(alu),
      control(control),
      assembler(assembler),
      programFile(programFile)
{
}


// ============================================================
// ADDRESS PARSER
// ============================================================

bool Debugger::parseAddress(
    const string& text,
    uint32_t& address
) {
    try {

        size_t position = 0;

        unsigned long value =
            stoul(
                text,
                &position,
                0
            );

        if (position != text.size()) {
            return false;
        }

        address =
            static_cast<uint32_t>(value);

        return true;
    }
    catch (...) {
        return false;
    }
}


// ============================================================
// STEP
// ============================================================

void Debugger::step() {

    if (cpu.halted) {
        cout << "CPU is halted. Use 'reset' to restart.\n";
        return;
    }

    uint32_t oldPC = cpu.pc;

    uint32_t instruction =
        mem.memRead32(oldPC);

    cout << hex
         << uppercase
         << setfill('0');

    cout << "0x"
         << setw(8)
         << oldPC
         << ": 0x"
         << setw(8)
         << instruction
         << "    "
         << disassemble(
                instruction,
                oldPC
            )
         << '\n';

    cpu.executeInstruction(
        mem,
        control,
        alu
    );

    cout << "PC -> 0x"
         << setw(8)
         << cpu.pc
         << '\n';

    cout << dec;
}


// ============================================================
// RUN
// ============================================================

void Debugger::run() {

    if (cpu.halted) {
        cout << "CPU is halted. Use 'reset' to restart.\n";
        return;
    }

    const int MAX_INSTRUCTIONS = 100000;

    int count = 0;

    while (
        !cpu.halted &&
        count < MAX_INSTRUCTIONS
    ) {

        // Stop BEFORE executing the instruction
        // located at a breakpoint.
        if (
            breakpoints.find(cpu.pc)
            != breakpoints.end()
        ) {

            cout << hex
                 << uppercase
                 << setfill('0');

            cout << "\nBreakpoint reached at 0x"
                 << setw(8)
                 << cpu.pc
                 << '\n';

            uint32_t instruction =
                mem.memRead32(cpu.pc);

            cout << "Next instruction: "
                 << disassemble(
                        instruction,
                        cpu.pc
                    )
                 << '\n';

            cout << dec;

            return;
        }

        cpu.executeInstruction(
            mem,
            control,
            alu
        );

        count++;
    }

    cout << '\n';

    if (cpu.halted) {

        cout << "Program halted.\n";
    }
    else {

        cout
            << "Execution stopped: "
            << "instruction limit reached.\n";
    }

    cout << "Instructions executed: "
         << count
         << '\n';
}


// ============================================================
// RESET
// ============================================================

void Debugger::reset() {

    for (int i = 0; i < 32; i++) {
        cpu.registers[i] = 0;
    }

    cpu.pc = 0;
    cpu.hi = 0;
    cpu.lo = 0;
    cpu.halted = false;

    if (!assembler.assembleFile(
            programFile,
            mem
        )) {

        cout << "Reset failed: assembly failed.\n";
        return;
    }

    cout << "CPU reset.\n";
    cout << "Program reloaded.\n";
}


// ============================================================
// REGISTERS
// ============================================================

void Debugger::printRegisters() {

    // Reset formatting left behind by other debugger commands
    cout << setfill(' ');

    cout << "\n";

    cout
        << left
        << setw(10) << "Register"
        << setw(14) << "Hex"
        << setw(14) << "Decimal"
        << '\n';

    cout
        << "--------------------------------------\n";

    for (int i = 0; i < 32; i++) {

        // Register name
        cout
            << left
            << setfill(' ')
            << setw(10)
            << REGISTER_NAMES[i];

        // Hex value
        stringstream hexValue;

        hexValue
            << "0x"
            << hex
            << uppercase
            << setfill('0')
            << setw(8)
            << cpu.registers[i];

        cout
            << left
            << setfill(' ')
            << setw(14)
            << hexValue.str();

        // Signed decimal value
        cout
            << left
            << setw(14)
            << dec
            << static_cast<int32_t>(
                   cpu.registers[i]
               );

        cout << '\n';
    }

    cout << "\n";

    cout
        << left
        << setw(10) << "PC"
        << "0x"
        << hex
        << uppercase
        << setfill('0')
        << setw(8)
        << cpu.pc
        << '\n';

    cout
        << setfill(' ')
        << left
        << setw(10) << "HI"
        << "0x"
        << hex
        << uppercase
        << setfill('0')
        << setw(8)
        << cpu.hi
        << '\n';

    cout
        << setfill(' ')
        << left
        << setw(10) << "LO"
        << "0x"
        << hex
        << uppercase
        << setfill('0')
        << setw(8)
        << cpu.lo
        << '\n';

    cout
        << setfill(' ')
        << left
        << setw(10) << "Halted"
        << (cpu.halted ? "Yes" : "No")
        << '\n';

    cout << dec;
}


// ============================================================
// MEMORY
// ============================================================

void Debugger::printMemory(
    uint32_t address,
    uint32_t count
) {

    cout << hex
         << uppercase
         << setfill('0');

    for (uint32_t i = 0; i < count; i++) {

        uint32_t currentAddress =
            address + i;

        uint8_t value =
            mem.memReadByte(
                currentAddress
            );

        cout << "0x"
             << setw(8)
             << currentAddress
             << " : 0x"
             << setw(2)
             << static_cast<unsigned int>(
                    value
                );

        if (
            value >= 32 &&
            value <= 126
        ) {

            cout << "    '"
                 << static_cast<char>(value)
                 << "'";
        }

        cout << '\n';
    }

    cout << dec;
}


// ============================================================
// ADD BREAKPOINT
// ============================================================

void Debugger::addBreakpoint(
    uint32_t address
) {

    if (address % 4 != 0) {

        cout
            << "Warning: instruction addresses "
            << "should normally be 4-byte aligned.\n";
    }

    auto result =
        breakpoints.insert(address);

    if (!result.second) {

        cout << "Breakpoint already exists at 0x"
             << hex
             << uppercase
             << address
             << dec
             << '\n';

        return;
    }

    cout << "Breakpoint added at 0x"
         << hex
         << uppercase
         << address
         << dec
         << '\n';
}


// ============================================================
// REMOVE BREAKPOINT
// ============================================================

void Debugger::removeBreakpoint(
    uint32_t address
) {

    size_t removed =
        breakpoints.erase(address);

    if (removed == 0) {

        cout << "No breakpoint exists at 0x"
             << hex
             << uppercase
             << address
             << dec
             << '\n';

        return;
    }

    cout << "Breakpoint removed from 0x"
         << hex
         << uppercase
         << address
         << dec
         << '\n';
}


// ============================================================
// PRINT BREAKPOINTS
// ============================================================

void Debugger::printBreakpoints() {

    if (breakpoints.empty()) {

        cout << "No breakpoints set.\n";
        return;
    }

    cout << "\nBreakpoints:\n";

    cout << hex
         << uppercase
         << setfill('0');

    for (uint32_t address : breakpoints) {

        uint32_t instruction =
            mem.memRead32(address);

        cout << "  0x"
             << setw(8)
             << address
             << "    "
             << disassemble(
                    instruction,
                    address
                )
             << '\n';
    }

    cout << dec;
}


// ============================================================
// DISASSEMBLER
// ============================================================

string Debugger::disassemble(
    uint32_t instruction,
    uint32_t address
) {

    uint8_t opcode =
        static_cast<uint8_t>(
            (instruction >> 26) & 0x3F
        );

    uint8_t rs =
        static_cast<uint8_t>(
            (instruction >> 21) & 0x1F
        );

    uint8_t rt =
        static_cast<uint8_t>(
            (instruction >> 16) & 0x1F
        );

    uint8_t rd =
        static_cast<uint8_t>(
            (instruction >> 11) & 0x1F
        );

    uint8_t shamt =
        static_cast<uint8_t>(
            (instruction >> 6) & 0x1F
        );

    uint8_t funct =
        static_cast<uint8_t>(
            instruction & 0x3F
        );

    uint16_t immediate =
        static_cast<uint16_t>(
            instruction & 0xFFFF
        );

    int16_t signedImmediate =
        static_cast<int16_t>(
            immediate
        );

    uint32_t target =
        instruction & 0x03FFFFFF;

    stringstream out;


    // ========================================================
    // SPECIAL / R-TYPE
    // ========================================================

    if (opcode == 0) {

        // NOP
        if (instruction == 0) {

            return "nop";
        }

        switch (funct) {

            case 0x20:
                out << "add "
                    << REGISTER_NAMES[rd]
                    << ", "
                    << REGISTER_NAMES[rs]
                    << ", "
                    << REGISTER_NAMES[rt];
                break;

            case 0x21:
                out << "addu "
                    << REGISTER_NAMES[rd]
                    << ", "
                    << REGISTER_NAMES[rs]
                    << ", "
                    << REGISTER_NAMES[rt];
                break;

            case 0x22:
                out << "sub "
                    << REGISTER_NAMES[rd]
                    << ", "
                    << REGISTER_NAMES[rs]
                    << ", "
                    << REGISTER_NAMES[rt];
                break;

            case 0x23:
                out << "subu "
                    << REGISTER_NAMES[rd]
                    << ", "
                    << REGISTER_NAMES[rs]
                    << ", "
                    << REGISTER_NAMES[rt];
                break;

            case 0x24:
                out << "and "
                    << REGISTER_NAMES[rd]
                    << ", "
                    << REGISTER_NAMES[rs]
                    << ", "
                    << REGISTER_NAMES[rt];
                break;

            case 0x25:
                out << "or "
                    << REGISTER_NAMES[rd]
                    << ", "
                    << REGISTER_NAMES[rs]
                    << ", "
                    << REGISTER_NAMES[rt];
                break;

            case 0x26:
                out << "xor "
                    << REGISTER_NAMES[rd]
                    << ", "
                    << REGISTER_NAMES[rs]
                    << ", "
                    << REGISTER_NAMES[rt];
                break;

            case 0x27:
                out << "nor "
                    << REGISTER_NAMES[rd]
                    << ", "
                    << REGISTER_NAMES[rs]
                    << ", "
                    << REGISTER_NAMES[rt];
                break;

            case 0x2A:
                out << "slt "
                    << REGISTER_NAMES[rd]
                    << ", "
                    << REGISTER_NAMES[rs]
                    << ", "
                    << REGISTER_NAMES[rt];
                break;

            case 0x2B:
                out << "sltu "
                    << REGISTER_NAMES[rd]
                    << ", "
                    << REGISTER_NAMES[rs]
                    << ", "
                    << REGISTER_NAMES[rt];
                break;

            case 0x00:
                out << "sll "
                    << REGISTER_NAMES[rd]
                    << ", "
                    << REGISTER_NAMES[rt]
                    << ", "
                    << static_cast<int>(shamt);
                break;

            case 0x02:
                out << "srl "
                    << REGISTER_NAMES[rd]
                    << ", "
                    << REGISTER_NAMES[rt]
                    << ", "
                    << static_cast<int>(shamt);
                break;

            case 0x03:
                out << "sra "
                    << REGISTER_NAMES[rd]
                    << ", "
                    << REGISTER_NAMES[rt]
                    << ", "
                    << static_cast<int>(shamt);
                break;

            case 0x08:
                out << "jr "
                    << REGISTER_NAMES[rs];
                break;

            case 0x18:
                out << "mult "
                    << REGISTER_NAMES[rs]
                    << ", "
                    << REGISTER_NAMES[rt];
                break;

            case 0x10:
                out << "mfhi "
                    << REGISTER_NAMES[rd];
                break;

            case 0x12:
                out << "mflo "
                    << REGISTER_NAMES[rd];
                break;

            case 0x0C:
                out << "syscall";
                break;

            default:
                out << "unknown-r-type funct=0x"
                    << hex
                    << uppercase
                    << static_cast<int>(funct);
                break;
        }

        return out.str();
    }


    // ========================================================
    // I-TYPE / J-TYPE
    // ========================================================

    switch (opcode) {

        case 0x08:

            out << "addi "
                << REGISTER_NAMES[rt]
                << ", "
                << REGISTER_NAMES[rs]
                << ", "
                << dec
                << signedImmediate;

            break;


        case 0x09:

            out << "addiu "
                << REGISTER_NAMES[rt]
                << ", "
                << REGISTER_NAMES[rs]
                << ", "
                << dec
                << signedImmediate;

            break;


        case 0x0C:

            out << "andi "
                << REGISTER_NAMES[rt]
                << ", "
                << REGISTER_NAMES[rs]
                << ", 0x"
                << hex
                << uppercase
                << immediate;

            break;


        case 0x0D:

            out << "ori "
                << REGISTER_NAMES[rt]
                << ", "
                << REGISTER_NAMES[rs]
                << ", 0x"
                << hex
                << uppercase
                << immediate;

            break;


        case 0x0E:

            out << "xori "
                << REGISTER_NAMES[rt]
                << ", "
                << REGISTER_NAMES[rs]
                << ", 0x"
                << hex
                << uppercase
                << immediate;

            break;


        case 0x0A:

            out << "slti "
                << REGISTER_NAMES[rt]
                << ", "
                << REGISTER_NAMES[rs]
                << ", "
                << dec
                << signedImmediate;

            break;


        case 0x0B:

            out << "sltiu "
                << REGISTER_NAMES[rt]
                << ", "
                << REGISTER_NAMES[rs]
                << ", "
                << dec
                << signedImmediate;

            break;


        case 0x0F:

            out << "lui "
                << REGISTER_NAMES[rt]
                << ", 0x"
                << hex
                << uppercase
                << immediate;

            break;


        case 0x23:

            out << "lw "
                << REGISTER_NAMES[rt]
                << ", "
                << dec
                << signedImmediate
                << "("
                << REGISTER_NAMES[rs]
                << ")";

            break;


        case 0x2B:

            out << "sw "
                << REGISTER_NAMES[rt]
                << ", "
                << dec
                << signedImmediate
                << "("
                << REGISTER_NAMES[rs]
                << ")";

            break;


        case 0x04: {

            uint32_t branchTarget =
                address +
                4 +
                (
                    static_cast<int32_t>(
                        signedImmediate
                    ) << 2
                );

            out << "beq "
                << REGISTER_NAMES[rs]
                << ", "
                << REGISTER_NAMES[rt]
                << ", 0x"
                << hex
                << uppercase
                << branchTarget;

            break;
        }


        case 0x05: {

            uint32_t branchTarget =
                address +
                4 +
                (
                    static_cast<int32_t>(
                        signedImmediate
                    ) << 2
                );

            out << "bne "
                << REGISTER_NAMES[rs]
                << ", "
                << REGISTER_NAMES[rt]
                << ", 0x"
                << hex
                << uppercase
                << branchTarget;

            break;
        }


        case 0x02: {

            uint32_t jumpAddress =
                ((address + 4) & 0xF0000000)
                |
                (target << 2);

            out << "j 0x"
                << hex
                << uppercase
                << jumpAddress;

            break;
        }


        case 0x03: {

            uint32_t jumpAddress =
                ((address + 4) & 0xF0000000)
                |
                (target << 2);

            out << "jal 0x"
                << hex
                << uppercase
                << jumpAddress;

            break;
        }


        default:

            out << ".word 0x"
                << hex
                << uppercase
                << instruction;

            break;
    }

    return out.str();
}


// ============================================================
// DISASSEMBLE MEMORY
// ============================================================

void Debugger::disassembleMemory(
    uint32_t address,
    uint32_t count
) {

    cout << hex
         << uppercase
         << setfill('0');

    for (uint32_t i = 0; i < count; i++) {

        uint32_t currentAddress =
            address + (i * 4);

        uint32_t instruction =
            mem.memRead32(
                currentAddress
            );

        cout << "0x"
             << setw(8)
             << currentAddress
             << ": 0x"
             << setw(8)
             << instruction
             << "    "
             << disassemble(
                    instruction,
                    currentAddress
                )
             << '\n';
    }

    cout << dec;
}


// ============================================================
// HELP
// ============================================================

void Debugger::printHelp() {

    cout << "\nCommands:\n\n";

    cout
        << "  step\n"
        << "      Execute one instruction.\n\n";

    cout
        << "  run\n"
        << "      Run until halt or breakpoint.\n\n";

    cout
        << "  reset\n"
        << "      Reset CPU and reload program.\n\n";

    cout
        << "  regs\n"
        << "      Display all registers.\n\n";

    cout
        << "  mem ADDRESS COUNT\n"
        << "      Display COUNT bytes of memory.\n"
        << "      Example: mem 0x1000 20\n\n";

    cout
        << "  break ADDRESS\n"
        << "      Add a breakpoint.\n"
        << "      Example: break 0x20\n\n";

    cout
        << "  breaks\n"
        << "      List all breakpoints.\n\n";

    cout
        << "  delete ADDRESS\n"
        << "      Remove a breakpoint.\n"
        << "      Example: delete 0x20\n\n";

    cout
        << "  disasm ADDRESS COUNT\n"
        << "      Disassemble COUNT instructions.\n"
        << "      Example: disasm 0x0 10\n\n";

    cout
        << "  help\n"
        << "      Show this help screen.\n\n";

    cout
        << "  quit\n"
        << "      Exit debugger.\n\n";
}


// ============================================================
// COMMAND LOOP
// ============================================================

void Debugger::start() {

    string line;

    cout << "\n=====================================\n";
    cout << "         MIPS32 DEBUGGER\n";
    cout << "=====================================\n";

    cout << "Program: "
         << programFile
         << '\n';

    cout << "Type 'help' for commands.\n\n";


    while (true) {

        cout << "mips> ";

        if (!getline(cin, line)) {
            break;
        }

        stringstream ss(line);

        string command;

        ss >> command;

        if (command.empty()) {
            continue;
        }


        // ====================================================
        // STEP
        // ====================================================

        if (
            command == "step" ||
            command == "s"
        ) {

            step();
        }


        // ====================================================
        // RUN
        // ====================================================

        else if (
            command == "run" ||
            command == "r"
        ) {

            run();
        }


        // ====================================================
        // RESET
        // ====================================================

        else if (command == "reset") {

            reset();
        }


        // ====================================================
        // REGISTERS
        // ====================================================

        else if (
            command == "regs" ||
            command == "registers"
        ) {

            printRegisters();
        }


        // ====================================================
        // MEMORY
        // ====================================================

        else if (command == "mem") {

            string addressText;

            uint32_t count = 0;

            ss >> addressText
               >> count;

            if (
                addressText.empty() ||
                count == 0
            ) {

                cout
                    << "Usage: mem ADDRESS COUNT\n";

                continue;
            }

            uint32_t address;

            if (!parseAddress(
                    addressText,
                    address
                )) {

                cout
                    << "Invalid memory address.\n";

                continue;
            }

            printMemory(
                address,
                count
            );
        }


        // ====================================================
        // BREAK
        // ====================================================

        else if (
            command == "break" ||
            command == "b"
        ) {

            string addressText;

            ss >> addressText;

            if (addressText.empty()) {

                cout
                    << "Usage: break ADDRESS\n";

                continue;
            }

            uint32_t address;

            if (!parseAddress(
                    addressText,
                    address
                )) {

                cout
                    << "Invalid breakpoint address.\n";

                continue;
            }

            addBreakpoint(address);
        }


        // ====================================================
        // BREAKS
        // ====================================================

        else if (command == "breaks") {

            printBreakpoints();
        }


        // ====================================================
        // DELETE
        // ====================================================

        else if (
            command == "delete" ||
            command == "del"
        ) {

            string addressText;

            ss >> addressText;

            if (addressText.empty()) {

                cout
                    << "Usage: delete ADDRESS\n";

                continue;
            }

            uint32_t address;

            if (!parseAddress(
                    addressText,
                    address
                )) {

                cout
                    << "Invalid breakpoint address.\n";

                continue;
            }

            removeBreakpoint(address);
        }


        // ====================================================
        // DISASSEMBLE
        // ====================================================

        else if (
            command == "disasm" ||
            command == "dis"
        ) {

            string addressText;

            uint32_t count = 0;

            ss >> addressText
               >> count;

            if (
                addressText.empty() ||
                count == 0
            ) {

                cout
                    << "Usage: disasm ADDRESS COUNT\n";

                continue;
            }

            uint32_t address;

            if (!parseAddress(
                    addressText,
                    address
                )) {

                cout
                    << "Invalid address.\n";

                continue;
            }

            disassembleMemory(
                address,
                count
            );
        }


        // ====================================================
        // HELP
        // ====================================================

        else if (
            command == "help" ||
            command == "h"
        ) {

            printHelp();
        }


        // ====================================================
        // QUIT
        // ====================================================

        else if (
            command == "quit" ||
            command == "exit" ||
            command == "q"
        ) {

            cout << "Debugger closed.\n";
            break;
        }


        // ====================================================
        // UNKNOWN
        // ====================================================

        else {

            cout << "Unknown command: "
                 << command
                 << '\n';

            cout
                << "Type 'help' for commands.\n";
        }
    }
}