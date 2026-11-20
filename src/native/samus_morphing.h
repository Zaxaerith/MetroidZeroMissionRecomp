// samus_morphing.h — Human-readable C++ replacement for SamusMorphingGfx (0x08009230).
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>
#include "samus_animation.h"
#include "samus_turning.h"

namespace mzm::native {

// Pose constants (from include/constants/samus.h)
constexpr uint8_t SPOSE_MORPHING = 16u;   // 0x10
constexpr uint8_t SPOSE_MORPH_BALL = 17u; // 0x11
// Note: SPOSE_NONE (0xFF) is imported from samus_turning.h

/**
 * @brief Handles Samus graphical update when morphing into morph ball.
 *
 * Replaces Thumb function at ROM 0x08009230 (28 bytes).
 * Exactly mirrors upstream C semantics in src/samus.c:4955:
 *   SamusPose SamusMorphingGfx(struct SamusData* pData)
 *
 * Calls SamusUpdateAnimation(pData, FALSE); if sequence ended (SAMUS_ANIM_STATE_ENDED = 2):
 *   - returns SPOSE_MORPH_BALL (17 / 0x11)
 * Otherwise returns SPOSE_NONE (0xFF / 255).
 *
 * Optionally records anim_state to out_anim_state for accurate CPSR C/V thunk restoration.
 */
uint8_t samus_morphing_gfx(uint32_t p_data_addr, uint8_t* out_anim_state = nullptr) noexcept;

// Optional test injection hook to simulate edge case return states from samus_update_animation
using SamusUpdateAnimationFn = uint8_t (*)(uint32_t, uint8_t) noexcept;
void set_samus_update_animation_override(SamusUpdateAnimationFn fn) noexcept;

} // namespace mzm::native
