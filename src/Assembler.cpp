#include "Assembler.h"
#include "DataMemory.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;


namespace {

bool removeLeadingLabel(
    vector<string>& tokens,
    string* label = nullptr
) {
    if (
        tokens.empty() ||
        tokens[0].empty() ||
        tokens[0].back() != ':'
    ) {
        return false;
    }


    if (label) {
        *label =
            tokens[0].substr(
                0,
                tokens[0].size() - 1
            );
    }


    tokens.erase(
        tokens.begin()
    );


    return true;
}

}


// ============================================================
// TOKENIZATION
// ============================================================

vector<string> Assembler::tokenize(
    const string& line
)
{
    string cleaned;

    cleaned.reserve(
        line.size()
    );


    bool insideString =
        false;


    bool escaped =
        false;


    for (
        char c :
        line
    ) {
        if (
            c == '"' &&
            !escaped
        ) {
            insideString =
                !insideString;
        }


        if (
            c == '#' &&
            !insideString
        ) {
            break;
        }


        if (
            !insideString &&
            (
                c == ',' ||
                c == '(' ||
                c == ')'
            )
        ) {
            cleaned.push_back(
                ' '
            );
        }
        else {
            cleaned.push_back(
                c
            );
        }


        if (
            insideString &&
            c == '\\' &&
            !escaped
        ) {
            escaped = true;
        }
        else {
            escaped = false;
        }
    }


    stringstream stream(
        cleaned
    );


    vector<string> tokens;

    string token;


    while (
        stream >>
        token
    ) {
        tokens.push_back(
            token
        );
    }


    return tokens;
}


// ============================================================
// LOWER CASE
// ============================================================

string Assembler::lowerCopy(
    const string& text
) const
{
    string result =
        text;


    transform(
        result.begin(),
        result.end(),
        result.begin(),

        [](
            unsigned char c
        ) {
            return static_cast<char>(
                tolower(c)
            );
        }
    );


    return result;
}


// ============================================================
// REGISTERS
// ============================================================

bool Assembler::validRegister(
    int reg
)
{
    return
        reg >= 0 &&
        reg < 32;
}


int Assembler::registerNumber(
    const string& registerText
)
{
    const string reg =
        lowerCopy(
            registerText
        );


    if (reg == "$zero") return 0;
    if (reg == "$at") return 1;

    if (reg == "$v0") return 2;
    if (reg == "$v1") return 3;

    if (reg == "$a0") return 4;
    if (reg == "$a1") return 5;
    if (reg == "$a2") return 6;
    if (reg == "$a3") return 7;

    if (reg == "$t0") return 8;
    if (reg == "$t1") return 9;
    if (reg == "$t2") return 10;
    if (reg == "$t3") return 11;
    if (reg == "$t4") return 12;
    if (reg == "$t5") return 13;
    if (reg == "$t6") return 14;
    if (reg == "$t7") return 15;

    if (reg == "$s0") return 16;
    if (reg == "$s1") return 17;
    if (reg == "$s2") return 18;
    if (reg == "$s3") return 19;
    if (reg == "$s4") return 20;
    if (reg == "$s5") return 21;
    if (reg == "$s6") return 22;
    if (reg == "$s7") return 23;

    if (reg == "$t8") return 24;
    if (reg == "$t9") return 25;

    if (reg == "$k0") return 26;
    if (reg == "$k1") return 27;

    if (reg == "$gp") return 28;
    if (reg == "$sp") return 29;

    if (
        reg == "$fp" ||
        reg == "$s8"
    ) {
        return 30;
    }

    if (reg == "$ra") return 31;


    // --------------------------------------------------------
    // Numeric names:
    //
    // $0
    // $1
    // ...
    // $31
    // --------------------------------------------------------

    if (
        reg.size() >= 2 &&
        reg[0] == '$'
    ) {
        try {
            size_t used =
                0;


            int number =
                stoi(
                    reg.substr(1),
                    &used,
                    10
                );


            if (
                used ==
                    reg.size() - 1 &&
                validRegister(number)
            ) {
                return number;
            }
        }
        catch (...) {
        }
    }


    return -1;
}


// ============================================================
// TOKEN COUNTS
// ============================================================

bool Assembler::expectTokenCount(
    const vector<string>& tokens,
    size_t expected,
    const string& name
)
{
    if (
        tokens.size() ==
        expected
    ) {
        return true;
    }


    reportError(
        name +
        " expects " +
        to_string(
            expected - 1
        ) +
        " operand(s), but got " +
        to_string(
            tokens.empty()
                ? 0
                : tokens.size() - 1
        )
    );


    return false;
}


bool Assembler::expectTokenCountEither(
    const vector<string>& tokens,
    size_t first,
    size_t second,
    const string& name
)
{
    if (
        tokens.size() == first ||
        tokens.size() == second
    ) {
        return true;
    }


    reportError(
        name +
        " expects " +
        to_string(
            first - 1
        ) +
        " or " +
        to_string(
            second - 1
        ) +
        " operand(s), but got " +
        to_string(
            tokens.empty()
                ? 0
                : tokens.size() - 1
        )
    );


    return false;
}


// ============================================================
// INTEGER PARSING
// ============================================================

bool Assembler::parseInt16(
    const string& text,
    int16_t& value
)
{
    try {
        size_t used =
            0;


        long long parsed =
            stoll(
                text,
                &used,
                0
            );


        if (
            used != text.size() ||
            parsed < INT16_MIN ||
            parsed > INT16_MAX
        ) {
            return false;
        }


        value =
            static_cast<int16_t>(
                parsed
            );


        return true;
    }
    catch (...) {
        return false;
    }
}


bool Assembler::parseUint16(
    const string& text,
    uint16_t& value
)
{
    try {
        if (
            !text.empty() &&
            text[0] == '-'
        ) {
            return false;
        }


        size_t used =
            0;


        unsigned long long parsed =
            stoull(
                text,
                &used,
                0
            );


        if (
            used != text.size() ||
            parsed > UINT16_MAX
        ) {
            return false;
        }


        value =
            static_cast<uint16_t>(
                parsed
            );


        return true;
    }
    catch (...) {
        return false;
    }
}


