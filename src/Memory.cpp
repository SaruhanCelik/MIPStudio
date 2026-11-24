#include "DataMemory.h"

#include <iomanip>
#include <iostream>


// ============================================================
// CONSTRUCTOR
// ============================================================

Memory::Memory()
{
    for (
        uint32_t i = 0;
        i < MEMORY_SIZE;
        ++i
    ) {
        data[i] = 0;
    }
}


// ============================================================
// RANGE CHECK
// ============================================================

bool Memory::rangeValid(
    uint32_t address,
    uint32_t size
) const
{
    if (
        size == 0 ||
        size > MEMORY_SIZE
    ) {
        return false;
    }


    return
        address < MEMORY_SIZE &&
        address <= MEMORY_SIZE - size;
}


// ============================================================
// BYTE READ
// ============================================================

uint8_t Memory::memReadByte(
    uint32_t address
)
{
    if (!rangeValid(
            address,
            1
        )) {
        std::cout
            << "Memory read address is out of boundaries!\n";

        return 0;
    }


    return data[address];
}


// ============================================================
// HALFWORD READ
// ============================================================

uint16_t Memory::memRead16(
    uint32_t address
)
{
    if (!rangeValid(
            address,
            2
        )) {
        std::cout
            << "Memory read address is out of boundaries!\n";

        return 0;
    }


    // Big-endian:
    //
    // address     = high byte
    // address + 1 = low byte

    return static_cast<uint16_t>(

        (
            static_cast<uint16_t>(
                data[address]
            )
            << 8
        )

        |

        static_cast<uint16_t>(
            data[address + 1]
        )
    );
}


// ============================================================
// WORD READ
// ============================================================

uint32_t Memory::memRead32(
    uint32_t address
)
{
    if (!rangeValid(
            address,
            4
        )) {
        std::cout
            << "Memory read address is out of boundaries!\n";

        return 0;
    }


    return
        (
            static_cast<uint32_t>(
                data[address]
            )
            << 24
        )
        |
        (
            static_cast<uint32_t>(
                data[address + 1]
            )
            << 16
        )
        |
        (
            static_cast<uint32_t>(
                data[address + 2]
            )
            << 8
        )
        |
        static_cast<uint32_t>(
            data[address + 3]
        );
}


// ============================================================
// BYTE WRITE
// ============================================================

void Memory::memWriteByte(
    uint32_t address,
    uint8_t value
)
{
    if (!rangeValid(
            address,
            1
        )) {
        std::cout
            << "Memory write address is out of boundaries!\n";

        return;
    }


    data[address] =
        value;
}


// ============================================================
// HALFWORD WRITE
// ============================================================

void Memory::memWrite16(
    uint32_t address,
    uint16_t value
)
{
    if (!rangeValid(
            address,
            2
        )) {
        std::cout
            << "Memory write address is out of boundaries!\n";

        return;
    }


    data[address] =
        static_cast<uint8_t>(
            (value >> 8) &
            0xFF
        );


    data[address + 1] =
        static_cast<uint8_t>(
            value &
            0xFF
        );
}


// ============================================================
// WORD WRITE
// ============================================================

void Memory::memWrite32(
    uint32_t address,
    uint32_t value
)
{
    if (!rangeValid(
            address,
            4
        )) {
        std::cout
            << "Memory write address is out of boundaries!\n";

        return;
    }


    data[address] =
        static_cast<uint8_t>(
            (value >> 24) &
            0xFF
        );


    data[address + 1] =
        static_cast<uint8_t>(
            (value >> 16) &
            0xFF
        );


    data[address + 2] =
        static_cast<uint8_t>(
            (value >> 8) &
            0xFF
        );


    data[address + 3] =
        static_cast<uint8_t>(
            value &
            0xFF
        );
}


// ============================================================
// PRINT BYTE
// ============================================================

void Memory::memPrintByteValue(
    uint32_t address
)
{
    std::cout
        << "address: "
        << address
        << "       0x"
        << std::hex
        << std::uppercase
        << std::setw(2)
        << std::setfill('0')
        << static_cast<int>(
            memReadByte(address)
        )
        << std::dec
        << std::setfill(' ')
        << std::endl;
}


// ============================================================
// DEBUG 32-BIT REGION
// ============================================================

void Memory::memTest32(
    uint32_t address
)
{
    std::cout
        << "Address     Value"
        << std::endl;


    for (
        uint32_t offset = 0;
        offset < 4;
        ++offset
    ) {
        std::cout
            << "0x"
            << std::uppercase
            << std::hex
            << std::setw(4)
            << std::setfill('0')
            << address + offset
            << "      0x"
            << std::setw(2)
            << static_cast<int>(
                memReadByte(
                    address +
                    offset
                )
            )
            << std::endl;
    }


    std::cout
        << std::dec
        << std::setfill(' ');
}