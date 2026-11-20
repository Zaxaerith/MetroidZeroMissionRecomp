// samus_unmorphing.cpp — Human-readable C++ implementation for SamusUnmorphingGfx.
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include "samus_unmorphing.h"

#if __has_include("runtime_arm.h")
#include "runtime_arm.h"
#else
extern "C" {
void bus_write_u8(uint32_t addr, uint8_t val);
}
#endif

namespace mzm::native {

uint8_t samus_unmorphing_gfx(uint32_t p_data_addr, uint8_t* out_anim_state) noexcept {
    if (p_data_addr == 0u) {
        if (out_anim_state) {
            *out_anim_state = SAMUS_ANIM_STATE_NONE;
        }
        return SPOSE_NONE;
    }

    // 1. Advance unmorphing animation (slowed = FALSE = 0)
    const uint8_t anim_state = samus_update_animation(p_data_addr, 0u);
    if (out_anim_state) {
        *out_anim_state = anim_state;
    }

    // 2. If animation sequence ended, reset palette timer and return SPOSE_CROUCHING
    if (anim_state == SAMUS_ANIM_STATE_ENDED) {
        bus_write_u8(p_data_addr + OFFSET_SAMUS_DATA_UNMORPH_PALETTE_TIMER, UNMORPH_PALETTE_TIMER_INIT);
        return SPOSE_CROUCHING;
    }

    return SPOSE_NONE;
}

} // namespace mzm::native
