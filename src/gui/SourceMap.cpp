#include "gui/SourceMap.h"

#include <QRegularExpression>
#include <QStringList>


namespace {

QString stripComment(
    const QString& source
)
{
    bool insideString =
        false;


    bool escaped =
        false;


    for (
        int i = 0;
        i < source.size();
        ++i
    ) {
        const QChar c =
            source[i];


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
            return source.left(
                i
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


    return source;
}


QString firstToken(
    const QString& source
)
{
    const QString trimmed =
        source.trimmed();


    if (
        trimmed.isEmpty()
    ) {
        return {};
    }


    const int separator =
        trimmed.indexOf(
            QRegularExpression(
                R"([\s,]+)"
            )
        );


    return
        separator < 0
        ? trimmed
        : trimmed.left(
              separator
          );
}


// ------------------------------------------------------------
// Supports:
// main:
//
// and:
// main: addiu $t0, $zero, 1
// ------------------------------------------------------------

QString stripLeadingLabels(
    QString line,
    QStringList* labels = nullptr
)
{
    static const QRegularExpression labelPattern(
        R"(^\s*([A-Za-z_.$][A-Za-z0-9_.$]*)\s*:\s*)"
    );


    while (
        true
    ) {
        QRegularExpressionMatch match =
            labelPattern.match(
                line
            );


        if (!match.hasMatch()) {
            break;
        }


        if (
            labels
        ) {
            labels->append(
                match.captured(
                    1
                )
            );
        }


        line.remove(
            0,
            match.capturedLength()
        );
    }


    return line.trimmed();
}


// ------------------------------------------------------------
// Match assembler LI parsing.
//
// 0xFFFFFFFF is a legal 32-bit bit pattern and represents -1.
// ------------------------------------------------------------

bool parseMipsInt32(
    const QString& text,
    qint32& value
)
{
    bool ok =
        false;


    qlonglong parsed =
        text.toLongLong(
            &ok,
            0
        );


    if (!ok) {
        return false;
    }


    if (
        parsed >= INT32_MIN &&
        parsed <= INT32_MAX
    ) {
        value =
            static_cast<qint32>(
                parsed
            );

        return true;
    }


    if (
        parsed >= 0 &&
        static_cast<qulonglong>(
            parsed
        ) <= UINT32_MAX
    ) {
        value =
            static_cast<qint32>(
                static_cast<quint32>(
                    parsed
                )
            );

        return true;
    }


    return false;
}

}


// ============================================================
// CLEAR
// ============================================================

void SourceMap::clear()
{
    lineToAddress.clear();

    addressToLine.clear();

    symbolList.clear();
}


// ============================================================
// REBUILD
// ============================================================

void SourceMap::rebuild(
    const QString& source
)
{
    clear();


    enum class Section {
        TEXT,
        DATA
    };


    Section section =
        Section::TEXT;


    uint32_t textAddress =
        TEXT_START;


    uint32_t dataAddress =
        DATA_START;


    const QStringList lines =
        source.split(
            '\n'
        );


    for (
        int index = 0;
        index < lines.size();
        ++index
    ) {
        const int sourceLine =
            index + 1;


        QString line =
            stripComment(
                lines[index]
            )
            .trimmed();


        if (
            line.isEmpty()
        ) {
            continue;
        }


        QStringList labels;


        line =
            stripLeadingLabels(
                line,
                &labels
            );


        // ----------------------------------------------------
        // Add symbols before advancing addresses.
        // ----------------------------------------------------

        for (
            const QString& label :
            labels
        ) {
            SourceSymbol symbol;


            symbol.name =
                label;


            symbol.line =
                sourceLine;


            if (
                section ==
                Section::TEXT
            ) {
                symbol.address =
                    textAddress;


                symbol.section =
                    ".text";
            }
            else {
                symbol.address =
                    dataAddress;


                symbol.section =
                    ".data";
            }


            symbolList.append(
                symbol
            );
        }


        if (
            line.isEmpty()
        ) {
            continue;
        }


        if (
            line.compare(
                ".text",
                Qt::CaseInsensitive
            )
            ==
            0
        ) {
            section =
                Section::TEXT;

            continue;
        }


        if (
            line.compare(
                ".data",
                Qt::CaseInsensitive
            )
            ==
            0
        ) {
            section =
                Section::DATA;

            continue;
        }


        // ====================================================
        // TEXT
        // ====================================================

        if (
            section ==
            Section::TEXT
        ) {
            lineToAddress[
                sourceLine
            ] = textAddress;


            const uint32_t size =
                instructionSize(
                    line
                );


            // Every machine instruction produced by a pseudo
            // points back to its original source line.

            for (
                uint32_t offset = 0;
                offset < size;
                offset += 4
            ) {
                addressToLine[
                    textAddress +
                    offset
                ] = sourceLine;
            }


            textAddress +=
                size;


            continue;
        }


        // ====================================================
        // DATA
        // ====================================================

        dataAddress +=
            dataSize(
                line
            );
    }
}


// ============================================================
// INSTRUCTION SIZE
// ============================================================

uint32_t SourceMap::instructionSize(
    const QString& line
) const
{
    const QString op =
        firstToken(
            line
        )
        .toLower();


    if (
        op ==
        "la"
    ) {
        return 8;
    }


    if (
        op ==
        "li"
    ) {
        QString working =
            line;


        working.replace(
            ',',
            ' '
        );


        const QStringList parts =
            working.split(
                QRegularExpression(
                    R"(\s+)"
                ),
                Qt::SkipEmptyParts
            );


        if (
            parts.size() >=
            3
        ) {
            qint32 value =
                0;


            if (
                parseMipsInt32(
                    parts[2],
                    value
                )
            ) {
                return
                    value >= INT16_MIN &&
                    value <= INT16_MAX
                    ? 4
                    : 8;
            }
        }


        return 4;
    }


    static const QStringList twoInstructionPseudos = {
        "blt",
        "ble",
        "bgt",
        "bge",

        "bltu",
        "bleu",
        "bgtu",
        "bgeu",

        "seq",
        "sne",

        "sge",
        "sle",

        "sgeu",
        "sleu",

        "rem",
        "remu"
    };


    if (
        twoInstructionPseudos.contains(
            op
        )
    ) {
        return 8;
    }


    return 4;
}


// ============================================================
// DATA SIZE
// ============================================================

uint32_t SourceMap::dataSize(
    const QString& line
) const
{
    const QString lower =
        line.toLower();


    if (
        lower.startsWith(
            ".word"
        )
    ) {
        QString working =
            line;


        working.replace(
            ',',
            ' '
        );


        QStringList parts =
            working.split(
                QRegularExpression(
                    R"(\s+)"
                ),
                Qt::SkipEmptyParts
            );


        if (
            parts.size() <=
            1
        ) {
            return 0;
        }


        return static_cast<uint32_t>(
            (
                parts.size() -
                1
            )
            *
            4
        );
    }


    if (
        lower.startsWith(
            ".asciiz"
        )
    ) {
        const int firstQuote =
            line.indexOf(
                '"'
            );


        const int lastQuote =
            line.lastIndexOf(
                '"'
            );


        if (
            firstQuote >= 0 &&
            lastQuote > firstQuote
        ) {
            const QString contents =
                line.mid(
                    firstQuote + 1,

                    lastQuote -
                    firstQuote -
                    1
                );


            return static_cast<uint32_t>(
                contents.toUtf8().size() +
                1
            );
        }
    }


    return 0;
}


// ============================================================
// LOOKUPS
// ============================================================

bool SourceMap::hasAddressForLine(
    int line
) const
{
    return lineToAddress.contains(
        line
    );
}


uint32_t SourceMap::addressForLine(
    int line
) const
{
    return lineToAddress.value(
        line,
        0
    );
}


int SourceMap::lineForAddress(
    uint32_t address
) const
{
    return addressToLine.value(
        address,
        -1
    );
}


const QVector<SourceSymbol>&
SourceMap::symbols() const
{
    return symbolList;
}