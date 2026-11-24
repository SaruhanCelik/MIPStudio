#include "ALU.h"

#include <climits>
#include <cstdint>


ALUResult ALU::execute(
    uint32_t op1,
    uint32_t op2,
    ALUOperation operation
)
{
    ALUResult result{};


    switch (
        operation
    ) {

        // ====================================================
        // SIGNED ADD
        // ====================================================

        case ALUOperation::ADD: {
            int64_t temp =
                static_cast<int64_t>(
                    static_cast<int32_t>(
                        op1
                    )
                )
                +
                static_cast<int64_t>(
                    static_cast<int32_t>(
                        op2
                    )
                );


            if (
                temp > INT32_MAX ||
                temp < INT32_MIN
            ) {
                result.overflow =
                    true;
            }
            else {
                result.value =
                    static_cast<uint32_t>(
                        static_cast<int32_t>(
                            temp
                        )
                    );
            }


            break;
        }


        // ====================================================
        // UNSIGNED ADD
        // ====================================================

        case ALUOperation::ADDU:

            result.value =
                op1 +
                op2;

            break;


        // ====================================================
        // SIGNED SUB
        // ====================================================

        case ALUOperation::SUB: {
            int64_t temp =
                static_cast<int64_t>(
                    static_cast<int32_t>(
                        op1
                    )
                )
                -
                static_cast<int64_t>(
                    static_cast<int32_t>(
                        op2
                    )
                );


            if (
                temp > INT32_MAX ||
                temp < INT32_MIN
            ) {
                result.overflow =
                    true;
            }
            else {
                result.value =
                    static_cast<uint32_t>(
                        static_cast<int32_t>(
                            temp
                        )
                    );
            }


            break;
        }


        case ALUOperation::SUBU:

            result.value =
                op1 -
                op2;

            break;


        // ====================================================
        // LOGICAL
        // ====================================================

        case ALUOperation::AND:

            result.value =
                op1 &
                op2;

            break;


        case ALUOperation::OR:

            result.value =
                op1 |
                op2;

            break;


        case ALUOperation::XOR:

            result.value =
                op1 ^
                op2;

            break;


        case ALUOperation::NOR:

            result.value =
                ~(
                    op1 |
                    op2
                );

            break;


        // ====================================================
        // COMPARISON
        // ====================================================

        case ALUOperation::SLT:

            result.value =
                static_cast<int32_t>(
                    op1
                )
                <
                static_cast<int32_t>(
                    op2
                )
                ? 1U
                : 0U;

            break;


        case ALUOperation::SLTU:

            result.value =
                op1 <
                op2
                ? 1U
                : 0U;

            break;


        // ====================================================
        // SHIFTS
        // ====================================================

        case ALUOperation::SLL:

            result.value =
                op2 <<
                (
                    op1 &
                    0x1FU
                );

            break;


        case ALUOperation::SRL:

            result.value =
                op2 >>
                (
                    op1 &
                    0x1FU
                );

            break;


        case ALUOperation::SRA:

            result.value =
                static_cast<uint32_t>(
                    static_cast<int32_t>(
                        op2
                    )
                    >>
                    (
                        op1 &
                        0x1FU
                    )
                );

            break;


        // ====================================================
        // MUL LOW 32
        // ====================================================

        case ALUOperation::MUL: {
            int64_t temp =
                static_cast<int64_t>(
                    static_cast<int32_t>(
                        op1
                    )
                )
                *
                static_cast<int64_t>(
                    static_cast<int32_t>(
                        op2
                    )
                );


            result.value =
                static_cast<uint32_t>(
                    static_cast<uint64_t>(
                        temp
                    )
                    &
                    0xFFFFFFFFULL
                );


            break;
        }


        case ALUOperation::NONE:

            break;
    }


    result.zero =
        result.value ==
        0;


    return result;
}