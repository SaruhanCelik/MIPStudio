#include "CPU.h"

#include "ALU.h"
#include "ControlUnit.h"
#include "DataMemory.h"

#include <climits>
#include <cstdint>
#include <iomanip>
#include <iostream>

using namespace std;


// ============================================================
// CONSTRUCTOR
// ============================================================

CPU::CPU()
{
    for (
        int i = 0;
        i < 32;
        ++i
    ) {
        registers[i] = 0;
    }


    pc = 0;

    hi = 0;
    lo = 0;

    ir = 0;

    halted = false;
}


// ============================================================
// SIGN EXTEND
// ============================================================

uint32_t CPU::signExtend16(
    uint16_t value
)
{
    return static_cast<uint32_t>(
        static_cast<int32_t>(
            static_cast<int16_t>(
                value
            )
        )
    );
}


// ============================================================
// EXECUTE
// ------------------------------------------------------------
// IMPORTANT:
//
// MIPS Studio currently uses NO architectural branch-delay
// slot.
//
// fetch() advances PC by four first.
//
// Therefore:
//
// branch target = current PC + offset * 4
//
// JAL/JALR link = current PC
//
// where "current PC" already points at the next instruction.
// ============================================================

void CPU::executeInstruction(
    Memory& memory,
    ControlUnit& controlUnit,
    ALU& alu
)
{
    // The ControlUnit remains part of the public emulator
    // architecture. Execution is instruction-specific now,
    // therefore this function doesn't need its signals directly.

    (void)controlUnit;


    if (
        halted
    ) {
        return;
    }


    fetch(
        memory
    );


    Instruction instruction =
        decode();


    instructionType type =
        decodeInstruction();


    const uint32_t rsValue =
        registers[
            instruction.rs
        ];


    const uint32_t rtValue =
        registers[
            instruction.rt
        ];


    // --------------------------------------------------------
    // Branch helper
    // --------------------------------------------------------

    auto branch =
        [&](
            bool condition
        )
        {
            if (!condition) {
                return;
            }


            int32_t offset =
                static_cast<int32_t>(
                    static_cast<int16_t>(
                        instruction.imm
                    )
                );


            pc =
                static_cast<uint32_t>(
                    static_cast<int64_t>(
                        pc
                    )
                    +
                    (
                        static_cast<int64_t>(
                            offset
                        )
                        *
                        4
                    )
                );
        };


    // --------------------------------------------------------
    // Overflow helper
    // --------------------------------------------------------

    auto arithmeticOverflow =
        [&]()
        {
            cout
                << "Arithmetic overflow at instruction 0x"
                << hex
                << uppercase
                << ir
                << dec
                << '\n';


            halted =
                true;
        };


    switch (
        type
    ) {

        // ====================================================
        // ADD
        // ====================================================

        case instructionType::ADD: {
            ALUResult result =
                alu.execute(
                    rsValue,
                    rtValue,
                    ALUOperation::ADD
                );


            if (
                result.overflow
            ) {
                arithmeticOverflow();

                return;
            }


            regWrite(
                instruction.rd,
                result.value
            );


            return;
        }


        // ====================================================
        // ADDU
        // ====================================================

        case instructionType::ADDU:

            regWrite(
                instruction.rd,
                rsValue +
                rtValue
            );

            return;


        // ====================================================
        // ADDI
        // ====================================================

        case instructionType::ADDI: {
            uint32_t immediate =
                signExtend16(
                    instruction.imm
                );


            ALUResult result =
                alu.execute(
                    rsValue,
                    immediate,
                    ALUOperation::ADD
                );


            if (
                result.overflow
            ) {
                arithmeticOverflow();

                return;
            }


            regWrite(
                instruction.rt,
                result.value
            );


            return;
        }


        // ====================================================
        // ADDIU
        // ====================================================

        case instructionType::ADDIU:

            regWrite(
                instruction.rt,

                rsValue +
                signExtend16(
                    instruction.imm
                )
            );

            return;


        // ====================================================
        // SUB
        // ====================================================

        case instructionType::SUB: {
            int64_t result =
                static_cast<int64_t>(
                    static_cast<int32_t>(
                        rsValue
                    )
                )
                -
                static_cast<int64_t>(
                    static_cast<int32_t>(
                        rtValue
                    )
                );


            if (
                result < INT32_MIN ||
                result > INT32_MAX
            ) {
                arithmeticOverflow();

                return;
            }


            regWrite(
                instruction.rd,

                static_cast<uint32_t>(
                    static_cast<int32_t>(
                        result
                    )
                )
            );


            return;
        }


        // ====================================================
        // SUBU
        // ====================================================

        case instructionType::SUBU:

            regWrite(
                instruction.rd,
                rsValue -
                rtValue
            );

            return;


        // ====================================================
        // LOGICAL
        // ====================================================

        case instructionType::AND:

            regWrite(
                instruction.rd,
                rsValue &
                rtValue
            );

            return;


        case instructionType::ANDI:

            regWrite(
                instruction.rt,

                rsValue &
                static_cast<uint32_t>(
                    instruction.imm
                )
            );

            return;


        case instructionType::OR:

            regWrite(
                instruction.rd,
                rsValue |
                rtValue
            );

            return;


        case instructionType::ORI:

            regWrite(
                instruction.rt,

                rsValue |
                static_cast<uint32_t>(
                    instruction.imm
                )
            );

            return;


        case instructionType::XOR:

            regWrite(
                instruction.rd,
                rsValue ^
                rtValue
            );

            return;


        case instructionType::XORI:

            regWrite(
                instruction.rt,

                rsValue ^
                static_cast<uint32_t>(
                    instruction.imm
                )
            );

            return;


        case instructionType::NOR:

            regWrite(
                instruction.rd,
                ~(
                    rsValue |
                    rtValue
                )
            );

            return;


        // ====================================================
        // COMPARISONS
        // ====================================================

        case instructionType::SLT:

            regWrite(
                instruction.rd,

                static_cast<int32_t>(
                    rsValue
                )
                <
                static_cast<int32_t>(
                    rtValue
                )
                ? 1U
                : 0U
            );

            return;


        case instructionType::SLTU:

            regWrite(
                instruction.rd,

                rsValue <
                rtValue
                ? 1U
                : 0U
            );

            return;


        case instructionType::SLTI:

            regWrite(
                instruction.rt,

                static_cast<int32_t>(
                    rsValue
                )
                <
                static_cast<int32_t>(
                    static_cast<int16_t>(
                        instruction.imm
                    )
                )
                ? 1U
                : 0U
            );

            return;


        case instructionType::SLTIU:

            regWrite(
                instruction.rt,

                rsValue <
                signExtend16(
                    instruction.imm
                )
                ? 1U
                : 0U
            );

            return;


        // ====================================================
        // FIXED SHIFTS
        // ====================================================

        case instructionType::SLL:

            regWrite(
                instruction.rd,

                rtValue <<
                instruction.shamt
            );

            return;


        case instructionType::SRL:

            regWrite(
                instruction.rd,

                rtValue >>
                instruction.shamt
            );

            return;


        case instructionType::SRA:

            regWrite(
                instruction.rd,

                static_cast<uint32_t>(
                    static_cast<int32_t>(
                        rtValue
                    )
                    >>
                    instruction.shamt
                )
            );

            return;


        // ====================================================
        // VARIABLE SHIFTS
        // ====================================================

        case instructionType::SLLV:

            regWrite(
                instruction.rd,

                rtValue <<
                (
                    rsValue &
                    0x1FU
                )
            );

            return;


        case instructionType::SRLV:

            regWrite(
                instruction.rd,

                rtValue >>
                (
                    rsValue &
                    0x1FU
                )
            );

            return;


        case instructionType::SRAV:

            regWrite(
                instruction.rd,

                static_cast<uint32_t>(
                    static_cast<int32_t>(
                        rtValue
                    )
                    >>
                    (
                        rsValue &
                        0x1FU
                    )
                )
            );

            return;


        // ====================================================
        // MULT
        // ====================================================

        case instructionType::MULT: {
            int64_t product =
                static_cast<int64_t>(
                    static_cast<int32_t>(
                        rsValue
                    )
                )
                *
                static_cast<int64_t>(
                    static_cast<int32_t>(
                        rtValue
                    )
                );


            uint64_t bits =
                static_cast<uint64_t>(
                    product
                );


            lo =
                static_cast<uint32_t>(
                    bits &
                    0xFFFFFFFFULL
                );


            hi =
                static_cast<uint32_t>(
                    bits >>
                    32
                );


            return;
        }


        // ====================================================
        // MULTU
        // ====================================================

        case instructionType::MULTU: {
            uint64_t product =
                static_cast<uint64_t>(
                    rsValue
                )
                *
                static_cast<uint64_t>(
                    rtValue
                );


            lo =
                static_cast<uint32_t>(
                    product &
                    0xFFFFFFFFULL
                );


            hi =
                static_cast<uint32_t>(
                    product >>
                    32
                );


            return;
        }


        // ====================================================
        // DIV
        // ====================================================

        case instructionType::DIV: {
            int32_t dividend =
                static_cast<int32_t>(
                    rsValue
                );


            int32_t divisor =
                static_cast<int32_t>(
                    rtValue
                );


            // Real MIPS leaves the result unpredictable here.
            //
            // We preserve HI/LO so the emulator stays safe and
            // deterministic.

            if (
                divisor ==
                0
            ) {
                cout
                    << "Division by zero: HI/LO unchanged.\n";

                return;
            }


            // Avoid C++ signed division overflow.

            if (
                dividend ==
                    INT32_MIN &&
                divisor ==
                    -1
            ) {
                lo =
                    static_cast<uint32_t>(
                        INT32_MIN
                    );


                hi =
                    0;


                return;
            }


            lo =
                static_cast<uint32_t>(
                    dividend /
                    divisor
                );


            hi =
                static_cast<uint32_t>(
                    dividend %
                    divisor
                );


            return;
        }


        // ====================================================
        // DIVU
        // ====================================================

        case instructionType::DIVU:

            if (
                rtValue ==
                0
            ) {
                cout
                    << "Division by zero: HI/LO unchanged.\n";

                return;
            }


            lo =
                rsValue /
                rtValue;


            hi =
                rsValue %
                rtValue;


            return;


        // ====================================================
        // MUL
        // ----------------------------------------------------
        // Unlike MULT, MUL writes the low 32 bits directly to
        // rd.
        // ====================================================

        case instructionType::MUL: {
            int64_t product =
                static_cast<int64_t>(
                    static_cast<int32_t>(
                        rsValue
                    )
                )
                *
                static_cast<int64_t>(
                    static_cast<int32_t>(
                        rtValue
                    )
                );


            regWrite(
                instruction.rd,

                static_cast<uint32_t>(
                    static_cast<uint64_t>(
                        product
                    )
                    &
                    0xFFFFFFFFULL
                )
            );


            return;
        }


        // ====================================================
        // HI / LO MOVES
        // ====================================================

        case instructionType::MFHI:

            regWrite(
                instruction.rd,
                hi
            );

            return;


        case instructionType::MFLO:

            regWrite(
                instruction.rd,
                lo
            );

            return;


        case instructionType::MTHI:

            hi =
                rsValue;

            return;


        case instructionType::MTLO:

            lo =
                rsValue;

            return;


        // ====================================================
        // CONDITIONAL MOVES
        // ====================================================

        case instructionType::MOVZ:

            if (
                rtValue ==
                0
            ) {
                regWrite(
                    instruction.rd,
                    rsValue
                );
            }

            return;


        case instructionType::MOVN:

            if (
                rtValue !=
                0
            ) {
                regWrite(
                    instruction.rd,
                    rsValue
                );
            }

            return;


        // ====================================================
        // CLZ
        // ====================================================

        case instructionType::CLZ: {
            uint32_t count =
                0;


            for (
                int bit = 31;
                bit >= 0;
                --bit
            ) {
                if (
                    (
                        rsValue &
                        (
                            1U <<
                            bit
                        )
                    )
                    != 0
                ) {
                    break;
                }


                ++count;
            }


            regWrite(
                instruction.rd,
                count
            );


            return;
        }


        // ====================================================
        // CLO
        // ====================================================

        case instructionType::CLO: {
            uint32_t count =
                0;


            for (
                int bit = 31;
                bit >= 0;
                --bit
            ) {
                if (
                    (
                        rsValue &
                        (
                            1U <<
                            bit
                        )
                    )
                    ==
                    0
                ) {
                    break;
                }


                ++count;
            }


            regWrite(
                instruction.rd,
                count
            );


            return;
        }


        // ====================================================
        // LUI
        // ====================================================

        case instructionType::LUI:

            regWrite(
                instruction.rt,

                static_cast<uint32_t>(
                    instruction.imm
                )
                <<
                16
            );

            return;


        // ====================================================
        // BYTE LOAD
        // ====================================================

        case instructionType::LB: {
            uint32_t address =
                rsValue +
                signExtend16(
                    instruction.imm
                );


            int8_t value =
                static_cast<int8_t>(
                    memory.memReadByte(
                        address
                    )
                );


            regWrite(
                instruction.rt,

                static_cast<uint32_t>(
                    static_cast<int32_t>(
                        value
                    )
                )
            );


            return;
        }


        case instructionType::LBU: {
            uint32_t address =
                rsValue +
                signExtend16(
                    instruction.imm
                );


            regWrite(
                instruction.rt,

                memory.memReadByte(
                    address
                )
            );


            return;
        }


        // ====================================================
        // HALFWORD LOAD
        // ====================================================

        case instructionType::LH: {
            uint32_t address =
                rsValue +
                signExtend16(
                    instruction.imm
                );


            int16_t value =
                static_cast<int16_t>(
                    memory.memRead16(
                        address
                    )
                );


            regWrite(
                instruction.rt,

                static_cast<uint32_t>(
                    static_cast<int32_t>(
                        value
                    )
                )
            );


            return;
        }


        case instructionType::LHU: {
            uint32_t address =
                rsValue +
                signExtend16(
                    instruction.imm
                );


            regWrite(
                instruction.rt,

                memory.memRead16(
                    address
                )
            );


            return;
        }


        // ====================================================
        // WORD LOAD
        // ====================================================

        case instructionType::LW: {
            uint32_t address =
                rsValue +
                signExtend16(
                    instruction.imm
                );


            regWrite(
                instruction.rt,

                memory.memRead32(
                    address
                )
            );


            return;
        }


        // ====================================================
        // BYTE STORE
        // ====================================================

        case instructionType::SB: {
            uint32_t address =
                rsValue +
                signExtend16(
                    instruction.imm
                );


            memory.memWriteByte(
                address,

                static_cast<uint8_t>(
                    rtValue &
                    0xFFU
                )
            );


            return;
        }


        // ====================================================
        // HALFWORD STORE
        // ====================================================

        case instructionType::SH: {
            uint32_t address =
                rsValue +
                signExtend16(
                    instruction.imm
                );


            memory.memWrite16(
                address,

                static_cast<uint16_t>(
                    rtValue &
                    0xFFFFU
                )
            );


            return;
        }


        // ====================================================
        // WORD STORE
        // ====================================================

        case instructionType::SW: {
            uint32_t address =
                rsValue +
                signExtend16(
                    instruction.imm
                );


            memory.memWrite32(
                address,
                rtValue
            );


            return;
        }


        // ====================================================
        // BRANCHES
        // ====================================================

        case instructionType::BEQ:

            branch(
                rsValue ==
                rtValue
            );

            return;


        case instructionType::BNE:

            branch(
                rsValue !=
                rtValue
            );

            return;


        case instructionType::BLEZ:

            branch(
                static_cast<int32_t>(
                    rsValue
                )
                <=
                0
            );

            return;


        case instructionType::BGTZ:

            branch(
                static_cast<int32_t>(
                    rsValue
                )
                >
                0
            );

            return;


        case instructionType::BLTZ:

            branch(
                static_cast<int32_t>(
                    rsValue
                )
                <
                0
            );

            return;


        case instructionType::BGEZ:

            branch(
                static_cast<int32_t>(
                    rsValue
                )
                >=
                0
            );

            return;


        // ====================================================
        // BRANCH AND LINK
        // ====================================================

        case instructionType::BLTZAL:

            if (
                static_cast<int32_t>(
                    rsValue
                )
                <
                0
            ) {
                regWrite(
                    31,
                    pc
                );


                branch(
                    true
                );
            }

            return;


        case instructionType::BGEZAL:

            if (
                static_cast<int32_t>(
                    rsValue
                )
                >=
                0
            ) {
                regWrite(
                    31,
                    pc
                );


                branch(
                    true
                );
            }

            return;


        // ====================================================
        // J
        // ====================================================

        case instructionType::J:

            pc =
                (
                    pc &
                    0xF0000000U
                )
                |
                (
                    instruction.jaddr <<
                    2
                );

            return;


        // ====================================================
        // JAL
        // ====================================================

        case instructionType::JAL:

            regWrite(
                31,
                pc
            );


            pc =
                (
                    pc &
                    0xF0000000U
                )
                |
                (
                    instruction.jaddr <<
                    2
                );

            return;


        // ====================================================
        // JR
        // ====================================================

        case instructionType::JR:

            pc =
                rsValue;

            return;


        // ====================================================
        // JALR
        // ====================================================

        case instructionType::JALR: {
            // Important if rd happens to equal rs:
            // save target BEFORE writing the link register.

            uint32_t target =
                rsValue;


            regWrite(
                instruction.rd,
                pc
            );


            pc =
                target;


            return;
        }


        // ====================================================
        // SYSCALL
        // ====================================================

        case instructionType::SYSCALL: {
            uint32_t syscallCode =
                registers[2];


            switch (
                syscallCode
            ) {

                // print integer
                case 1:

                    cout
                        << dec
                        << static_cast<int32_t>(
                            registers[4]
                        );

                    break;


                // print string
                case 4: {
                    uint32_t address =
                        registers[4];


                    while (
                        true
                    ) {
                        uint8_t c =
                            memory.memReadByte(
                                address++
                            );


                        if (
                            c ==
                            0
                        ) {
                            break;
                        }


                        cout
                            << static_cast<char>(
                                c
                            );
                    }


                    break;
                }


                // exit
                case 10:

                    halted =
                        true;

                    break;


                default:

                    cout
                        << "Unsupported syscall: "
                        << syscallCode
                        << '\n';


                    halted =
                        true;

                    break;
            }


            return;
        }


        // ====================================================
        // BREAK
        // ----------------------------------------------------
        // Real MIPS raises a Breakpoint exception.
        //
        // We don't have an exception subsystem yet, so BREAK
        // currently halts the emulator cleanly.
        // ====================================================

        case instructionType::BREAK:

            halted =
                true;

            return;


        // ====================================================
        // UNKNOWN
        // ----------------------------------------------------
        // Preserve historical emulator behavior:
        // unknown machine words act as no-op-like instructions.
        //
        // This also keeps all old integration tests compatible.
        // ====================================================

        case instructionType::UNKNOWN:
        default:

            return;
    }
}


