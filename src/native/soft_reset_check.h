// soft_reset_check.h — Human-readable C++ replacement for SoftResetCheck (0x080007D0).
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>

namespace mzm::native {

// Game mode and key constants matching upstream Zero Mission headers
constexpr uint16_t GM_START_SOFT_RESET = 0x000Eu;
constexpr uint16_t SOFT_RESET_KEYS = 0x000Fu; // KEY_A | KEY_B | KEY_SELECT | KEY_START

// Verified guest IWRAM memory addresses in Metroid: Zero Mission USA Rev 0
constexpr uint32_t ADDR_G_MAIN_GAME_MODE = 0x03000C70u;
constexpr uint32_t ADDR_G_DISABLE_SOFT_RESET = 0x03000043u;
constexpr uint32_t ADDR_G_BUTTON_INPUT = 0x0300137Cu;

/**
 * @brief Checks for soft reset conditions and transitions gMainGameMode if triggered.
 *
 * Replaces Thumb function at ROM 0x080007D0 (52 bytes).
 * Exactly mirrors upstream C semantics in src/soft_reset_input.c:
 *   if (gMainGameMode == GM_START_SOFT_RESET) return;
 *   if (gDisableSoftReset) return;
 *   if ((gButtonInput & SOFT_RESET_KEYS) == SOFT_RESET_KEYS)
 *       gMainGameMode = GM_START_SOFT_RESET;
 */
void soft_reset_check() noexcept;

} // namespace mzm::native
