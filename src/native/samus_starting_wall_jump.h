// samus_starting_wall_jump.h — Human-readable C++ replacement for SamusStartingWallJumpGfx (0x080090E0).
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>
#include "samus_animation.h"
#include "samus_turning.h"
#include "samus_turning_midair.h"

namespace mzm::native {

// OFFSET_SAMUS_DATA_FORCED_MOVEMENT (0x04) is defined in samus_turning_midair.h
// SPOSE_MID_AIR_REQUEST (0xFE) is defined in samus_turning_midair.h
// SPOSE_NONE = 0xFFu (255) is defined in samus_turning.h

// Forced movement constants (from include/constants/samus.h)
constexpr uint8_t FORCED_MOVEMENT_MID_AIR_JUMP = 1u;

// Pose constants (from include/constants/samus.h)
constexpr uint8_t SPOSE_STARTING_WALL_JUMP = 13u;

/**
 * @brief Handles Samus graphical update when starting a wall jump.
 *
 * Replaces Thumb function at ROM 0x080090E0 (36 bytes).
 * Exactly mirrors upstream C semantics in src/samus.c:4840:
 *   SamusPose SamusStartingWallJumpGfx(struct SamusData* pData)
 *
 * Calls SamusUpdateAnimation(pData, FALSE); if sequence ended (SAMUS_ANIM_STATE_ENDED = 2):
 *   - pData->forcedMovement = FORCED_MOVEMENT_MID_AIR_JUMP (1)
 *   - returns SPOSE_MID_AIR_REQUEST (0xFE / 254)
 * Otherwise returns SPOSE_NONE (0xFF / 255).
 *
 * Exact CPSR Flags contract:
 *   cmp r0, #2 => N=0, Z=(anim_state==2), C=(anim_state>=2), V=0.
 *   On path 1 (anim!=2): movs r0, #255 => N=0, Z=0, C/V unchanged (so C=(anim_state>=2), V=0).
 *   On path 2 (anim==2): movs r0, #1 (Z=0, N=0), strb, movs r0, #254 (Z=0, N=0, C/V unchanged so C=1, V=0).
 *   Therefore:
 *   N = 0, Z = 0, V = 0, C = (anim_state >= 2).
 *
 * Optionally records anim_state to out_anim_state for accurate CPSR C/V thunk restoration.
 */
uint8_t samus_starting_wall_jump_gfx(uint32_t p_data_addr, uint8_t* out_anim_state = nullptr) noexcept;

} // namespace mzm::native
