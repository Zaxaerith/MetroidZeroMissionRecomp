// samus_shooting_crouching.h — Human-readable C++ replacement for SamusShootingAndCrouchingGfx (0x08008C50).
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>
#include "samus_animation.h"
#include "samus_turning.h"

namespace mzm::native {

constexpr uint8_t SPOSE_CROUCHING = 4u;

/**
 * @brief Handles Samus graphical update when crouching and shooting.
 *
 * Replaces Thumb function at ROM 0x08008C50 (28 bytes).
 * Exactly mirrors upstream C semantics in src/samus.c:4440:
 *   SamusPose SamusShootingAndCrouchingGfx(struct SamusData* pData)
 *
 * Calls SamusUpdateAnimation(pData, FALSE); if sequence ended (SAMUS_ANIM_STATE_ENDED = 2),
 * returns SPOSE_CROUCHING (4); otherwise returns SPOSE_NONE (0xFF).
 * Optionally records anim_state to out_anim_state for accurate CPSR C/V thunk restoration.
 */
uint8_t samus_shooting_and_crouching_gfx(uint32_t p_data_addr, uint8_t* out_anim_state = nullptr) noexcept;

} // namespace mzm::native
