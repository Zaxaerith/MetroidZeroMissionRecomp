// samus_standing.h — Human-readable C++ replacement for SamusStandingGfx (0x0800881C).
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>
#include "samus_animation.h"
#include "samus_turning.h"

namespace mzm::native {

/**
 * @brief Handles Samus graphical update when standing, looping animation when it ends.
 *
 * Replaces Thumb function at ROM 0x0800881C (32 bytes).
 * Exactly mirrors upstream C semantics in src/samus.c:4115:
 *   SamusPose SamusStandingGfx(struct SamusData* pData)
 *
 * Calls SamusUpdateAnimation(pData, FALSE); if sequence ended, resets
 * currentAnimationFrame to 0. Always returns SPOSE_NONE (0xFF).
 */
uint8_t samus_standing_gfx(uint32_t p_data_addr) noexcept;

} // namespace mzm::native