bool Assembler::parseUint32(
    const string& text,
    uint32_t& value
)
{
    try {
        if (
            !text.empty() &&
            text[0] == '-'
        ) {
            return false;
        }


        size_t used =
            0;


        unsigned long long parsed =
            stoull(
                text,
                &used,
                0
            );


        if (
            used != text.size() ||
            parsed > UINT32_MAX
        ) {
            return false;
        }


        value =
            static_cast<uint32_t>(
                parsed
            );


        return true;
    }
    catch (...) {
        return false;
    }
}


bool Assembler::parseInt32(
    const string& text,
    int32_t& value
)
{
    try {
        size_t used =
            0;


        long long parsed =
            stoll(
                text,
                &used,
                0
            );


        if (
            used != text.size()
        ) {
            return false;
        }


        if (
            parsed >= INT32_MIN &&
            parsed <= INT32_MAX
        ) {
            value =
                static_cast<int32_t>(
                    parsed
                );

            return true;
        }


        // Allow raw 32-bit hexadecimal patterns such as:
        //
        // 0xFFFFFFFF

        if (
            parsed >= 0 &&
            static_cast<unsigned long long>(
                parsed
            ) <= UINT32_MAX
        ) {
            value =
                static_cast<int32_t>(
                    static_cast<uint32_t>(
                        parsed
                    )
                );

            return true;
        }


        return false;
    }
    catch (...) {
        return false;
    }
}


// ============================================================
// ASCIIZ
// ============================================================

bool Assembler::parseAsciiz(
    const string& line,
    string& value
)
{
    size_t first =
        line.find('"');


    size_t last =
        line.rfind('"');


    if (
        first == string::npos ||
        last == string::npos ||
        first == last
    ) {
        return false;
    }


    value =
        line.substr(
            first + 1,
            last - first - 1
        );


    return true;
}


// ============================================================
// BRANCH TARGET
// ============================================================

bool Assembler::parseBranchTarget(
    const string& text,
    uint32_t currentAddress,
    int16_t& immediate,
    const string& name
)
{
    auto it =
        labels.find(
            text
        );


    if (
        it != labels.end()
    ) {
        int64_t difference =
            static_cast<int64_t>(
                it->second
            )
            -
            static_cast<int64_t>(
                currentAddress + 4
            );


        if (
            difference %
                4 !=
            0
        ) {
            reportError(
                "branch target '" +
                text +
                "' is not word-aligned in " +
                name
            );

            return false;
        }


        int64_t offset =
            difference /
            4;


        if (
            offset < INT16_MIN ||
            offset > INT16_MAX
        ) {
            reportError(
                "branch target '" +
                text +
                "' is out of range in " +
                name
            );

            return false;
        }


        immediate =
            static_cast<int16_t>(
                offset
            );


        return true;
    }


    if (!parseInt16(
            text,
            immediate
        )) {
        reportError(
            "invalid branch target '" +
            text +
            "' in " +
            name
        );

        return false;
    }


    return true;
}


// ============================================================
// JUMP TARGET
// ============================================================

bool Assembler::parseJumpTarget(
    const string& text,
    uint32_t& targetAddress,
    const string& name
)
{
    auto it =
        labels.find(
            text
        );


    if (
        it != labels.end()
    ) {
        targetAddress =
            it->second;
    }
    else if (!parseUint32(
                 text,
                 targetAddress
             )) {
        reportError(
            "invalid jump target '" +
            text +
            "' in " +
            name
        );

        return false;
    }


    if (
        (
            targetAddress &
            3U
        ) != 0
    ) {
        reportError(
            "jump target in " +
            name +
            " must be word-aligned"
        );

        return false;
    }


    return true;
}


// ============================================================
// ERROR
// ============================================================

void Assembler::reportError(
    const string& message
)
{
    cout
        << "Assembler error on line "
        << currentLineNumber
        << ":\n"
        << currentLineText
        << '\n'
        << message
        << '\n';


    assemblyError =
        true;
}


// ============================================================
// ENCODE R
// ============================================================

uint32_t Assembler::encodeR(
    uint8_t rs,
    uint8_t rt,
    uint8_t rd,
    uint8_t shamt,
    uint8_t funct
)
{
    return
        (
            static_cast<uint32_t>(
                rs
            )
            << 21
        )
        |
        (
            static_cast<uint32_t>(
                rt
            )
            << 16
        )
        |
        (
            static_cast<uint32_t>(
                rd
            )
            << 11
        )
        |
        (
            static_cast<uint32_t>(
                shamt
            )
            << 6
        )
        |
        static_cast<uint32_t>(
            funct
        );
}


// ============================================================
// ENCODE I
// ============================================================

uint32_t Assembler::encodeI(
    uint8_t opcode,
    uint8_t rs,
    uint8_t rt,
    int16_t imm
)
{
    return
        (
            static_cast<uint32_t>(
                opcode
            )
            << 26
        )
        |
        (
            static_cast<uint32_t>(
                rs
            )
            << 21
        )
        |
        (
            static_cast<uint32_t>(
                rt
            )
            << 16
        )
        |
        static_cast<uint16_t>(
            imm
        );
}


// ============================================================
// ENCODE J
// ============================================================

uint32_t Assembler::encodeJ(
    uint8_t opcode,
    uint32_t addressField
)
{
    return
        (
            static_cast<uint32_t>(
                opcode
            )
            << 26
        )
        |
        (
            addressField &
            0x03FFFFFFU
        );
}


// ============================================================
// SPECIAL2
// ============================================================

uint32_t Assembler::encodeSpecial2(
    uint8_t rs,
    uint8_t rt,
    uint8_t rd,
    uint8_t shamt,
    uint8_t funct
)
{
    return
        (
            0x1CU <<
            26
        )
        |
        encodeR(
            rs,
            rt,
            rd,
            shamt,
            funct
        );
}


// ============================================================
// INSTRUCTION SIZE
// ============================================================

