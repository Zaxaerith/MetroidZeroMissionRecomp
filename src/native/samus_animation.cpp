// samus_animation.cpp — Human-readable C++ implementation for SamusUpdateAnimation.
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include "samus_animation.h"

#if __has_include("runtime_arm.h")
#include "runtime_arm.h"
#else
extern "C" {
uint8_t bus_read_u8(uint32_t addr);
uint32_t bus_read_u32(uint32_t addr);
void bus_write_u8(uint32_t addr, uint8_t val);
}
#endif

namespace mzm::native {

uint8_t samus_update_animation(uint32_t p_data_addr, uint8_t slowed) noexcept {
    if (p_data_addr == 0u) {
        return SAMUS_ANIM_STATE_NONE;
    }

    // 1. Determine animation table based on suit type in gEquipment
    const uint8_t suit_type = bus_read_u8(ADDR_G_EQUIPMENT + OFFSET_EQUIPMENT_SUIT_TYPE);
    const uint32_t anim_table = (suit_type == SUIT_SUITLESS)
        ? ADDR_SAMUS_ANIM_POINTERS_SUITLESS
        : ADDR_SAMUS_ANIM_POINTERS_POWER_SUIT;

    // 2. Read pose from SamusData
    const uint8_t pose = bus_read_u8(p_data_addr + OFFSET_SAMUS_DATA_POSE);

    // 3. Look up animation array pointer: anim_table[pose][0]
    // Each entry has 2 pointers (8 bytes per pose). Index [0] is at offset 0.
    const uint32_t anim_ptr_addr = anim_table + static_cast<uint32_t>(pose) * 8u;
    const uint32_t anim_base = bus_read_u32(anim_ptr_addr);
    if (anim_base == 0u) {
        return SAMUS_ANIM_STATE_NONE;
    }

    // 4. Read current animation frame index
    const uint8_t current_frame = bus_read_u8(p_data_addr + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME);

    // 5. Locate current frame data and fetch timer
    const uint32_t current_frame_addr = anim_base + static_cast<uint32_t>(current_frame) * SIZEOF_SAMUS_ANIMATION_DATA;
    uint8_t timer = bus_read_u8(current_frame_addr + OFFSET_SAMUS_ANIMATION_DATA_TIMER);

    // 6. Double timer if slowed (e.g. by liquid)
    if (slowed != 0) {
        timer = static_cast<uint8_t>(timer * 2u);
    }

    // 7. Check if current frame duration counter has elapsed
    const uint8_t duration_counter = bus_read_u8(p_data_addr + OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER);
    if (duration_counter >= timer) {
        // Reset duration counter
        bus_write_u8(p_data_addr + OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER, 0u);

        // Advance to next frame
        const uint8_t next_frame = static_cast<uint8_t>(current_frame + 1u);
        bus_write_u8(p_data_addr + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME, next_frame);

        // Inspect next frame's timer
        const uint32_t next_frame_addr = current_frame_addr + SIZEOF_SAMUS_ANIMATION_DATA;
        const uint8_t next_timer = bus_read_u8(next_frame_addr + OFFSET_SAMUS_ANIMATION_DATA_TIMER);

        if (next_timer == 0u) {
            return SAMUS_ANIM_STATE_ENDED;
        } else {
            return SAMUS_ANIM_STATE_SUB_ENDED;
        }
    }

    return SAMUS_ANIM_STATE_NONE;
}

} // namespace mzm::native
