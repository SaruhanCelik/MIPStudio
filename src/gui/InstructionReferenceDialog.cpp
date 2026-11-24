#include "gui/InstructionReferenceDialog.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSplitter>
#include <QTextBrowser>
#include <QVBoxLayout>


// ============================================================
// CONSTRUCTOR
// ============================================================

InstructionReferenceDialog::InstructionReferenceDialog(
    QWidget* parent
)
    : QDialog(parent),
      entries(
          buildEntries()
      )
{
    setObjectName("InstructionReferenceDialog");
    buildUI();

    refreshList();


    setWindowTitle(
        "Supported MIPS Instructions — MIPS Studio"
    );


    resize(
        980,
        650
    );


    setMinimumSize(
        760,
        500
    );
}


// ============================================================
// UI
// ============================================================

void InstructionReferenceDialog::buildUI()
{
    QVBoxLayout* mainLayout =
        new QVBoxLayout(
            this
        );


    QLabel* intro =
        new QLabel(
            "This reference describes the instructions accepted "
            "by the current MIPS Studio assembler. "
            "Pseudo-instructions are expanded by the assembler "
            "before execution. MIPS Studio currently executes "
            "without architectural branch-delay slots.",
            this
        );


    intro->setWordWrap(
        true
    );


    intro->setObjectName(
        "SettingsHint"
    );


    mainLayout->addWidget(
        intro
    );


    // ========================================================
    // FILTERS
    // ========================================================

    QHBoxLayout* filters =
        new QHBoxLayout();


    searchEdit =
        new QLineEdit(
            this
        );


    searchEdit->setPlaceholderText(
        "Search instruction, syntax, category or description..."
    );


    kindCombo =
        new QComboBox(
            this
        );


    kindCombo->addItem(
        "All",
        "all"
    );


    kindCombo->addItem(
        "Real instructions",
        "real"
    );


    kindCombo->addItem(
        "Pseudo-instructions",
        "pseudo"
    );


    categoryCombo =
        new QComboBox(
            this
        );


    categoryCombo->addItem(
        "All categories"
    );


    categoryCombo->addItems({
        "Arithmetic",
        "Logical",
        "Comparison",
        "Shift",
        "Multiply / Divide",
        "Move / Bit",
        "Memory",
        "Branch",
        "Jump",
        "Immediate",
        "System"
    });


    filters->addWidget(
        searchEdit,
        1
    );


    filters->addWidget(
        kindCombo
    );


    filters->addWidget(
        categoryCombo
    );


    mainLayout->addLayout(
        filters
    );


    // ========================================================
    // CONTENT
    // ========================================================

    QSplitter* splitter =
        new QSplitter(
            Qt::Horizontal,
            this
        );


    instructionList =
        new QListWidget(
            splitter
        );


    instructionList->setMinimumWidth(
        310
    );
    instructionList->setObjectName("InstructionList");


    detailsBrowser =
        new QTextBrowser(
            splitter
        );


    detailsBrowser->setOpenExternalLinks(
        false
    );
    detailsBrowser->setObjectName("InstructionDetails");


    splitter->addWidget(
        instructionList
    );


    splitter->addWidget(
        detailsBrowser
    );


    splitter->setStretchFactor(
        0,
        0
    );


    splitter->setStretchFactor(
        1,
        1
    );


    mainLayout->addWidget(
        splitter,
        1
    );


    // ========================================================
    // CLOSE
    // ========================================================

    QHBoxLayout* bottom =
        new QHBoxLayout();


    bottom->addStretch();


    QPushButton* closeButton =
        new QPushButton(
            "Close",
            this
        );


    bottom->addWidget(
        closeButton
    );


    mainLayout->addLayout(
        bottom
    );


    // ========================================================
    // CONNECTIONS
    // ========================================================

    connect(
        searchEdit,
        &QLineEdit::textChanged,
        this,
        &InstructionReferenceDialog::refreshList
    );


    connect(
        kindCombo,
        &QComboBox::currentIndexChanged,
        this,
        &InstructionReferenceDialog::refreshList
    );


    connect(
        categoryCombo,
        &QComboBox::currentIndexChanged,
        this,
        &InstructionReferenceDialog::refreshList
    );


    connect(
        instructionList,
        &QListWidget::currentRowChanged,
        this,
        &InstructionReferenceDialog::showSelectedEntry
    );


    connect(
        closeButton,
        &QPushButton::clicked,
        this,
        &QDialog::accept
    );
}


