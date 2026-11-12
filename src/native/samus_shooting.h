// samus_shooting.h — Human-readable C++ replacement for SamusShootingGfx (0x08008940).
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>
#include "samus_animation.h"
#include "samus_turning.h"

namespace mzm::native {

/**
 * @brief Handles Samus graphical update when shooting (also landing and uncrouching).
 *
 * Replaces Thumb function at ROM 0x08008940 (28 bytes).
 * Exactly mirrors upstream C semantics in src/samus.c:4203:
 *   SamusPose SamusShootingGfx(struct SamusData* pData)
 *
 * Calls SamusUpdateAnimation(pData, FALSE); if sequence ended (SAMUS_ANIM_STATE_ENDED),
 * returns SPOSE_STANDING (1); otherwise returns SPOSE_NONE (0xFF).
 * Optionally records anim_state to out_anim_state for accurate CPSR C/V thunk restoration.
 */
uint8_t samus_shooting_gfx(uint32_t p_data_addr, uint8_t* out_anim_state = nullptr) noexcept;

} // namespace mzm::native
