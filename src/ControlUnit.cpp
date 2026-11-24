#include "ControlUnit.h"


ControlSignals ControlUnit::generateSignals(
    instructionType instruction
)
{
    ControlSignals signals{};


    switch (
        instruction
    ) {

        // ====================================================
        // REGISTER DESTINATION
        // ====================================================

        case instructionType::ADD:
        case instructionType::ADDU:

        case instructionType::SUB:
        case instructionType::SUBU:

        case instructionType::AND:
        case instructionType::OR:
        case instructionType::XOR:
        case instructionType::NOR:

        case instructionType::SLT:
        case instructionType::SLTU:

        case instructionType::SLL:
        case instructionType::SRL:
        case instructionType::SRA:

        case instructionType::SLLV:
        case instructionType::SRLV:
        case instructionType::SRAV:

        case instructionType::MUL:

        case instructionType::MFHI:
        case instructionType::MFLO:

        case instructionType::MOVZ:
        case instructionType::MOVN:

        case instructionType::CLZ:
        case instructionType::CLO:

            signals.RegDst =
                true;

            signals.RegWrite =
                true;

            signals.ALUOp =
                2;

            break;


        // ====================================================
        // IMMEDIATE
        // ====================================================

        case instructionType::ADDI:
        case instructionType::ADDIU:

        case instructionType::ANDI:
        case instructionType::ORI:
        case instructionType::XORI:

        case instructionType::SLTI:
        case instructionType::SLTIU:

        case instructionType::LUI:

            signals.ALUSrc =
                true;

            signals.RegWrite =
                true;

            break;


        // ====================================================
        // LOAD
        // ====================================================

        case instructionType::LB:
        case instructionType::LBU:

        case instructionType::LH:
        case instructionType::LHU:

        case instructionType::LW:

            signals.ALUSrc =
                true;

            signals.MemRead =
                true;

            signals.MemtoReg =
                true;

            signals.RegWrite =
                true;

            break;


        // ====================================================
        // STORE
        // ====================================================

        case instructionType::SB:
        case instructionType::SH:
        case instructionType::SW:

            signals.ALUSrc =
                true;

            signals.MemWrite =
                true;

            break;


        // ====================================================
        // BRANCH
        // ====================================================

        case instructionType::BEQ:
        case instructionType::BNE:

        case instructionType::BLEZ:
        case instructionType::BGTZ:

        case instructionType::BLTZ:
        case instructionType::BGEZ:

        case instructionType::BLTZAL:
        case instructionType::BGEZAL:

            signals.Branch =
                true;

            break;


        default:
            break;
    }


    return signals;
}


// ============================================================
// ALU OPERATION
// ============================================================

ALUOperation ControlUnit::getALUOperation(
    instructionType instruction
)
{
    switch (
        instruction
    ) {

        case instructionType::ADD:
        case instructionType::ADDI:

        case instructionType::LB:
        case instructionType::LBU:

        case instructionType::LH:
        case instructionType::LHU:

        case instructionType::LW:

        case instructionType::SB:
        case instructionType::SH:
        case instructionType::SW:

            return ALUOperation::ADD;


        case instructionType::ADDU:
        case instructionType::ADDIU:

            return ALUOperation::ADDU;


        case instructionType::SUB:
        case instructionType::BEQ:
        case instructionType::BNE:

            return ALUOperation::SUB;


        case instructionType::SUBU:

            return ALUOperation::SUBU;


        case instructionType::AND:
        case instructionType::ANDI:

            return ALUOperation::AND;


        case instructionType::OR:
        case instructionType::ORI:

            return ALUOperation::OR;


        case instructionType::XOR:
        case instructionType::XORI:

            return ALUOperation::XOR;


        case instructionType::NOR:

            return ALUOperation::NOR;


        case instructionType::SLT:
        case instructionType::SLTI:

            return ALUOperation::SLT;


        case instructionType::SLTU:
        case instructionType::SLTIU:

            return ALUOperation::SLTU;


        case instructionType::SLL:
        case instructionType::SLLV:

            return ALUOperation::SLL;


        case instructionType::SRL:
        case instructionType::SRLV:

            return ALUOperation::SRL;


        case instructionType::SRA:
        case instructionType::SRAV:

            return ALUOperation::SRA;


        case instructionType::MUL:

            return ALUOperation::MUL;


        default:

            return ALUOperation::NONE;
    }
}