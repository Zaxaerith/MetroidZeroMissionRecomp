// fixed_multiplication.h — Human-readable C++ replacement for FixedMultiplication (0x08000A4C).
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>

namespace mzm::native {

/**
 * @brief Zero Mission FixedMultiplication Q8.8 fixed-point multiplication.
 * 
 * Replaces Thumb function at ROM 0x08000A4C (28 bytes).
 * Exactly mirrors upstream C semantics:
 *   res = (s32)a * (s32)b;
 *   res = (s32)(res / 256); // C99 truncation towards zero
 *   return (s16)res;        // Narrowing cast to signed 16-bit
 * 
 * @param a First signed 16-bit operand
 * @param b Second signed 16-bit operand
 * @return int16_t Result truncated towards zero and narrowed to s16
 */
int16_t fixed_multiplication(int16_t a, int16_t b) noexcept;

} // namespace mzm::native
