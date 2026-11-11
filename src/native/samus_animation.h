// samus_animation.h — Human-readable C++ replacement for SamusUpdateAnimation (0x0800847C).
//
// Origin: Metroid: Zero Mission USA Rev 0 decomp reference (metroidret/mzm @ 43b7fd5)
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>

namespace mzm::native {

// Animation state return values (from include/constants/samus.h)
constexpr uint8_t SAMUS_ANIM_STATE_NONE = 0u;
constexpr uint8_t SAMUS_ANIM_STATE_SUB_ENDED = 1u;
constexpr uint8_t SAMUS_ANIM_STATE_ENDED = 2u;

// Suit types (from include/constants/samus.h)
constexpr uint8_t SUIT_NORMAL = 0u;
constexpr uint8_t SUIT_FULLY_POWERED = 1u;
constexpr uint8_t SUIT_SUITLESS = 2u;

// Verified memory addresses in Metroid: Zero Mission USA Rev 0
constexpr uint32_t ADDR_G_EQUIPMENT = 0x03001530u;
constexpr uint32_t OFFSET_EQUIPMENT_SUIT_TYPE = 0x12u;

// Animation pointer tables in ROM
constexpr uint32_t ADDR_SAMUS_ANIM_POINTERS_POWER_SUIT = 0x08238B74u;
constexpr uint32_t ADDR_SAMUS_ANIM_POINTERS_SUITLESS = 0x082399B0u;

// Struct SamusData member offsets
constexpr uint32_t OFFSET_SAMUS_DATA_POSE = 0x00u;
constexpr uint32_t OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER = 0x1Cu;
constexpr uint32_t OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME = 0x1Du;

// Struct SamusAnimationData stride and member offset
constexpr uint32_t SIZEOF_SAMUS_ANIMATION_DATA = 16u; // 0x10 bytes per frame
constexpr uint32_t OFFSET_SAMUS_ANIMATION_DATA_TIMER = 12u; // 0x0C: timer field

/**
 * @brief Updates Samus animation state, advancing frames when elapsed.
 *
 * Replaces Thumb function at ROM 0x0800847C (96 bytes).
 * Exactly mirrors upstream C semantics in src/samus.c:3805:
 *   SamusAnimState SamusUpdateAnimation(struct SamusData* pData, u8 slowed)
 */
uint8_t samus_update_animation(uint32_t p_data_addr, uint8_t slowed) noexcept;

} // namespace mzm::native