uint32_t Assembler::instructionSize(
    const vector<string>& tokens
)
{
    if (
        tokens.empty()
    ) {
        return 0;
    }


    const string op =
        lowerCopy(
            tokens[0]
        );


    // --------------------------------------------------------
    // LI
    // --------------------------------------------------------

    if (
        op ==
        "li"
    ) {
        if (
            tokens.size() !=
            3
        ) {
            return 4;
        }


        int32_t value =
            0;


        if (!parseInt32(
                tokens[2],
                value
            )) {
            return 4;
        }


        return
            (
                value >= INT16_MIN &&
                value <= INT16_MAX
            )
            ? 4
            : 8;
    }


    // LA always uses LUI + ORI.
    if (
        op ==
        "la"
    ) {
        return 8;
    }


    // --------------------------------------------------------
    // TWO-INSTRUCTION PSEUDOS
    // --------------------------------------------------------

    if (
        op == "blt" ||
        op == "ble" ||
        op == "bgt" ||
        op == "bge" ||

        op == "bltu" ||
        op == "bleu" ||
        op == "bgtu" ||
        op == "bgeu" ||

        op == "seq" ||
        op == "sne" ||
        op == "sge" ||
        op == "sle" ||
        op == "sgeu" ||
        op == "sleu" ||

        op == "rem" ||
        op == "remu"
    ) {
        return 8;
    }


    return 4;
}


// ============================================================
// FIRST PASS
// ============================================================

bool Assembler::firstPass(
    const string& filename,
    uint32_t startAddress
)
{
    ifstream file(
        filename
    );


    if (!file.is_open()) {
        return false;
    }


    labels.clear();


    Section section =
        Section::TEXT;


    uint32_t textAddress =
        startAddress;


    uint32_t dataAddress =
        DATA_START;


    string line;

    int lineNumber =
        0;


    while (
        getline(
            file,
            line
        )
    ) {
        ++lineNumber;


        currentLineNumber =
            lineNumber;


        currentLineText =
            line;


        vector<string> tokens =
            tokenize(
                line
            );


        if (
            tokens.empty()
        ) {
            continue;
        }


        // ----------------------------------------------------
        // Labels may now share a line with code:
        //
        // loop: addiu $t0, $t0, 1
        // ----------------------------------------------------

        while (
            !tokens.empty() &&
            tokens[0].back() == ':'
        ) {
            string label;


            removeLeadingLabel(
                tokens,
                &label
            );


            if (
                label.empty()
            ) {
                reportError(
                    "empty label"
                );

                return false;
            }


            if (
                labels.find(label) !=
                labels.end()
            ) {
                reportError(
                    "duplicate label '" +
                    label +
                    "'"
                );

                return false;
            }


            labels[label] =
                section ==
                    Section::TEXT
                ? textAddress
                : dataAddress;
        }


        if (
            tokens.empty()
        ) {
            continue;
        }


        const string op =
            lowerCopy(
                tokens[0]
            );


        if (
            op ==
            ".text"
        ) {
            section =
                Section::TEXT;

            continue;
        }


        if (
            op ==
            ".data"
        ) {
            section =
                Section::DATA;

            continue;
        }


        if (
            section ==
            Section::TEXT
        ) {
            textAddress +=
                instructionSize(
                    tokens
                );

            continue;
        }


        if (
            op ==
            ".word"
        ) {
            if (
                tokens.size() >=
                2
            ) {
                dataAddress +=
                    static_cast<uint32_t>(
                        (
                            tokens.size() -
                            1
                        )
                        *
                        4
                    );
            }

            continue;
        }


        if (
            op ==
            ".asciiz"
        ) {
            string value;


            if (!parseAsciiz(
                    line,
                    value
                )) {
                reportError(
                    "invalid .asciiz string"
                );

                return false;
            }


            dataAddress +=
                static_cast<uint32_t>(
                    value.size() +
                    1
                );
        }
    }


    return
        !assemblyError;
}


// ============================================================
// ASSEMBLE FILE
// ============================================================

bool Assembler::assembleFile(
    const string& filename,
    Memory& mem,
    uint32_t startAddress
)
{
    assemblyError =
        false;


    currentLineNumber =
        0;


    currentLineText.clear();


    if (!firstPass(
            filename,
            startAddress
        )) {
        if (!assemblyError) {
            cout
                << "First pass failed\n";
        }

        return false;
    }


    ifstream file(
        filename
    );


    if (!file.is_open()) {
        cout
            << "Could not open assembly file: "
            << filename
            << '\n';

        return false;
    }


    Section section =
        Section::TEXT;


    uint32_t textAddress =
        startAddress;


    uint32_t dataAddress =
        DATA_START;


    string line;

    int lineNumber =
        0;


    while (
        getline(
            file,
            line
        )
    ) {
        ++lineNumber;


        currentLineNumber =
            lineNumber;


        currentLineText =
            line;


        vector<string> tokens =
            tokenize(
                line
            );


        if (
            tokens.empty()
        ) {
            continue;
        }


        while (
            !tokens.empty() &&
            tokens[0].back() == ':'
        ) {
            removeLeadingLabel(
                tokens
            );
        }


        if (
            tokens.empty()
        ) {
            continue;
        }


        const string op =
            lowerCopy(
                tokens[0]
            );


        // ----------------------------------------------------
        // Sections
        // ----------------------------------------------------

        if (
            op ==
            ".text"
        ) {
            section =
                Section::TEXT;

            continue;
        }


        if (
            op ==
            ".data"
        ) {
            section =
                Section::DATA;

            continue;
        }


        // ====================================================
        // DATA
        // ====================================================

        if (
            section ==
            Section::DATA
        ) {
            if (
                op ==
                ".word"
            ) {
                if (
                    tokens.size() <
                    2
                ) {
                    reportError(
                        ".WORD expects at least 1 value"
                    );

                    return false;
                }


                // Multiple values now work:
                //
                // .word 1, 2, 3, 4

                for (
                    size_t i = 1;
                    i < tokens.size();
                    ++i
                ) {
                    int32_t value =
                        0;


                    if (!parseInt32(
                            tokens[i],
                            value
                        )) {
                        reportError(
                            "invalid 32-bit value '" +
                            tokens[i] +
                            "' in .word"
                        );

                        return false;
                    }


                    mem.memWrite32(
                        dataAddress,
                        static_cast<uint32_t>(
                            value
                        )
                    );


                    dataAddress +=
                        4;
                }


                continue;
            }


            if (
                op ==
                ".asciiz"
            ) {
                string value;


                if (!parseAsciiz(
                        line,
                        value
                    )) {
                    reportError(
                        "invalid .asciiz string"
                    );

                    return false;
                }


                for (
                    unsigned char c :
                    value
                ) {
                    mem.memWriteByte(
                        dataAddress++,
                        c
                    );
                }


                mem.memWriteByte(
                    dataAddress++,
                    0
                );


                continue;
            }


            reportError(
                "unknown data directive '" +
                tokens[0] +
                "'"
            );


            return false;
        }


        // ====================================================
        // TEXT
        // ====================================================

        vector<uint32_t> machineCodes =
            expandInstruction(
                tokens,
                textAddress
            );


        if (
            assemblyError
        ) {
            return false;
        }


        for (
            uint32_t machineCode :
            machineCodes
        ) {
            mem.memWrite32(
                textAddress,
                machineCode
            );


            cout
                << "0x"
                << hex
                << uppercase
                << machineCode
                << '\n';


            textAddress +=
                4;
        }
    }


    return
        !assemblyError;
}


