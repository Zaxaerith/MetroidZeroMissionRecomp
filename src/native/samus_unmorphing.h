// samus_unmorphing.h — Human-readable C++ replacement for SamusUnmorphingGfx (0x08009508).
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>
#include "samus_animation.h"
#include "samus_turning.h"
#include "samus_shooting_crouching.h"

namespace mzm::native {

// Struct SamusData member offsets
constexpr uint32_t OFFSET_SAMUS_DATA_UNMORPH_PALETTE_TIMER = 0x09u;

// SamusPose constants (from include/constants/samus.h)
// SPOSE_CROUCHING (4) is defined in samus_shooting_crouching.h
// SPOSE_NONE (0xFF) is defined in samus_turning.h
constexpr uint8_t SPOSE_UNMORPHING = 19u;

// Unmorph palette timer initial value
constexpr uint8_t UNMORPH_PALETTE_TIMER_INIT = 15u;

/**
 * @brief Handles Samus graphical update when unmorphing from morph ball.
 *
 * Replaces Thumb function at ROM 0x08009508 (36 bytes).
 * Exactly mirrors upstream C semantics in src/samus.c:5193:
 *   SamusPose SamusUnmorphingGfx(struct SamusData* pData)
 *
 * Calls SamusUpdateAnimation(pData, FALSE); if sequence ended (SAMUS_ANIM_STATE_ENDED = 2):
 *   - pData->unmorphPaletteTimer = UNMORPH_PALETTE_TIMER_INIT (15)
 *   - returns SPOSE_CROUCHING (4)
 * Otherwise returns SPOSE_NONE (0xFF / 255).
 *
 * Optionally records anim_state to out_anim_state for accurate CPSR C/V thunk restoration:
 *   N = 0, Z = 0, V = 0, C = (anim_state >= 2).
 */
uint8_t samus_unmorphing_gfx(uint32_t p_data_addr, uint8_t* out_anim_state = nullptr) noexcept;

} // namespace mzm::native
