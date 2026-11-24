#ifndef DATAMEMORY_H
#define DATAMEMORY_H

#include <cstdint>


// ============================================================
// MEMORY
// ------------------------------------------------------------
// 64 KiB byte-addressable memory.
//
// The emulator has historically used big-endian word storage,
// so byte / halfword / word operations continue to use the
// same byte order.
// ============================================================

struct Memory {

private:
    static constexpr uint32_t MEMORY_SIZE =
        65536;


    uint8_t data[
        MEMORY_SIZE
    ];


    bool rangeValid(
        uint32_t address,
        uint32_t size
    ) const;


public:
    Memory();


    // ========================================================
    // READ
    // ========================================================

    uint8_t memReadByte(
        uint32_t address
    );


    uint16_t memRead16(
        uint32_t address
    );


    uint32_t memRead32(
        uint32_t address
    );


    // ========================================================
    // WRITE
    // ========================================================

    void memWriteByte(
        uint32_t address,
        uint8_t value
    );


    void memWrite16(
        uint32_t address,
        uint16_t value
    );


    void memWrite32(
        uint32_t address,
        uint32_t value
    );


    // ========================================================
    // DEBUG HELPERS
    // ========================================================

    void memPrintByteValue(
        uint32_t address
    );


    void memTest32(
        uint32_t address
    );
};


#endif