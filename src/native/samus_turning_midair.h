// samus_turning_midair.h — Human-readable C++ replacement for SamusTurningAroundMidAirGfx (0x08008EB8).
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>
#include "samus_animation.h"
#include "samus_turning.h"

namespace mzm::native {

// Offset of forcedMovement field in struct SamusData
constexpr uint32_t OFFSET_SAMUS_DATA_FORCED_MOVEMENT = 0x04u;

// Forced movement constants (from include/constants/samus.h)
constexpr uint8_t FORCED_MOVEMENT_MID_AIR_CARRY = 2u;

// Pose constants (from include/constants/samus.h)
constexpr uint8_t SPOSE_MID_AIR_REQUEST = 0xFEu; // 254

/**
 * @brief Handles Samus graphical update when turning around mid-air.
 *
 * Replaces Thumb function at ROM 0x08008EB8 (32 bytes).
 * Exactly mirrors upstream C semantics in src/samus.c:4660:
 *   SamusPose SamusTurningAroundMidAirGfx(struct SamusData* pData)
 *
 * Calls SamusUpdateAnimation(pData, FALSE); if sequence ended (SAMUS_ANIM_STATE_ENDED = 2):
 *   - pData->forcedMovement = FORCED_MOVEMENT_MID_AIR_CARRY (2)
 *   - returns SPOSE_MID_AIR_REQUEST (0xFE / 254)
 * Otherwise returns SPOSE_NONE (0xFF / 255).
 *
 * Optionally records anim_state to out_anim_state for accurate CPSR C/V thunk restoration.
 */
uint8_t samus_turning_around_mid_air_gfx(uint32_t p_data_addr, uint8_t* out_anim_state = nullptr) noexcept;

} // namespace mzm::native