// ============================================================
// PSEUDO-INSTRUCTION EXPANSION
// ============================================================

vector<uint32_t> Assembler::expandInstruction(
    const vector<string>& tokens,
    uint32_t currentAddress
)
{
    if (
        tokens.empty()
    ) {
        return {};
    }


    const string op =
        lowerCopy(
            tokens[0]
        );


    auto one =
        [&](
            const vector<string>& real
        ) -> vector<uint32_t>
        {
            uint32_t code =
                assembleInstruction(
                    real,
                    currentAddress
                );


            return
                assemblyError
                ? vector<uint32_t>{}
                : vector<uint32_t>{
                      code
                  };
        };


    auto two =
        [&](
            const vector<string>& first,
            const vector<string>& second
        ) -> vector<uint32_t>
        {
            uint32_t firstCode =
                assembleInstruction(
                    first,
                    currentAddress
                );


            if (
                assemblyError
            ) {
                return {};
            }


            uint32_t secondCode =
                assembleInstruction(
                    second,
                    currentAddress +
                    4
                );


            if (
                assemblyError
            ) {
                return {};
            }


            return {
                firstCode,
                secondCode
            };
        };


    // ========================================================
    // LI
    // ========================================================

    if (
        op ==
        "li"
    ) {
        if (!expectTokenCount(
                tokens,
                3,
                "LI"
            )) {
            return {};
        }


        int rt =
            registerNumber(
                tokens[1]
            );


        if (!validRegister(
                rt
            )) {
            reportError(
                "invalid destination register '" +
                tokens[1] +
                "' in LI"
            );

            return {};
        }


        int32_t value =
            0;


        if (!parseInt32(
                tokens[2],
                value
            )) {
            reportError(
                "invalid 32-bit immediate '" +
                tokens[2] +
                "' in LI"
            );

            return {};
        }


        // Small values need only one ADDIU.

        if (
            value >= INT16_MIN &&
            value <= INT16_MAX
        ) {
            return {
                encodeI(
                    0x09,
                    0,
                    static_cast<uint8_t>(
                        rt
                    ),
                    static_cast<int16_t>(
                        value
                    )
                )
            };
        }


        // Full-width LI:
        //
        // lui rt, upper
        // ori rt, rt, lower

        uint32_t raw =
            static_cast<uint32_t>(
                value
            );


        uint16_t upper =
            static_cast<uint16_t>(
                raw >>
                16
            );


        uint16_t lower =
            static_cast<uint16_t>(
                raw &
                0xFFFFU
            );


        return {
            encodeI(
                0x0F,
                0,
                static_cast<uint8_t>(
                    rt
                ),
                static_cast<int16_t>(
                    upper
                )
            ),

            encodeI(
                0x0D,
                static_cast<uint8_t>(
                    rt
                ),
                static_cast<uint8_t>(
                    rt
                ),
                static_cast<int16_t>(
                    lower
                )
            )
        };
    }


    // ========================================================
    // LA
    // ========================================================

    if (
        op ==
        "la"
    ) {
        if (!expectTokenCount(
                tokens,
                3,
                "LA"
            )) {
            return {};
        }


        int rt =
            registerNumber(
                tokens[1]
            );


        if (!validRegister(
                rt
            )) {
            reportError(
                "invalid destination register '" +
                tokens[1] +
                "' in LA"
            );

            return {};
        }


        auto it =
            labels.find(
                tokens[2]
            );


        if (
            it ==
            labels.end()
        ) {
            reportError(
                "unknown label '" +
                tokens[2] +
                "' in LA"
            );

            return {};
        }


        uint32_t address =
            it->second;


        return {
            encodeI(
                0x0F,
                0,
                static_cast<uint8_t>(
                    rt
                ),
                static_cast<int16_t>(
                    address >>
                    16
                )
            ),

            encodeI(
                0x0D,
                static_cast<uint8_t>(
                    rt
                ),
                static_cast<uint8_t>(
                    rt
                ),
                static_cast<int16_t>(
                    address &
                    0xFFFFU
                )
            )
        };
    }


    // ========================================================
    // ONE-INSTRUCTION PSEUDOS
    // ========================================================

    if (
        op ==
        "nop"
    ) {
        if (!expectTokenCount(
                tokens,
                1,
                "NOP"
            )) {
            return {};
        }

        return one({
            "sll",
            "$zero",
            "$zero",
            "0"
        });
    }


    if (
        op ==
        "move"
    ) {
        if (!expectTokenCount(
                tokens,
                3,
                "MOVE"
            )) {
            return {};
        }

        return one({
            "addu",
            tokens[1],
            tokens[2],
            "$zero"
        });
    }


    if (
        op ==
        "clear"
    ) {
        if (!expectTokenCount(
                tokens,
                2,
                "CLEAR"
            )) {
            return {};
        }

        return one({
            "addu",
            tokens[1],
            "$zero",
            "$zero"
        });
    }


    if (
        op ==
        "neg"
    ) {
        if (!expectTokenCount(
                tokens,
                3,
                "NEG"
            )) {
            return {};
        }

        return one({
            "sub",
            tokens[1],
            "$zero",
            tokens[2]
        });
    }


    if (
        op ==
        "negu"
    ) {
        if (!expectTokenCount(
                tokens,
                3,
                "NEGU"
            )) {
            return {};
        }

        return one({
            "subu",
            tokens[1],
            "$zero",
            tokens[2]
        });
    }


    if (
        op ==
        "not"
    ) {
        if (!expectTokenCount(
                tokens,
                3,
                "NOT"
            )) {
            return {};
        }

        return one({
            "nor",
            tokens[1],
            tokens[2],
            "$zero"
        });
    }


    // ========================================================
    // SIMPLE BRANCH PSEUDOS
    // ========================================================

    if (
        op ==
        "b"
    ) {
        if (!expectTokenCount(
                tokens,
                2,
                "B"
            )) {
            return {};
        }

        return one({
            "beq",
            "$zero",
            "$zero",
            tokens[1]
        });
    }


    if (
        op ==
        "bal"
    ) {
        if (!expectTokenCount(
                tokens,
                2,
                "BAL"
            )) {
            return {};
        }

        return one({
            "bgezal",
            "$zero",
            tokens[1]
        });
    }


    if (
        op ==
        "beqz"
    ) {
        if (!expectTokenCount(
                tokens,
                3,
                "BEQZ"
            )) {
            return {};
        }

        return one({
            "beq",
            tokens[1],
            "$zero",
            tokens[2]
        });
    }


    if (
        op ==
        "bnez"
    ) {
        if (!expectTokenCount(
                tokens,
                3,
                "BNEZ"
            )) {
            return {};
        }

        return one({
            "bne",
            tokens[1],
            "$zero",
            tokens[2]
        });
    }


    // ========================================================
    // SIMPLE COMPARISON PSEUDOS
    // ========================================================

    if (
        op ==
        "sgt"
    ) {
        if (!expectTokenCount(
                tokens,
                4,
                "SGT"
            )) {
            return {};
        }

        return one({
            "slt",
            tokens[1],
            tokens[3],
            tokens[2]
        });
    }


    if (
        op ==
        "sgtu"
    ) {
        if (!expectTokenCount(
                tokens,
                4,
                "SGTU"
            )) {
            return {};
        }

        return one({
            "sltu",
            tokens[1],
            tokens[3],
            tokens[2]
        });
    }


    // ========================================================
    // RELATIONAL BRANCH PSEUDOS
    // --------------------------------------------------------
    // These use $at as the assembler temporary.
    // ========================================================

    if (op == "blt") {
        if (!expectTokenCount(tokens, 4, "BLT")) return {};

        return two(
            {
                "slt",
                "$at",
                tokens[1],
                tokens[2]
            },
            {
                "bne",
                "$at",
                "$zero",
                tokens[3]
            }
        );
    }


    if (op == "bge") {
        if (!expectTokenCount(tokens, 4, "BGE")) return {};

        return two(
            {
                "slt",
                "$at",
                tokens[1],
                tokens[2]
            },
            {
                "beq",
                "$at",
                "$zero",
                tokens[3]
            }
        );
    }


    if (op == "bgt") {
        if (!expectTokenCount(tokens, 4, "BGT")) return {};

        return two(
            {
                "slt",
                "$at",
                tokens[2],
                tokens[1]
            },
            {
                "bne",
                "$at",
                "$zero",
                tokens[3]
            }
        );
    }


    if (op == "ble") {
        if (!expectTokenCount(tokens, 4, "BLE")) return {};

        return two(
            {
                "slt",
                "$at",
                tokens[2],
                tokens[1]
            },
            {
                "beq",
                "$at",
                "$zero",
                tokens[3]
            }
        );
    }


    if (op == "bltu") {
        if (!expectTokenCount(tokens, 4, "BLTU")) return {};

        return two(
            {
                "sltu",
                "$at",
                tokens[1],
                tokens[2]
            },
            {
                "bne",
                "$at",
                "$zero",
                tokens[3]
            }
        );
    }


    if (op == "bgeu") {
        if (!expectTokenCount(tokens, 4, "BGEU")) return {};

        return two(
            {
                "sltu",
                "$at",
                tokens[1],
                tokens[2]
            },
            {
                "beq",
                "$at",
                "$zero",
                tokens[3]
            }
        );
    }


    if (op == "bgtu") {
        if (!expectTokenCount(tokens, 4, "BGTU")) return {};

        return two(
            {
                "sltu",
                "$at",
                tokens[2],
                tokens[1]
            },
            {
                "bne",
                "$at",
                "$zero",
                tokens[3]
            }
        );
    }


    if (op == "bleu") {
        if (!expectTokenCount(tokens, 4, "BLEU")) return {};

        return two(
            {
                "sltu",
                "$at",
                tokens[2],
                tokens[1]
            },
            {
                "beq",
                "$at",
                "$zero",
                tokens[3]
            }
        );
    }


    // ========================================================
    // BOOLEAN COMPARISON PSEUDOS
    // ========================================================

    if (op == "seq") {
        if (!expectTokenCount(tokens, 4, "SEQ")) return {};

        return two(
            {
                "xor",
                tokens[1],
                tokens[2],
                tokens[3]
            },
            {
                "sltiu",
                tokens[1],
                tokens[1],
                "1"
            }
        );
    }


    if (op == "sne") {
        if (!expectTokenCount(tokens, 4, "SNE")) return {};

        return two(
            {
                "xor",
                tokens[1],
                tokens[2],
                tokens[3]
            },
            {
                "sltu",
                tokens[1],
                "$zero",
                tokens[1]
            }
        );
    }


    if (op == "sge") {
        if (!expectTokenCount(tokens, 4, "SGE")) return {};

        return two(
            {
                "slt",
                tokens[1],
                tokens[2],
                tokens[3]
            },
            {
                "xori",
                tokens[1],
                tokens[1],
                "1"
            }
        );
    }


    if (op == "sle") {
        if (!expectTokenCount(tokens, 4, "SLE")) return {};

        return two(
            {
                "slt",
                tokens[1],
                tokens[3],
                tokens[2]
            },
            {
                "xori",
                tokens[1],
                tokens[1],
                "1"
            }
        );
    }


    if (op == "sgeu") {
        if (!expectTokenCount(tokens, 4, "SGEU")) return {};

        return two(
            {
                "sltu",
                tokens[1],
                tokens[2],
                tokens[3]
            },
            {
                "xori",
                tokens[1],
                tokens[1],
                "1"
            }
        );
    }


    if (op == "sleu") {
        if (!expectTokenCount(tokens, 4, "SLEU")) return {};

        return two(
            {
                "sltu",
                tokens[1],
                tokens[3],
                tokens[2]
            },
            {
                "xori",
                tokens[1],
                tokens[1],
                "1"
            }
        );
    }


    // ========================================================
    // REMAINDER PSEUDOS
    // ========================================================

    if (op == "rem") {
        if (!expectTokenCount(tokens, 4, "REM")) return {};

        return two(
            {
                "div",
                tokens[2],
                tokens[3]
            },
            {
                "mfhi",
                tokens[1]
            }
        );
    }


    if (op == "remu") {
        if (!expectTokenCount(tokens, 4, "REMU")) return {};

        return two(
            {
                "divu",
                tokens[2],
                tokens[3]
            },
            {
                "mfhi",
                tokens[1]
            }
        );
    }


    // ========================================================
    // REAL INSTRUCTION
    // ========================================================

    uint32_t machineCode =
        assembleInstruction(
            tokens,
            currentAddress
        );


    if (
        assemblyError
    ) {
        return {};
    }


    return {
        machineCode
    };
}


