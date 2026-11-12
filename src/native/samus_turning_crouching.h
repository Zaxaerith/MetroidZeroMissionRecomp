// samus_turning_crouching.h — Human-readable C++ replacement for SamusTurningAroundAndCrouchingGfx (0x08008C24).
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>
#include "samus_animation.h"
#include "samus_turning.h"
#include "samus_shooting_crouching.h"

namespace mzm::native {

constexpr uint8_t SPOSE_CROUCHING_SUITLESS = 60u; // 0x3C

/**
 * @brief Handles Samus graphical update when turning around while crouching.
 *
 * Replaces Thumb function at ROM 0x08008C24 (44 bytes).
 * Exactly mirrors upstream C semantics in src/samus.c:4420:
 *   SamusPose SamusTurningAroundAndCrouchingGfx(struct SamusData* pData)
 *
 * Calls SamusUpdateAnimation(pData, FALSE); if sequence ended (SAMUS_ANIM_STATE_ENDED = 2):
 *   - if gEquipment.suitType == SUIT_SUITLESS (2), returns SPOSE_CROUCHING_SUITLESS (60 / 0x3C)
 *   - else returns SPOSE_CROUCHING (4)
 * Otherwise returns SPOSE_NONE (0xFF).
 *
 * Optionally records anim_state and suit_type to out parameters for precise CPSR C/V restoration in the Thunk.
 */
uint8_t samus_turning_around_and_crouching_gfx(
    uint32_t p_data_addr,
    uint8_t* out_anim_state = nullptr,
    uint8_t* out_suit_type = nullptr
) noexcept;

} // namespace mzm::native
