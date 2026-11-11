// soft_reset_check.cpp — Human-readable C++ implementation for SoftResetCheck.
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include "soft_reset_check.h"
#include "runtime_arm.h"

namespace mzm::native {

void soft_reset_check() noexcept {
    // 1. Check if soft reset is already active
    const uint16_t game_mode = bus_read_u16(ADDR_G_MAIN_GAME_MODE);
    if (game_mode == GM_START_SOFT_RESET) {
        return;
    }

    // 2. Check if soft reset is disabled by game state
    const uint8_t disable = bus_read_u8(ADDR_G_DISABLE_SOFT_RESET);
    if (disable != 0) {
        return;
    }

    // 3. Check if all soft reset keys are held (A + B + SELECT + START)
    const uint16_t buttons = bus_read_u16(ADDR_G_BUTTON_INPUT);
    if ((buttons & SOFT_RESET_KEYS) == SOFT_RESET_KEYS) {
        bus_write_u16(ADDR_G_MAIN_GAME_MODE, GM_START_SOFT_RESET);
    }
}

} // namespace mzm::native