// ============================================================
// REAL INSTRUCTION ENCODING
// ============================================================

uint32_t Assembler::assembleInstruction(
    const vector<string>& tokens,
    uint32_t currentAddress
)
{
    if (
        tokens.empty()
    ) {
        return 0;
    }


    const string op =
        lowerCopy(
            tokens[0]
        );


    // --------------------------------------------------------
    // Register parser with useful error text.
    // --------------------------------------------------------

    auto reg =
        [&](
            const string& text,
            const string& role
        ) -> int
        {
            int value =
                registerNumber(
                    text
                );


            if (!validRegister(
                    value
                )) {
                reportError(
                    "invalid " +
                    role +
                    " register '" +
                    text +
                    "' in " +
                    tokens[0]
                );

                return -1;
            }


            return value;
        };


    // --------------------------------------------------------
    // Standard rd, rs, rt R-type encoder.
    // --------------------------------------------------------

    auto r3 =
        [&](
            uint8_t funct
        ) -> uint32_t
        {
            if (!expectTokenCount(
                    tokens,
                    4,
                    tokens[0]
                )) {
                return 0;
            }


            int rd =
                reg(
                    tokens[1],
                    "destination"
                );


            int rs =
                reg(
                    tokens[2],
                    "source"
                );


            int rt =
                reg(
                    tokens[3],
                    "source"
                );


            if (
                assemblyError
            ) {
                return 0;
            }


            return encodeR(
                static_cast<uint8_t>(
                    rs
                ),

                static_cast<uint8_t>(
                    rt
                ),

                static_cast<uint8_t>(
                    rd
                ),

                0,

                funct
            );
        };


    // ========================================================
    // NORMAL R-TYPE
    // ========================================================

    if (op == "add") return r3(0x20);
    if (op == "addu") return r3(0x21);

    if (op == "sub") return r3(0x22);
    if (op == "subu") return r3(0x23);

    if (op == "and") return r3(0x24);
    if (op == "or") return r3(0x25);

    if (op == "xor") return r3(0x26);
    if (op == "nor") return r3(0x27);

    if (op == "slt") return r3(0x2A);
    if (op == "sltu") return r3(0x2B);

    if (op == "movz") return r3(0x0A);
    if (op == "movn") return r3(0x0B);


    // ========================================================
    // FIXED SHIFTS
    // ========================================================

    if (
        op == "sll" ||
        op == "srl" ||
        op == "sra"
    ) {
        if (!expectTokenCount(
                tokens,
                4,
                tokens[0]
            )) {
            return 0;
        }


        int rd =
            reg(
                tokens[1],
                "destination"
            );


        int rt =
            reg(
                tokens[2],
                "source"
            );


        if (
            assemblyError
        ) {
            return 0;
        }


        uint32_t shamt =
            0;


        if (
            !parseUint32(
                tokens[3],
                shamt
            )
            ||
            shamt > 31
        ) {
            reportError(
                "shift amount in " +
                tokens[0] +
                " must be between 0 and 31"
            );

            return 0;
        }


        uint8_t funct =
            op == "sll"
            ? 0x00
            : op == "srl"
              ? 0x02
              : 0x03;


        return encodeR(
            0,

            static_cast<uint8_t>(
                rt
            ),

            static_cast<uint8_t>(
                rd
            ),

            static_cast<uint8_t>(
                shamt
            ),

            funct
        );
    }


    // ========================================================
    // VARIABLE SHIFTS
    // syntax:
    //
    // sllv rd, rt, rs
    // ========================================================

    if (
        op == "sllv" ||
        op == "srlv" ||
        op == "srav"
    ) {
        if (!expectTokenCount(
                tokens,
                4,
                tokens[0]
            )) {
            return 0;
        }


        int rd =
            reg(
                tokens[1],
                "destination"
            );


        int rt =
            reg(
                tokens[2],
                "source"
            );


        int rs =
            reg(
                tokens[3],
                "shift-count"
            );


        if (
            assemblyError
        ) {
            return 0;
        }


        uint8_t funct =
            op == "sllv"
            ? 0x04
            : op == "srlv"
              ? 0x06
              : 0x07;


        return encodeR(
            static_cast<uint8_t>(
                rs
            ),

            static_cast<uint8_t>(
                rt
            ),

            static_cast<uint8_t>(
                rd
            ),

            0,

            funct
        );
    }


    // ========================================================
    // JR
    // ========================================================

    if (
        op ==
        "jr"
    ) {
        if (!expectTokenCount(
                tokens,
                2,
                "JR"
            )) {
            return 0;
        }


        int rs =
            reg(
                tokens[1],
                "source"
            );


        if (
            assemblyError
        ) {
            return 0;
        }


        return encodeR(
            static_cast<uint8_t>(
                rs
            ),
            0,
            0,
            0,
            0x08
        );
    }


    // ========================================================
    // JALR
    // --------------------------------------------------------
    // Both forms are accepted:
    //
    // jalr $rs
    // jalr $rd, $rs
    //
    // First form defaults rd to $ra.
    // ========================================================

    if (
        op ==
        "jalr"
    ) {
        if (!expectTokenCountEither(
                tokens,
                2,
                3,
                "JALR"
            )) {
            return 0;
        }


        int rd =
            31;


        int rs =
            -1;


        if (
            tokens.size() ==
            2
        ) {
            rs =
                reg(
                    tokens[1],
                    "source"
                );
        }
        else {
            rd =
                reg(
                    tokens[1],
                    "destination"
                );


            rs =
                reg(
                    tokens[2],
                    "source"
                );
        }


        if (
            assemblyError
        ) {
            return 0;
        }


        return encodeR(
            static_cast<uint8_t>(
                rs
            ),
            0,
            static_cast<uint8_t>(
                rd
            ),
            0,
            0x09
        );
    }


    // ========================================================
    // MULT / MULTU / DIV / DIVU
    // ========================================================

    if (
        op == "mult" ||
        op == "multu" ||
        op == "div" ||
        op == "divu"
    ) {
        if (!expectTokenCount(
                tokens,
                3,
                tokens[0]
            )) {
            return 0;
        }


        int rs =
            reg(
                tokens[1],
                "source"
            );


        int rt =
            reg(
                tokens[2],
                "source"
            );


        if (
            assemblyError
        ) {
            return 0;
        }


        uint8_t funct =
            op == "mult"
            ? 0x18
            : op == "multu"
              ? 0x19
              : op == "div"
                ? 0x1A
                : 0x1B;


        return encodeR(
            static_cast<uint8_t>(
                rs
            ),

            static_cast<uint8_t>(
                rt
            ),

            0,
            0,

            funct
        );
    }


    // ========================================================
    // MFHI / MFLO
    // ========================================================

    if (
        op == "mfhi" ||
        op == "mflo"
    ) {
        if (!expectTokenCount(
                tokens,
                2,
                tokens[0]
            )) {
            return 0;
        }


        int rd =
            reg(
                tokens[1],
                "destination"
            );


        if (
            assemblyError
        ) {
            return 0;
        }


        return encodeR(
            0,
            0,
            static_cast<uint8_t>(
                rd
            ),
            0,
            op == "mfhi"
                ? 0x10
                : 0x12
        );
    }


    // ========================================================
    // MTHI / MTLO
    // ========================================================

    if (
        op == "mthi" ||
        op == "mtlo"
    ) {
        if (!expectTokenCount(
                tokens,
                2,
                tokens[0]
            )) {
            return 0;
        }


        int rs =
            reg(
                tokens[1],
                "source"
            );


        if (
            assemblyError
        ) {
            return 0;
        }


        return encodeR(
            static_cast<uint8_t>(
                rs
            ),
            0,
            0,
            0,
            op == "mthi"
                ? 0x11
                : 0x13
        );
    }


    // ========================================================
    // MUL -- SPECIAL2
    // ========================================================

    if (
        op ==
        "mul"
    ) {
        if (!expectTokenCount(
                tokens,
                4,
                "MUL"
            )) {
            return 0;
        }


        int rd =
            reg(
                tokens[1],
                "destination"
            );


        int rs =
            reg(
                tokens[2],
                "source"
            );


        int rt =
            reg(
                tokens[3],
                "source"
            );


        if (
            assemblyError
        ) {
            return 0;
        }


        return encodeSpecial2(
            static_cast<uint8_t>(
                rs
            ),

            static_cast<uint8_t>(
                rt
            ),

            static_cast<uint8_t>(
                rd
            ),

            0,

            0x02
        );
    }


    // ========================================================
    // CLZ / CLO -- SPECIAL2
    // ========================================================

    if (
        op == "clz" ||
        op == "clo"
    ) {
        if (!expectTokenCount(
                tokens,
                3,
                tokens[0]
            )) {
            return 0;
        }


        int rd =
            reg(
                tokens[1],
                "destination"
            );


        int rs =
            reg(
                tokens[2],
                "source"
            );


        if (
            assemblyError
        ) {
            return 0;
        }


        return encodeSpecial2(
            static_cast<uint8_t>(
                rs
            ),
            0,
            static_cast<uint8_t>(
                rd
            ),
            0,
            op == "clz"
                ? 0x20
                : 0x21
        );
    }


    // ========================================================
    // SYSCALL / BREAK
    // ========================================================

    if (
        op == "syscall" ||
        op == "break"
    ) {
        if (!expectTokenCountEither(
                tokens,
                1,
                2,
                tokens[0]
            )) {
            return 0;
        }


        uint32_t code =
            0;


        if (
            tokens.size() ==
            2
        ) {
            if (
                !parseUint32(
                    tokens[1],
                    code
                )
                ||
                code > 0xFFFFFU
            ) {
                reportError(
                    "code in " +
                    tokens[0] +
                    " must be between 0 and 0xFFFFF"
                );

                return 0;
            }
        }


        return
            (
                (
                    code &
                    0xFFFFFU
                )
                << 6
            )
            |
            (
                op == "syscall"
                ? 0x0CU
                : 0x0DU
            );
    }


    // ========================================================
    // SIGNED IMMEDIATE
    // ========================================================

    if (
        op == "addi" ||
        op == "addiu" ||
        op == "slti" ||
        op == "sltiu"
    ) {
        if (!expectTokenCount(
                tokens,
                4,
                tokens[0]
            )) {
            return 0;
        }


        int rt =
            reg(
                tokens[1],
                "destination"
            );


        int rs =
            reg(
                tokens[2],
                "source"
            );


        if (
            assemblyError
        ) {
            return 0;
        }


        int16_t imm =
            0;


        if (!parseInt16(
                tokens[3],
                imm
            )) {
            reportError(
                "invalid signed 16-bit immediate '" +
                tokens[3] +
                "' in " +
                tokens[0]
            );

            return 0;
        }


        uint8_t opcode =
            op == "addi"
            ? 0x08
            : op == "addiu"
              ? 0x09
              : op == "slti"
                ? 0x0A
                : 0x0B;


        return encodeI(
            opcode,

            static_cast<uint8_t>(
                rs
            ),

            static_cast<uint8_t>(
                rt
            ),

            imm
        );
    }


    // ========================================================
    // ZERO-EXTENDED IMMEDIATE
    // ========================================================

    if (
        op == "andi" ||
        op == "ori" ||
        op == "xori"
    ) {
        if (!expectTokenCount(
                tokens,
                4,
                tokens[0]
            )) {
            return 0;
        }


        int rt =
            reg(
                tokens[1],
                "destination"
            );


        int rs =
            reg(
                tokens[2],
                "source"
            );


        if (
            assemblyError
        ) {
            return 0;
        }


        uint16_t imm =
            0;


        if (!parseUint16(
                tokens[3],
                imm
            )) {
            reportError(
                "invalid unsigned 16-bit immediate '" +
                tokens[3] +
                "' in " +
                tokens[0]
            );

            return 0;
        }


        uint8_t opcode =
            op == "andi"
            ? 0x0C
            : op == "ori"
              ? 0x0D
              : 0x0E;


        return encodeI(
            opcode,

            static_cast<uint8_t>(
                rs
            ),

            static_cast<uint8_t>(
                rt
            ),

            static_cast<int16_t>(
                imm
            )
        );
    }


    // ========================================================
    // LUI
    // ========================================================

    if (
        op ==
        "lui"
    ) {
        if (!expectTokenCount(
                tokens,
                3,
                "LUI"
            )) {
            return 0;
        }


        int rt =
            reg(
                tokens[1],
                "destination"
            );


        if (
            assemblyError
        ) {
            return 0;
        }


        uint16_t imm =
            0;


        if (!parseUint16(
                tokens[2],
                imm
            )) {
            reportError(
                "invalid 16-bit immediate '" +
                tokens[2] +
                "' in LUI"
            );

            return 0;
        }


        return encodeI(
            0x0F,
            0,
            static_cast<uint8_t>(
                rt
            ),
            static_cast<int16_t>(
                imm
            )
        );
    }


    // ========================================================
    // MEMORY
    // ========================================================

    if (
        op == "lb" ||
        op == "lbu" ||
        op == "lh" ||
        op == "lhu" ||
        op == "lw" ||

        op == "sb" ||
        op == "sh" ||
        op == "sw"
    ) {
        if (!expectTokenCount(
                tokens,
                4,
                tokens[0]
            )) {
            return 0;
        }


        bool store =
            op == "sb" ||
            op == "sh" ||
            op == "sw";


        int rt =
            reg(
                tokens[1],
                store
                    ? "source"
                    : "destination"
            );


        int rs =
            reg(
                tokens[3],
                "base"
            );


        if (
            assemblyError
        ) {
            return 0;
        }


        int16_t offset =
            0;


        if (!parseInt16(
                tokens[2],
                offset
            )) {
            reportError(
                "invalid memory offset '" +
                tokens[2] +
                "' in " +
                tokens[0]
            );

            return 0;
        }


        uint8_t opcode =
            0;


        if (op == "lb") opcode = 0x20;
        else if (op == "lh") opcode = 0x21;
        else if (op == "lw") opcode = 0x23;
        else if (op == "lbu") opcode = 0x24;
        else if (op == "lhu") opcode = 0x25;
        else if (op == "sb") opcode = 0x28;
        else if (op == "sh") opcode = 0x29;
        else opcode = 0x2B;


        return encodeI(
            opcode,

            static_cast<uint8_t>(
                rs
            ),

            static_cast<uint8_t>(
                rt
            ),

            offset
        );
    }


    // ========================================================
    // BEQ / BNE
    // ========================================================

    if (
        op == "beq" ||
        op == "bne"
    ) {
        if (!expectTokenCount(
                tokens,
                4,
                tokens[0]
            )) {
            return 0;
        }


        int rs =
            reg(
                tokens[1],
                "source"
            );


        int rt =
            reg(
                tokens[2],
                "source"
            );


        if (
            assemblyError
        ) {
            return 0;
        }


        int16_t imm =
            0;


        if (!parseBranchTarget(
                tokens[3],
                currentAddress,
                imm,
                tokens[0]
            )) {
            return 0;
        }


        return encodeI(
            op == "beq"
                ? 0x04
                : 0x05,

            static_cast<uint8_t>(
                rs
            ),

            static_cast<uint8_t>(
                rt
            ),

            imm
        );
    }


    // ========================================================
    // BLEZ / BGTZ
    // ========================================================

    if (
        op == "blez" ||
        op == "bgtz"
    ) {
        if (!expectTokenCount(
                tokens,
                3,
                tokens[0]
            )) {
            return 0;
        }


        int rs =
            reg(
                tokens[1],
                "source"
            );


        if (
            assemblyError
        ) {
            return 0;
        }


        int16_t imm =
            0;


        if (!parseBranchTarget(
                tokens[2],
                currentAddress,
                imm,
                tokens[0]
            )) {
            return 0;
        }


        return encodeI(
            op == "blez"
                ? 0x06
                : 0x07,

            static_cast<uint8_t>(
                rs
            ),

            0,

            imm
        );
    }


    // ========================================================
    // REGIMM
    // ========================================================

    if (
        op == "bltz" ||
        op == "bgez" ||
        op == "bltzal" ||
        op == "bgezal"
    ) {
        if (!expectTokenCount(
                tokens,
                3,
                tokens[0]
            )) {
            return 0;
        }


        int rs =
            reg(
                tokens[1],
                "source"
            );


        if (
            assemblyError
        ) {
            return 0;
        }


        int16_t imm =
            0;


        if (!parseBranchTarget(
                tokens[2],
                currentAddress,
                imm,
                tokens[0]
            )) {
            return 0;
        }


        uint8_t rtField =
            op == "bltz"
            ? 0x00
            : op == "bgez"
              ? 0x01
              : op == "bltzal"
                ? 0x10
                : 0x11;


        return encodeI(
            0x01,

            static_cast<uint8_t>(
                rs
            ),

            rtField,

            imm
        );
    }


    // ========================================================
    // J / JAL
    // ========================================================

    if (
        op == "j" ||
        op == "jal"
    ) {
        if (!expectTokenCount(
                tokens,
                2,
                tokens[0]
            )) {
            return 0;
        }


        uint32_t target =
            0;


        if (!parseJumpTarget(
                tokens[1],
                target,
                tokens[0]
            )) {
            return 0;
        }


        return encodeJ(
            op == "j"
                ? 0x02
                : 0x03,

            target >>
            2
        );
    }


    // ========================================================
    // UNKNOWN
    // ========================================================

    reportError(
        "unknown instruction '" +
        tokens[0] +
        "'"
    );


    return 0;
}