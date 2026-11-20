// samus_morphing.cpp — Human-readable C++ implementation for SamusMorphingGfx.
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include "samus_morphing.h"

namespace mzm::native {

static SamusUpdateAnimationFn s_update_anim_override = nullptr;

void set_samus_update_animation_override(SamusUpdateAnimationFn fn) noexcept {
    s_update_anim_override = fn;
}

uint8_t samus_morphing_gfx(uint32_t p_data_addr, uint8_t* out_anim_state) noexcept {
    if (p_data_addr == 0u) {
        if (out_anim_state) {
            *out_anim_state = SAMUS_ANIM_STATE_NONE;
        }
        return SPOSE_NONE;
    }

    // 1. Advance morphing animation exactly once (slowed = FALSE = 0)
    const uint8_t anim_state = s_update_anim_override
        ? s_update_anim_override(p_data_addr, 0u)
        : samus_update_animation(p_data_addr, 0u);

    if (out_anim_state) {
        *out_anim_state = anim_state;
    }

    // 2. If animation sequence ended (anim_state == 2), transition to SPOSE_MORPH_BALL (17 / 0x11)
    if (anim_state == SAMUS_ANIM_STATE_ENDED) {
        return SPOSE_MORPH_BALL;
    }

    // 3. Otherwise return SPOSE_NONE (255 / 0xFF)
    return SPOSE_NONE;
}

} // namespace mzm::native