// ============================================================
// FILTER LIST
// ============================================================

void InstructionReferenceDialog::refreshList()
{
    const QString search =
        searchEdit
            ->text()
            .trimmed();


    const QString kind =
        kindCombo
            ->currentData()
            .toString();


    const QString category =
        categoryCombo
            ->currentText();


    instructionList->clear();


    for (
        int index = 0;
        index < entries.size();
        ++index
    ) {
        const Entry& entry =
            entries[index];


        if (
            kind == "real" &&
            entry.pseudo
        ) {
            continue;
        }


        if (
            kind == "pseudo" &&
            !entry.pseudo
        ) {
            continue;
        }


        if (
            category !=
                "All categories" &&
            entry.category !=
                category
        ) {
            continue;
        }


        const QString searchable =
            entry.name +
            " " +
            entry.syntax +
            " " +
            entry.category +
            " " +
            entry.description +
            " " +
            entry.operation;


        if (
            !search.isEmpty() &&
            !searchable.contains(
                search,
                Qt::CaseInsensitive
            )
        ) {
            continue;
        }


        QListWidgetItem* item =
            new QListWidgetItem(
                QString(
                    "%1    %2"
                )
                .arg(
                    entry.name,
                    entry.syntax
                ),
                instructionList
            );


        item->setData(
            Qt::UserRole,
            index
        );
    }


    if (
        instructionList->count() >
        0
    ) {
        instructionList->setCurrentRow(
            0
        );
    }
    else {
        detailsBrowser->setHtml(
            "<h3>No matching instructions</h3>"
            "<p>Try changing the search or filters.</p>"
        );
    }
}


// ============================================================
// SHOW DETAILS
// ============================================================

void InstructionReferenceDialog::showSelectedEntry()
{
    QListWidgetItem* item =
        instructionList
            ->currentItem();


    if (!item) {
        return;
    }


    const int index =
        item
            ->data(
                Qt::UserRole
            )
            .toInt();


    if (
        index < 0 ||
        index >= entries.size()
    ) {
        return;
    }


    const Entry& entry =
        entries[index];


    const QString kindText =
        entry.pseudo
        ? "Pseudo-instruction"
        : "Real machine instruction";


    QString html;


    html +=
        "<h2>" +
        entry.name.toHtmlEscaped() +
        "</h2>";


    html +=
        "<p><b>Type:</b> " +
        kindText.toHtmlEscaped() +
        "<br>";


    html +=
        "<b>Category:</b> " +
        entry.category.toHtmlEscaped() +
        "</p>";


    html +=
        "<h3>Syntax</h3><pre>" +
        entry.syntax.toHtmlEscaped() +
        "</pre>";


    html +=
        "<h3>What it does</h3><p>" +
        entry.description.toHtmlEscaped() +
        "</p>";


    html +=
        entry.pseudo
        ? "<h3>Assembler expansion / operation</h3><pre>"
        : "<h3>Operation</h3><pre>";


    html +=
        entry.operation.toHtmlEscaped() +
        "</pre>";


    html +=
        "<h3>Example</h3><pre>" +
        entry.example.toHtmlEscaped() +
        "</pre>";


    detailsBrowser->setHtml(
        html
    );
}


// ============================================================
// REFERENCE DATABASE
// ============================================================

