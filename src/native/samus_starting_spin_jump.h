// samus_starting_spin_jump.h — Human-readable C++ replacement for SamusStartingSpinJumpGfx (0x08008ED8).
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>
#include "samus_animation.h"
#include "samus_turning.h"

namespace mzm::native {

constexpr uint8_t SPOSE_STARTING_SPIN_JUMP = 11u; // 0x0B
constexpr uint8_t SPOSE_SPINNING = 12u;           // 0x0C

/**
 * @brief Handles Samus graphical update when starting a spin jump.
 *
 * Replaces Thumb function at ROM 0x08008ED8 (36 bytes).
 * Exactly mirrors upstream C semantics in src/samus.c:4675:
 *   SamusPose SamusStartingSpinJumpGfx(struct SamusData* pData)
 *
 * Calls SamusUpdateAnimation(pData, FALSE); if sequence ended (SAMUS_ANIM_STATE_ENDED = 2):
 *   - pData->pose = SPOSE_SPINNING (12 / 0x0C)
 *   - pData->currentAnimationFrame = 0
 * Always returns SPOSE_NONE (0xFF).
 *
 * Optionally records anim_state to out_anim_state for accurate CPSR C/V thunk restoration.
 */
uint8_t samus_starting_spin_jump_gfx(uint32_t p_data_addr, uint8_t* out_anim_state = nullptr) noexcept;

} // namespace mzm::native
