// samus_turning.cpp — Human-readable C++ implementation for SamusTurningAroundGfx.
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include "samus_turning.h"

#if __has_include("runtime_arm.h")
#include "runtime_arm.h"
#else
extern "C" {
uint8_t bus_read_u8(uint32_t addr);
uint16_t bus_read_u16(uint32_t addr);
uint32_t bus_read_u32(uint32_t addr);
void bus_write_u8(uint32_t addr, uint8_t val);
void bus_write_u16(uint32_t addr, uint16_t val);
void bus_write_u32(uint32_t addr, uint32_t val);
}
#endif

namespace mzm::native {

uint8_t samus_turning_around_gfx(uint32_t p_data_addr) noexcept {
    if (p_data_addr == 0u) {
        return SPOSE_NONE;
    }

    // 1. Advance turning animation (slowed = FALSE = 0)
    const uint8_t anim_state = samus_update_animation(p_data_addr, 0u);

    // 2. If turning animation ended, evaluate motion state transition
    if (anim_state == SAMUS_ANIM_STATE_ENDED) {
        // Read Samus facing direction
        const uint16_t direction = bus_read_u16(p_data_addr + OFFSET_SAMUS_DATA_DIRECTION);
        const uint16_t opposite_dir = direction ^ KEY_DIRECTIONS;

        // Check if player is holding input in the new facing direction (continue running)
        const uint16_t buttons = bus_read_u16(ADDR_G_BUTTON_INPUT);
        if ((buttons & opposite_dir) != 0u) {
            return SPOSE_RUNNING;
        }

        // Check suit type to select standing pose (Power Suit vs Suitless)
        const uint8_t suit_type = bus_read_u8(ADDR_G_EQUIPMENT + OFFSET_EQUIPMENT_SUIT_TYPE);
        if (suit_type == SUIT_SUITLESS) {
            return SPOSE_UNCROUCHING_SUITLESS;
        } else {
            return SPOSE_STANDING;
        }
    }

    // Animation still in progress: remain in turning pose
    return SPOSE_NONE;
}

} // namespace mzm::native
