// samus_turning.h — Human-readable C++ replacement for SamusTurningAroundGfx (0x080088F8).
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>
#include "samus_animation.h"
#include "soft_reset_check.h"

namespace mzm::native {

// SamusPose return values (from include/constants/samus.h)
constexpr uint8_t SPOSE_RUNNING = 0u;
constexpr uint8_t SPOSE_STANDING = 1u;
constexpr uint8_t SPOSE_UNCROUCHING_SUITLESS = 59u; // 0x3B
constexpr uint8_t SPOSE_NONE = 0xFFu;

// Key input direction constants (from include/constants/game_state.h)
constexpr uint16_t KEY_RIGHT = 0x0010u;
constexpr uint16_t KEY_LEFT  = 0x0020u;
constexpr uint16_t KEY_DIRECTIONS = KEY_RIGHT | KEY_LEFT; // 0x0030u

// Verified memory addresses in Metroid: Zero Mission USA Rev 0
// ADDR_G_BUTTON_INPUT (0x0300137Cu) is imported from soft_reset_check.h
constexpr uint32_t OFFSET_SAMUS_DATA_DIRECTION = 0x0Eu;

/**
 * @brief Handles Samus graphical update when turning around, determining state transitions.
 *
 * Replaces Thumb function at ROM 0x080088F8 (72 bytes).
 * Exactly mirrors upstream C semantics in src/samus.c:4180:
 *   SamusPose SamusTurningAroundGfx(struct SamusData* pData)
 */
uint8_t samus_turning_around_gfx(uint32_t p_data_addr) noexcept;

} // namespace mzm::native