// ============================================================
// FETCH
// ============================================================

void CPU::fetch(
    Memory& memory
)
{
    ir =
        memory.memRead32(
            pc
        );


    pc +=
        4;
}


// ============================================================
// REGISTER WRITE
// ============================================================

void CPU::regWrite(
    int regIndex,
    uint32_t value
)
{
    // $zero is immutable.

    if (
        regIndex ==
        0
    ) {
        return;
    }


    if (
        regIndex > 0 &&
        regIndex < 32
    ) {
        registers[
            regIndex
        ] = value;


        return;
    }


    cout
        << "Register index out of range!\n";
}


// ============================================================
// PRINT REGISTER
// ============================================================

void CPU::printRegisterValue(
    int regIndex
)
{
    if (
        regIndex >= 0 &&
        regIndex < 32
    ) {
        cout
            << "reg"
            << regIndex
            << "       0x"
            << uppercase
            << hex
            << setw(8)
            << setfill('0')
            << registers[
                   regIndex
               ]
            << dec
            << setfill(' ')
            << endl;
    }
    else {
        cout
            << "Index you've entered is not in range!"
            << endl;
    }
}


// ============================================================
// FIELD EXTRACTION
// ============================================================

uint8_t CPU::getOpcode()
{
    return static_cast<uint8_t>(
        (
            ir >>
            26
        )
        &
        0x3F
    );
}


