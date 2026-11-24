#ifndef MIPSSTUDIO_SOURCEMAP_H
#define MIPSSTUDIO_SOURCEMAP_H

#include <cstdint>

#include <QMap>
#include <QString>
#include <QVector>


struct SourceSymbol {
    QString name;

    uint32_t address =
        0;

    QString section;

    int line =
        -1;
};


// ============================================================
// SOURCE MAP
// ------------------------------------------------------------
// Maps source lines to real machine-code addresses.
//
// This must mirror assembler pseudo expansion.
//
// Example:
//
// blt $t0, $t1, label
//
// expands to two instructions, therefore one source line owns
// two addresses.
// ============================================================

class SourceMap {

public:
    void rebuild(
        const QString& source
    );


    void clear();


    bool hasAddressForLine(
        int line
    ) const;


    uint32_t addressForLine(
        int line
    ) const;


    int lineForAddress(
        uint32_t address
    ) const;


    const QVector<SourceSymbol>&
    symbols() const;


private:
    QMap<int, uint32_t>
        lineToAddress;


    QMap<uint32_t, int>
        addressToLine;


    QVector<SourceSymbol>
        symbolList;


    static constexpr uint32_t TEXT_START =
        0x00000000;


    static constexpr uint32_t DATA_START =
        0x00001000;


    uint32_t instructionSize(
        const QString& line
    ) const;


    uint32_t dataSize(
        const QString& line
    ) const;
};


#endif