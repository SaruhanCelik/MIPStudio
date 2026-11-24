#include "gui/MipsSyntaxHighlighter.h"

#include <QFont>


MipsSyntaxHighlighter::MipsSyntaxHighlighter(
    QTextDocument* parent
)
    : QSyntaxHighlighter(parent)
{
    currentPalette.normalText =
        QColor("#E6EDF3");


    currentPalette.instruction =
        QColor("#79C0FF");


    currentPalette.reg =
        QColor("#D2A8FF");


    currentPalette.directive =
        QColor("#FFA657");


    currentPalette.label =
        QColor("#7EE787");


    currentPalette.comment =
        QColor("#8B949E");


    currentPalette.string =
        QColor("#A5D6FF");


    currentPalette.number =
        QColor("#FF7B72");


    rebuildRules();
}


// ============================================================
// PALETTE
// ============================================================

void MipsSyntaxHighlighter::setPalette(
    const SyntaxPalette& palette
)
{
    currentPalette =
        palette;


    rebuildRules();


    rehighlight();
}


// ============================================================
// RULES
// ============================================================

void MipsSyntaxHighlighter::rebuildRules()
{
    rules.clear();


    QTextCharFormat instructionFormat;

    instructionFormat.setForeground(
        currentPalette.instruction
    );

    instructionFormat.setFontWeight(
        QFont::DemiBold
    );


    QTextCharFormat registerFormat;

    registerFormat.setForeground(
        currentPalette.reg
    );


    QTextCharFormat directiveFormat;

    directiveFormat.setForeground(
        currentPalette.directive
    );

    directiveFormat.setFontWeight(
        QFont::DemiBold
    );


    QTextCharFormat labelFormat;

    labelFormat.setForeground(
        currentPalette.label
    );

    labelFormat.setFontWeight(
        QFont::DemiBold
    );


    QTextCharFormat commentFormat;

    commentFormat.setForeground(
        currentPalette.comment
    );

    commentFormat.setFontItalic(
        true
    );


    QTextCharFormat stringFormat;

    stringFormat.setForeground(
        currentPalette.string
    );


    QTextCharFormat numberFormat;

    numberFormat.setForeground(
        currentPalette.number
    );


    // ========================================================
    // ALL REAL + PSEUDO INSTRUCTIONS
    // ========================================================

    rules.append({
        QRegularExpression(
            R"(\b(add|addu|addi|addiu|sub|subu|and|andi|or|ori|xor|xori|nor|slt|sltu|slti|sltiu|sll|srl|sra|sllv|srlv|srav|mult|multu|div|divu|mul|mfhi|mflo|mthi|mtlo|movz|movn|clz|clo|lui|lb|lbu|lh|lhu|lw|sb|sh|sw|beq|bne|blez|bgtz|bltz|bgez|bltzal|bgezal|j|jal|jalr|jr|syscall|break|li|la|move|clear|nop|b|bal|beqz|bnez|blt|ble|bgt|bge|bltu|bleu|bgtu|bgeu|neg|negu|not|seq|sne|sgt|sge|sle|sgtu|sgeu|sleu|rem|remu)\b)",
            QRegularExpression::CaseInsensitiveOption
        ),
        instructionFormat
    });


    // ========================================================
    // REGISTERS
    // ========================================================

    rules.append({
        QRegularExpression(
            R"(\$(zero|at|v[01]|a[0-3]|t[0-9]|s[0-8]|k[01]|gp|sp|fp|ra|[0-9]|[12][0-9]|3[01])\b)",
            QRegularExpression::CaseInsensitiveOption
        ),
        registerFormat
    });


    // ========================================================
    // DIRECTIVES
    // ========================================================

    rules.append({
        QRegularExpression(
            R"(\.(text|data|word|asciiz)\b)",
            QRegularExpression::CaseInsensitiveOption
        ),
        directiveFormat
    });


    // ========================================================
    // LABEL DEFINITIONS
    // ========================================================

    rules.append({
        QRegularExpression(
            R"(\b[A-Za-z_.$][A-Za-z0-9_.$]*\s*:)"
        ),
        labelFormat
    });


    // ========================================================
    // NUMBERS
    // ========================================================

    rules.append({
        QRegularExpression(
            R"((?<![\w])[-+]?(?:0[xX][0-9A-Fa-f]+|\d+)\b)"
        ),
        numberFormat
    });


    // ========================================================
    // STRINGS
    // ========================================================

    rules.append({
        QRegularExpression(
            R"("([^"\\]|\\.)*")"
        ),
        stringFormat
    });


    // Comments intentionally come LAST so they override
    // instruction/register-looking text after '#'.

    rules.append({
        QRegularExpression(
            R"(#.*$)"
        ),
        commentFormat
    });
}


// ============================================================
// HIGHLIGHT
// ============================================================

void MipsSyntaxHighlighter::highlightBlock(
    const QString& text
)
{
    for (
        const Rule& rule :
        rules
    ) {
        QRegularExpressionMatchIterator iterator =
            rule.pattern.globalMatch(
                text
            );


        while (
            iterator.hasNext()
        ) {
            const QRegularExpressionMatch match =
                iterator.next();


            setFormat(
                match.capturedStart(),
                match.capturedLength(),
                rule.format
            );
        }
    }
}