// fixed_multiplication.cpp — Human-readable C++ implementation for FixedMultiplication.
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include "fixed_multiplication.h"

namespace mzm::native {

int16_t fixed_multiplication(int16_t a, int16_t b) noexcept {
    // 1. Promote 16-bit operands to 32-bit signed integers
    const int32_t x = static_cast<int32_t>(a);
    const int32_t y = static_cast<int32_t>(b);

    // 2. Perform signed 32-bit multiplication
    const int32_t product = x * y;

    // 3. Fixed-point scaling by 256.
    // Under C++11, integer division strictly truncates towards zero.
    // This exactly matches upstream macro Q_8_8_TO_S32(n) ((s32)((n) / 256))
    // and the Thumb assembly sequence: cmp r0, #0; bge +1; adds r1, #255.
    const int32_t scaled = product / 256;

    // 4. Narrowing cast to 16-bit signed integer
    return static_cast<int16_t>(scaled);
}

} // namespace mzm::native
