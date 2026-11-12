// samus_shooting_crouching.cpp — Human-readable C++ implementation for SamusShootingAndCrouchingGfx.
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include "samus_shooting_crouching.h"

namespace mzm::native {

uint8_t samus_shooting_and_crouching_gfx(uint32_t p_data_addr, uint8_t* out_anim_state) noexcept {
    if (p_data_addr == 0u) {
        if (out_anim_state) {
            *out_anim_state = SAMUS_ANIM_STATE_NONE;
        }
        return SPOSE_NONE;
    }

    // 1. Advance crouch-shooting animation (slowed = FALSE = 0)
    const uint8_t anim_state = samus_update_animation(p_data_addr, 0u);
    if (out_anim_state) {
        *out_anim_state = anim_state;
    }

    // 2. If animation sequence ended, transition back to SPOSE_CROUCHING (4)
    if (anim_state == SAMUS_ANIM_STATE_ENDED) {
        return SPOSE_CROUCHING;
    }

    return SPOSE_NONE;
}

} // namespace mzm::native