QVector<InstructionReferenceDialog::Entry>
InstructionReferenceDialog::buildEntries()
{
    QVector<Entry> entries;


    auto real =
        [&](
            const char* name,
            const char* syntax,
            const char* category,
            const char* description,
            const char* operation,
            const char* example
        )
        {
            entries.append({
                name,
                syntax,
                category,
                description,
                operation,
                example,
                false
            });
        };


    auto pseudo =
        [&](
            const char* name,
            const char* syntax,
            const char* category,
            const char* description,
            const char* operation,
            const char* example
        )
        {
            entries.append({
                name,
                syntax,
                category,
                description,
                operation,
                example,
                true
            });
        };


    // ========================================================
    // ARITHMETIC
    // ========================================================

    real(
        "ADD",
        "add $rd, $rs, $rt",
        "Arithmetic",
        "Signed 32-bit addition. MIPS Studio stops on signed overflow.",
        "$rd = signed($rs) + signed($rt)",
        "add $t0, $t1, $t2"
    );


    real(
        "ADDU",
        "addu $rd, $rs, $rt",
        "Arithmetic",
        "32-bit addition without signed-overflow trapping.",
        "$rd = $rs + $rt",
        "addu $t0, $t1, $t2"
    );


    real(
        "ADDI",
        "addi $rt, $rs, imm",
        "Arithmetic",
        "Adds a sign-extended 16-bit immediate and checks signed overflow.",
        "$rt = signed($rs) + sign_extend(imm)",
        "addi $t0, $t1, -4"
    );


    real(
        "ADDIU",
        "addiu $rt, $rs, imm",
        "Arithmetic",
        "Adds a sign-extended 16-bit immediate without overflow trapping.",
        "$rt = $rs + sign_extend(imm)",
        "addiu $sp, $sp, -16"
    );


    real(
        "SUB",
        "sub $rd, $rs, $rt",
        "Arithmetic",
        "Signed 32-bit subtraction with signed-overflow checking.",
        "$rd = signed($rs) - signed($rt)",
        "sub $t0, $t1, $t2"
    );


    real(
        "SUBU",
        "subu $rd, $rs, $rt",
        "Arithmetic",
        "32-bit subtraction without signed-overflow trapping.",
        "$rd = $rs - $rt",
        "subu $t0, $t1, $t2"
    );


    // ========================================================
    // LOGICAL
    // ========================================================

    real(
        "AND",
        "and $rd, $rs, $rt",
        "Logical",
        "Performs a bitwise AND.",
        "$rd = $rs & $rt",
        "and $t0, $t1, $t2"
    );


    real(
        "ANDI",
        "andi $rt, $rs, imm",
        "Logical",
        "Bitwise AND with a zero-extended 16-bit immediate.",
        "$rt = $rs & zero_extend(imm)",
        "andi $t0, $t1, 0x00FF"
    );


    real(
        "OR",
        "or $rd, $rs, $rt",
        "Logical",
        "Performs a bitwise OR.",
        "$rd = $rs | $rt",
        "or $t0, $t1, $t2"
    );


    real(
        "ORI",
        "ori $rt, $rs, imm",
        "Logical",
        "Bitwise OR with a zero-extended immediate.",
        "$rt = $rs | zero_extend(imm)",
        "ori $t0, $t1, 0x1234"
    );


    real(
        "XOR",
        "xor $rd, $rs, $rt",
        "Logical",
        "Performs bitwise exclusive OR.",
        "$rd = $rs ^ $rt",
        "xor $t0, $t1, $t2"
    );


    real(
        "XORI",
        "xori $rt, $rs, imm",
        "Logical",
        "Exclusive OR with a zero-extended immediate.",
        "$rt = $rs ^ zero_extend(imm)",
        "xori $t0, $t1, 0xFFFF"
    );


    real(
        "NOR",
        "nor $rd, $rs, $rt",
        "Logical",
        "Performs bitwise NOR.",
        "$rd = ~($rs | $rt)",
        "nor $t0, $t1, $t2"
    );


    // ========================================================
    // COMPARISON
    // ========================================================

    real(
        "SLT",
        "slt $rd, $rs, $rt",
        "Comparison",
        "Signed less-than comparison.",
        "$rd = signed($rs) < signed($rt) ? 1 : 0",
        "slt $t0, $t1, $t2"
    );


    real(
        "SLTU",
        "sltu $rd, $rs, $rt",
        "Comparison",
        "Unsigned less-than comparison.",
        "$rd = $rs < $rt ? 1 : 0",
        "sltu $t0, $t1, $t2"
    );


    real(
        "SLTI",
        "slti $rt, $rs, imm",
        "Comparison",
        "Signed less-than comparison with a sign-extended immediate.",
        "$rt = signed($rs) < sign_extend(imm) ? 1 : 0",
        "slti $t0, $t1, 10"
    );


    real(
        "SLTIU",
        "sltiu $rt, $rs, imm",
        "Comparison",
        "Unsigned comparison. The 16-bit immediate is sign-extended before comparison.",
        "$rt = $rs < unsigned(sign_extend(imm)) ? 1 : 0",
        "sltiu $t0, $t1, 10"
    );


    // ========================================================
    // SHIFTS
    // ========================================================

    real(
        "SLL",
        "sll $rd, $rt, shamt",
        "Shift",
        "Logical left shift by a constant amount from 0 to 31.",
        "$rd = $rt << shamt",
        "sll $t0, $t1, 2"
    );


    real(
        "SRL",
        "srl $rd, $rt, shamt",
        "Shift",
        "Logical right shift by a constant amount.",
        "$rd = $rt >> shamt",
        "srl $t0, $t1, 2"
    );


    real(
        "SRA",
        "sra $rd, $rt, shamt",
        "Shift",
        "Arithmetic right shift. The sign bit is propagated.",
        "$rd = signed($rt) >> shamt",
        "sra $t0, $t1, 2"
    );


    real(
        "SLLV",
        "sllv $rd, $rt, $rs",
        "Shift",
        "Logical left shift using the low five bits of rs as the shift amount.",
        "$rd = $rt << ($rs & 31)",
        "sllv $t0, $t1, $t2"
    );


    real(
        "SRLV",
        "srlv $rd, $rt, $rs",
        "Shift",
        "Logical right shift using the low five bits of rs.",
        "$rd = $rt >> ($rs & 31)",
        "srlv $t0, $t1, $t2"
    );


    real(
        "SRAV",
        "srav $rd, $rt, $rs",
        "Shift",
        "Arithmetic right shift using the low five bits of rs.",
        "$rd = signed($rt) >> ($rs & 31)",
        "srav $t0, $t1, $t2"
    );


    // ========================================================
    // MULTIPLY / DIVIDE
    // ========================================================

    real(
        "MULT",
        "mult $rs, $rt",
        "Multiply / Divide",
        "Signed 32x32 multiplication. The 64-bit result is stored in HI:LO.",
        "HI:LO = signed($rs) * signed($rt)",
        "mult $t0, $t1"
    );


    real(
        "MULTU",
        "multu $rs, $rt",
        "Multiply / Divide",
        "Unsigned 32x32 multiplication into HI:LO.",
        "HI:LO = unsigned($rs) * unsigned($rt)",
        "multu $t0, $t1"
    );


    real(
        "DIV",
        "div $rs, $rt",
        "Multiply / Divide",
        "Signed division. Quotient is written to LO and remainder to HI.",
        "LO = signed($rs) / signed($rt)\nHI = signed($rs) % signed($rt)",
        "div $t0, $t1"
    );


    real(
        "DIVU",
        "divu $rs, $rt",
        "Multiply / Divide",
        "Unsigned division. Quotient is written to LO and remainder to HI.",
        "LO = $rs / $rt\nHI = $rs % $rt",
        "divu $t0, $t1"
    );


    real(
        "MUL",
        "mul $rd, $rs, $rt",
        "Multiply / Divide",
        "MIPS32 SPECIAL2 multiply that writes the low 32 bits directly to rd.",
        "$rd = low32(signed($rs) * signed($rt))",
        "mul $t0, $t1, $t2"
    );


    real(
        "MFHI",
        "mfhi $rd",
        "Multiply / Divide",
        "Copies HI into a general-purpose register.",
        "$rd = HI",
        "mfhi $t0"
    );


    real(
        "MFLO",
        "mflo $rd",
        "Multiply / Divide",
        "Copies LO into a general-purpose register.",
        "$rd = LO",
        "mflo $t0"
    );


    real(
        "MTHI",
        "mthi $rs",
        "Multiply / Divide",
        "Copies a general-purpose register into HI.",
        "HI = $rs",
        "mthi $t0"
    );


    real(
        "MTLO",
        "mtlo $rs",
        "Multiply / Divide",
        "Copies a general-purpose register into LO.",
        "LO = $rs",
        "mtlo $t0"
    );


    // ========================================================
    // MOVE / BIT
    // ========================================================

    real(
        "MOVZ",
        "movz $rd, $rs, $rt",
        "Move / Bit",
        "Copies rs to rd only when rt is zero.",
        "if ($rt == 0) $rd = $rs",
        "movz $t0, $t1, $t2"
    );


    real(
        "MOVN",
        "movn $rd, $rs, $rt",
        "Move / Bit",
        "Copies rs to rd only when rt is nonzero.",
        "if ($rt != 0) $rd = $rs",
        "movn $t0, $t1, $t2"
    );


    real(
        "CLZ",
        "clz $rd, $rs",
        "Move / Bit",
        "Counts consecutive zero bits starting at bit 31.",
        "$rd = count_leading_zeros($rs)",
        "clz $t0, $t1"
    );


    real(
        "CLO",
        "clo $rd, $rs",
        "Move / Bit",
        "Counts consecutive one bits starting at bit 31.",
        "$rd = count_leading_ones($rs)",
        "clo $t0, $t1"
    );


    // ========================================================
    // IMMEDIATE
    // ========================================================

    real(
        "LUI",
        "lui $rt, imm",
        "Immediate",
        "Places a 16-bit immediate into the upper half of rt and clears the lower half.",
        "$rt = imm << 16",
        "lui $t0, 0x1234"
    );


    // ========================================================
    // MEMORY
    // ========================================================

    real(
        "LB",
        "lb $rt, offset($rs)",
        "Memory",
        "Loads one byte and sign-extends it to 32 bits.",
        "$rt = sign_extend8(Memory[$rs + sign_extend(offset)])",
        "lb $t0, 0($sp)"
    );


    real(
        "LBU",
        "lbu $rt, offset($rs)",
        "Memory",
        "Loads one byte and zero-extends it to 32 bits.",
        "$rt = zero_extend8(Memory[$rs + sign_extend(offset)])",
        "lbu $t0, 0($sp)"
    );


    real(
        "LH",
        "lh $rt, offset($rs)",
        "Memory",
        "Loads a 16-bit halfword and sign-extends it.",
        "$rt = sign_extend16(Memory16[$rs + sign_extend(offset)])",
        "lh $t0, 2($sp)"
    );


    real(
        "LHU",
        "lhu $rt, offset($rs)",
        "Memory",
        "Loads a halfword and zero-extends it.",
        "$rt = zero_extend16(Memory16[$rs + sign_extend(offset)])",
        "lhu $t0, 2($sp)"
    );


    real(
        "LW",
        "lw $rt, offset($rs)",
        "Memory",
        "Loads a 32-bit word.",
        "$rt = Memory32[$rs + sign_extend(offset)]",
        "lw $t0, 4($sp)"
    );


    real(
        "SB",
        "sb $rt, offset($rs)",
        "Memory",
        "Stores the low eight bits of rt.",
        "Memory8[$rs + sign_extend(offset)] = low8($rt)",
        "sb $t0, 0($sp)"
    );


    real(
        "SH",
        "sh $rt, offset($rs)",
        "Memory",
        "Stores the low sixteen bits of rt.",
        "Memory16[$rs + sign_extend(offset)] = low16($rt)",
        "sh $t0, 2($sp)"
    );


    real(
        "SW",
        "sw $rt, offset($rs)",
        "Memory",
        "Stores a full 32-bit word.",
        "Memory32[$rs + sign_extend(offset)] = $rt",
        "sw $t0, 4($sp)"
    );


    // ========================================================
    // BRANCHES
    // ========================================================

    real(
        "BEQ",
        "beq $rs, $rt, label",
        "Branch",
        "Branches when rs and rt are equal.",
        "if ($rs == $rt) PC = next_PC + offset * 4",
        "beq $t0, $t1, equal"
    );


    real(
        "BNE",
        "bne $rs, $rt, label",
        "Branch",
        "Branches when rs and rt are different.",
        "if ($rs != $rt) PC = next_PC + offset * 4",
        "bne $t0, $t1, different"
    );


    real(
        "BLEZ",
        "blez $rs, label",
        "Branch",
        "Signed branch when rs is less than or equal to zero.",
        "if (signed($rs) <= 0) branch",
        "blez $t0, done"
    );


    real(
        "BGTZ",
        "bgtz $rs, label",
        "Branch",
        "Signed branch when rs is greater than zero.",
        "if (signed($rs) > 0) branch",
        "bgtz $t0, loop"
    );


    real(
        "BLTZ",
        "bltz $rs, label",
        "Branch",
        "Branches when rs is negative.",
        "if (signed($rs) < 0) branch",
        "bltz $t0, negative"
    );


    real(
        "BGEZ",
        "bgez $rs, label",
        "Branch",
        "Branches when rs is zero or positive.",
        "if (signed($rs) >= 0) branch",
        "bgez $t0, nonnegative"
    );


    real(
        "BLTZAL",
        "bltzal $rs, label",
        "Branch",
        "Branches when rs is negative and links through $ra.",
        "if (signed($rs) < 0) { $ra = next_PC; branch; }",
        "bltzal $t0, function"
    );


    real(
        "BGEZAL",
        "bgezal $rs, label",
        "Branch",
        "Branches when rs is nonnegative and links through $ra.",
        "if (signed($rs) >= 0) { $ra = next_PC; branch; }",
        "bgezal $t0, function"
    );


    // ========================================================
    // JUMPS
    // ========================================================

    real(
        "J",
        "j target",
        "Jump",
        "Unconditionally jumps to a pseudo-direct target.",
        "PC = {next_PC[31:28], target[25:0], 00}",
        "j done"
    );


    real(
        "JAL",
        "jal target",
        "Jump",
        "Jumps and stores the return address in $ra. MIPS Studio currently has no delay slot.",
        "$ra = next_PC\nPC = target",
        "jal function"
    );


    real(
        "JR",
        "jr $rs",
        "Jump",
        "Jumps to an address stored in a register.",
        "PC = $rs",
        "jr $ra"
    );


    real(
        "JALR",
        "jalr [$rd,] $rs",
        "Jump",
        "Register-indirect jump and link. If rd is omitted, $ra is used.",
        "$rd = next_PC\nPC = $rs",
        "jalr $s0, $t0"
    );


    // ========================================================
    // SYSTEM
    // ========================================================

    real(
        "SYSCALL",
        "syscall [code]",
        "System",
        "Invokes a MIPS Studio syscall. $v0=1 prints an integer, $v0=4 prints a string and $v0=10 exits.",
        "service = $v0; arguments begin at $a0",
        "li $v0, 10\nsyscall"
    );


    real(
        "BREAK",
        "break [code]",
        "System",
        "Represents a software breakpoint exception. Until exception handling exists, MIPS Studio treats BREAK as a clean halt.",
        "halt execution",
        "break"
    );


    // ========================================================
    // PSEUDOS
    // ========================================================

    pseudo(
        "LI",
        "li $rt, value",
        "Immediate",
        "Loads a signed value or complete 32-bit bit pattern.",
        "small: addiu $rt, $zero, value\n"
        "large: lui $rt, upper16\n"
        "       ori $rt, $rt, lower16",
        "li $t0, 0x12345678"
    );


    pseudo(
        "LA",
        "la $rt, label",
        "Immediate",
        "Loads a label address.",
        "lui $rt, upper16(address)\n"
        "ori $rt, $rt, lower16(address)",
        "la $a0, message"
    );


    pseudo(
        "MOVE",
        "move $rd, $rs",
        "Move / Bit",
        "Copies a register.",
        "addu $rd, $rs, $zero",
        "move $s0, $t0"
    );


    pseudo(
        "CLEAR",
        "clear $rd",
        "Move / Bit",
        "Sets a register to zero.",
        "addu $rd, $zero, $zero",
        "clear $t0"
    );


    pseudo(
        "NOP",
        "nop",
        "System",
        "Does not modify architectural state.",
        "sll $zero, $zero, 0",
        "nop"
    );


    pseudo(
        "B",
        "b label",
        "Branch",
        "Unconditional PC-relative branch.",
        "beq $zero, $zero, label",
        "b loop"
    );


    pseudo(
        "BAL",
        "bal label",
        "Branch",
        "Unconditional PC-relative branch and link.",
        "bgezal $zero, label",
        "bal function"
    );


    pseudo(
        "BEQZ",
        "beqz $rs, label",
        "Branch",
        "Branches when rs equals zero.",
        "beq $rs, $zero, label",
        "beqz $t0, done"
    );


    pseudo(
        "BNEZ",
        "bnez $rs, label",
        "Branch",
        "Branches when rs is nonzero.",
        "bne $rs, $zero, label",
        "bnez $t0, loop"
    );


    pseudo(
        "BLT",
        "blt $rs, $rt, label",
        "Branch",
        "Signed branch if rs is less than rt. Uses $at.",
        "slt $at, $rs, $rt\n"
        "bne $at, $zero, label",
        "blt $t0, $t1, smaller"
    );


    pseudo(
        "BLE",
        "ble $rs, $rt, label",
        "Branch",
        "Signed branch if rs is less than or equal to rt. Uses $at.",
        "slt $at, $rt, $rs\n"
        "beq $at, $zero, label",
        "ble $t0, $t1, ok"
    );


    pseudo(
        "BGT",
        "bgt $rs, $rt, label",
        "Branch",
        "Signed branch if rs is greater than rt. Uses $at.",
        "slt $at, $rt, $rs\n"
        "bne $at, $zero, label",
        "bgt $t0, $t1, greater"
    );


    pseudo(
        "BGE",
        "bge $rs, $rt, label",
        "Branch",
        "Signed branch if rs is greater than or equal to rt. Uses $at.",
        "slt $at, $rs, $rt\n"
        "beq $at, $zero, label",
        "bge $t0, $t1, ok"
    );


    pseudo(
        "BLTU",
        "bltu $rs, $rt, label",
        "Branch",
        "Unsigned branch if rs is less than rt. Uses $at.",
        "sltu $at, $rs, $rt\n"
        "bne $at, $zero, label",
        "bltu $t0, $t1, smaller"
    );


    pseudo(
        "BLEU",
        "bleu $rs, $rt, label",
        "Branch",
        "Unsigned branch if rs is less than or equal to rt. Uses $at.",
        "sltu $at, $rt, $rs\n"
        "beq $at, $zero, label",
        "bleu $t0, $t1, ok"
    );


    pseudo(
        "BGTU",
        "bgtu $rs, $rt, label",
        "Branch",
        "Unsigned branch if rs is greater than rt. Uses $at.",
        "sltu $at, $rt, $rs\n"
        "bne $at, $zero, label",
        "bgtu $t0, $t1, greater"
    );


    pseudo(
        "BGEU",
        "bgeu $rs, $rt, label",
        "Branch",
        "Unsigned branch if rs is greater than or equal to rt. Uses $at.",
        "sltu $at, $rs, $rt\n"
        "beq $at, $zero, label",
        "bgeu $t0, $t1, ok"
    );


    pseudo(
        "NEG",
        "neg $rd, $rs",
        "Arithmetic",
        "Signed arithmetic negation.",
        "sub $rd, $zero, $rs",
        "neg $t0, $t1"
    );


    pseudo(
        "NEGU",
        "negu $rd, $rs",
        "Arithmetic",
        "Two's-complement negation without overflow trapping.",
        "subu $rd, $zero, $rs",
        "negu $t0, $t1"
    );


    pseudo(
        "NOT",
        "not $rd, $rs",
        "Logical",
        "Bitwise inversion.",
        "nor $rd, $rs, $zero",
        "not $t0, $t1"
    );


    pseudo(
        "SEQ",
        "seq $rd, $rs, $rt",
        "Comparison",
        "Sets rd to one when rs equals rt.",
        "xor $rd, $rs, $rt\n"
        "sltiu $rd, $rd, 1",
        "seq $t0, $t1, $t2"
    );


    pseudo(
        "SNE",
        "sne $rd, $rs, $rt",
        "Comparison",
        "Sets rd to one when rs and rt are different.",
        "xor $rd, $rs, $rt\n"
        "sltu $rd, $zero, $rd",
        "sne $t0, $t1, $t2"
    );


    pseudo(
        "SGT",
        "sgt $rd, $rs, $rt",
        "Comparison",
        "Signed greater-than comparison.",
        "slt $rd, $rt, $rs",
        "sgt $t0, $t1, $t2"
    );


    pseudo(
        "SGE",
        "sge $rd, $rs, $rt",
        "Comparison",
        "Signed greater-than-or-equal comparison.",
        "slt $rd, $rs, $rt\n"
        "xori $rd, $rd, 1",
        "sge $t0, $t1, $t2"
    );


    pseudo(
        "SLE",
        "sle $rd, $rs, $rt",
        "Comparison",
        "Signed less-than-or-equal comparison.",
        "slt $rd, $rt, $rs\n"
        "xori $rd, $rd, 1",
        "sle $t0, $t1, $t2"
    );


    pseudo(
        "SGTU",
        "sgtu $rd, $rs, $rt",
        "Comparison",
        "Unsigned greater-than comparison.",
        "sltu $rd, $rt, $rs",
        "sgtu $t0, $t1, $t2"
    );


    pseudo(
        "SGEU",
        "sgeu $rd, $rs, $rt",
        "Comparison",
        "Unsigned greater-than-or-equal comparison.",
        "sltu $rd, $rs, $rt\n"
        "xori $rd, $rd, 1",
        "sgeu $t0, $t1, $t2"
    );


    pseudo(
        "SLEU",
        "sleu $rd, $rs, $rt",
        "Comparison",
        "Unsigned less-than-or-equal comparison.",
        "sltu $rd, $rt, $rs\n"
        "xori $rd, $rd, 1",
        "sleu $t0, $t1, $t2"
    );


    pseudo(
        "REM",
        "rem $rd, $rs, $rt",
        "Multiply / Divide",
        "Signed remainder convenience instruction.",
        "div $rs, $rt\n"
        "mfhi $rd",
        "rem $t0, $t1, $t2"
    );


    pseudo(
        "REMU",
        "remu $rd, $rs, $rt",
        "Multiply / Divide",
        "Unsigned remainder convenience instruction.",
        "divu $rs, $rt\n"
        "mfhi $rd",
        "remu $t0, $t1, $t2"
    );


    return entries;
}