uint8_t CPU::getrs()
{
    return static_cast<uint8_t>(
        (
            ir >>
            21
        )
        &
        0x1F
    );
}


uint8_t CPU::getrt()
{
    return static_cast<uint8_t>(
        (
            ir >>
            16
        )
        &
        0x1F
    );
}


uint8_t CPU::getrd()
{
    return static_cast<uint8_t>(
        (
            ir >>
            11
        )
        &
        0x1F
    );
}


uint8_t CPU::getshamt()
{
    return static_cast<uint8_t>(
        (
            ir >>
            6
        )
        &
        0x1F
    );
}


uint8_t CPU::getfunct()
{
    return static_cast<uint8_t>(
        ir &
        0x3F
    );
}


uint16_t CPU::getimm()
{
    return static_cast<uint16_t>(
        ir &
        0xFFFF
    );
}


uint32_t CPU::getJaddr()
{
    return
        ir &
        0x03FFFFFFU;
}


void CPU::testgetimm()
{
    cout
        << "reg"
        << pc
        << "       0x"
        << uppercase
        << hex
        << setw(4)
        << setfill('0')
        << getimm()
        << dec
        << setfill(' ')
        << endl;
}


// ============================================================
// DECODE FIELDS
// ============================================================

Instruction CPU::decode()
{
    Instruction instruction{};


    instruction.opcode =
        getOpcode();


    instruction.rs =
        getrs();


    instruction.rt =
        getrt();


    instruction.rd =
        getrd();


    instruction.shamt =
        getshamt();


    instruction.funct =
        getfunct();


    instruction.imm =
        getimm();


    instruction.jaddr =
        getJaddr();


    return instruction;
}


