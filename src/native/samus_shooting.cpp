// samus_shooting.cpp — Human-readable C++ implementation for SamusShootingGfx.
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include "samus_shooting.h"

namespace mzm::native {

uint8_t samus_shooting_gfx(uint32_t p_data_addr, uint8_t* out_anim_state) noexcept {
    if (p_data_addr == 0u) {
        if (out_anim_state) {
            *out_anim_state = SAMUS_ANIM_STATE_NONE;
        }
        return SPOSE_NONE;
    }

    // 1. Advance shooting/landing animation (slowed = FALSE = 0)
    const uint8_t anim_state = samus_update_animation(p_data_addr, 0u);
    if (out_anim_state) {
        *out_anim_state = anim_state;
    }

    // 2. If animation sequence ended, transition back to SPOSE_STANDING (1)
    if (anim_state == SAMUS_ANIM_STATE_ENDED) {
        return SPOSE_STANDING;
    }

    return SPOSE_NONE;
}

} // namespace mzm::native
