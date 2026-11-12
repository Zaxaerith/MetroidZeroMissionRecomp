// samus_turning_crouching.cpp — Human-readable C++ implementation for SamusTurningAroundAndCrouchingGfx.
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include "samus_turning_crouching.h"

#if __has_include("runtime_arm.h")
#include "runtime_arm.h"
#else
extern "C" {
uint8_t bus_read_u8(uint32_t addr);
}
#endif

namespace mzm::native {

uint8_t samus_turning_around_and_crouching_gfx(
    uint32_t p_data_addr,
    uint8_t* out_anim_state,
    uint8_t* out_suit_type
) noexcept {
    if (p_data_addr == 0u) {
        if (out_anim_state) *out_anim_state = SAMUS_ANIM_STATE_NONE;
        if (out_suit_type) *out_suit_type = SUIT_NORMAL;
        return SPOSE_NONE;
    }

    // 1. Advance turn-crouching animation exactly once (slowed = FALSE = 0)
    const uint8_t anim_state = samus_update_animation(p_data_addr, 0u);
    if (out_anim_state) {
        *out_anim_state = anim_state;
    }

    // 2. If animation sequence ended, transition back to crouching pose based on suit type
    if (anim_state == SAMUS_ANIM_STATE_ENDED) {
        const uint8_t suit_type = bus_read_u8(ADDR_G_EQUIPMENT + OFFSET_EQUIPMENT_SUIT_TYPE);
        if (out_suit_type) {
            *out_suit_type = suit_type;
        }

        if (suit_type == SUIT_SUITLESS) {
            return SPOSE_CROUCHING_SUITLESS;
        } else {
            return SPOSE_CROUCHING;
        }
    }

    if (out_suit_type) {
        *out_suit_type = SUIT_NORMAL;
    }
    return SPOSE_NONE;
}

} // namespace mzm::native
