#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;


struct Memory;


enum class Section {
    TEXT,
    DATA
};


// ============================================================
// TWO-PASS MIPS32 ASSEMBLER
// ------------------------------------------------------------
// assembleInstruction():
//     Encodes one REAL machine instruction.
//
// expandInstruction():
//     Expands assembler pseudo-instructions.
//
// instructionSize():
//     Must exactly mirror pseudo expansion so labels and
//     branches get correct addresses during the first pass.
// ============================================================

struct Assembler {

public:
    bool assembleFile(
        const string& filename,
        Memory& mem,
        uint32_t startAddress = 0
    );


private:
    static constexpr uint32_t TEXT_START =
        0x00000000;


    static constexpr uint32_t DATA_START =
        0x00001000;


    bool assemblyError =
        false;


    int currentLineNumber =
        0;


    string currentLineText;


    unordered_map<string, uint32_t>
        labels;


    // ========================================================
    // TOKENIZATION / TEXT
    // ========================================================

    vector<string> tokenize(
        const string& line
    );


    string lowerCopy(
        const string& text
    ) const;


    // ========================================================
    // REGISTERS
    // ========================================================

    int registerNumber(
        const string& reg
    );


    bool validRegister(
        int reg
    );


    // ========================================================
    // VALIDATION
    // ========================================================

    bool expectTokenCount(
        const vector<string>& tokens,
        size_t expected,
        const string& instructionName
    );


    bool expectTokenCountEither(
        const vector<string>& tokens,
        size_t first,
        size_t second,
        const string& instructionName
    );


    bool parseInt16(
        const string& text,
        int16_t& value
    );


    bool parseUint16(
        const string& text,
        uint16_t& value
    );


    bool parseInt32(
        const string& text,
        int32_t& value
    );


    bool parseUint32(
        const string& text,
        uint32_t& value
    );


    bool parseAsciiz(
        const string& line,
        string& value
    );


    bool parseBranchTarget(
        const string& text,
        uint32_t currentAddress,
        int16_t& immediate,
        const string& instructionName
    );


    bool parseJumpTarget(
        const string& text,
        uint32_t& targetAddress,
        const string& instructionName
    );


    void reportError(
        const string& message
    );


    // ========================================================
    // PASSES / SIZING
    // ========================================================

    bool firstPass(
        const string& filename,
        uint32_t startAddress
    );


    uint32_t instructionSize(
        const vector<string>& tokens
    );


    // ========================================================
    // ENCODING
    // ========================================================

    uint32_t encodeR(
        uint8_t rs,
        uint8_t rt,
        uint8_t rd,
        uint8_t shamt,
        uint8_t funct
    );


    uint32_t encodeI(
        uint8_t opcode,
        uint8_t rs,
        uint8_t rt,
        int16_t immediate
    );


    uint32_t encodeJ(
        uint8_t opcode,
        uint32_t addressField
    );


    uint32_t encodeSpecial2(
        uint8_t rs,
        uint8_t rt,
        uint8_t rd,
        uint8_t shamt,
        uint8_t funct
    );


    // ========================================================
    // ASSEMBLY
    // ========================================================

    vector<uint32_t> expandInstruction(
        const vector<string>& tokens,
        uint32_t currentAddress
    );


    uint32_t assembleInstruction(
        const vector<string>& tokens,
        uint32_t currentAddress
    );
};


#endif