// ============================================================
// DECODE INSTRUCTION TYPE
// ============================================================

instructionType CPU::decodeInstruction()
{
    uint8_t opcode =
        getOpcode();


    switch (
        opcode
    ) {

        // ====================================================
        // SPECIAL
        // ====================================================

        case 0x00:

            switch (
                getfunct()
            ) {
                case 0x00:
                    return instructionType::SLL;

                case 0x02:
                    return instructionType::SRL;

                case 0x03:
                    return instructionType::SRA;

                case 0x04:
                    return instructionType::SLLV;

                case 0x06:
                    return instructionType::SRLV;

                case 0x07:
                    return instructionType::SRAV;

                case 0x08:
                    return instructionType::JR;

                case 0x09:
                    return instructionType::JALR;

                case 0x0A:
                    return instructionType::MOVZ;

                case 0x0B:
                    return instructionType::MOVN;

                case 0x0C:
                    return instructionType::SYSCALL;

                case 0x0D:
                    return instructionType::BREAK;

                case 0x10:
                    return instructionType::MFHI;

                case 0x11:
                    return instructionType::MTHI;

                case 0x12:
                    return instructionType::MFLO;

                case 0x13:
                    return instructionType::MTLO;

                case 0x18:
                    return instructionType::MULT;

                case 0x19:
                    return instructionType::MULTU;

                case 0x1A:
                    return instructionType::DIV;

                case 0x1B:
                    return instructionType::DIVU;

                case 0x20:
                    return instructionType::ADD;

                case 0x21:
                    return instructionType::ADDU;

                case 0x22:
                    return instructionType::SUB;

                case 0x23:
                    return instructionType::SUBU;

                case 0x24:
                    return instructionType::AND;

                case 0x25:
                    return instructionType::OR;

                case 0x26:
                    return instructionType::XOR;

                case 0x27:
                    return instructionType::NOR;

                case 0x2A:
                    return instructionType::SLT;

                case 0x2B:
                    return instructionType::SLTU;

                default:
                    return instructionType::UNKNOWN;
            }


        // ====================================================
        // REGIMM
        // ====================================================

        case 0x01:

            switch (
                getrt()
            ) {
                case 0x00:
                    return instructionType::BLTZ;

                case 0x01:
                    return instructionType::BGEZ;

                case 0x10:
                    return instructionType::BLTZAL;

                case 0x11:
                    return instructionType::BGEZAL;

                default:
                    return instructionType::UNKNOWN;
            }


        // ====================================================
        // NORMAL OPCODES
        // ====================================================

        case 0x02:
            return instructionType::J;

        case 0x03:
            return instructionType::JAL;

        case 0x04:
            return instructionType::BEQ;

        case 0x05:
            return instructionType::BNE;

        case 0x06:
            return instructionType::BLEZ;

        case 0x07:
            return instructionType::BGTZ;

        case 0x08:
            return instructionType::ADDI;

        case 0x09:
            return instructionType::ADDIU;

        case 0x0A:
            return instructionType::SLTI;

        case 0x0B:
            return instructionType::SLTIU;

        case 0x0C:
            return instructionType::ANDI;

        case 0x0D:
            return instructionType::ORI;

        case 0x0E:
            return instructionType::XORI;

        case 0x0F:
            return instructionType::LUI;


        // ====================================================
        // SPECIAL2
        // ====================================================

        case 0x1C:

            switch (
                getfunct()
            ) {
                case 0x02:
                    return instructionType::MUL;

                case 0x20:
                    return instructionType::CLZ;

                case 0x21:
                    return instructionType::CLO;

                default:
                    return instructionType::UNKNOWN;
            }


        // ====================================================
        // LOADS
        // ====================================================

        case 0x20:
            return instructionType::LB;

        case 0x21:
            return instructionType::LH;

        case 0x23:
            return instructionType::LW;

        case 0x24:
            return instructionType::LBU;

        case 0x25:
            return instructionType::LHU;


        // ====================================================
        // STORES
        // ====================================================

        case 0x28:
            return instructionType::SB;

        case 0x29:
            return instructionType::SH;

        case 0x2B:
            return instructionType::SW;


        default:
            return instructionType::UNKNOWN;
    }
}