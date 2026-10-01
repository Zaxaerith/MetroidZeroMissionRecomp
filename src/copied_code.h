#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <array>

namespace mzm {
// Observed DMA3 copy, not imported decomp symbols. See RAM_COPIES.md.
inline constexpr std::uint32_t kCollisionRam = 0x030016C4;
inline constexpr std::uint32_t kCollisionRomOffset = 0x00057F7C;
inline constexpr std::uint32_t kCollisionImageSize = 640;
inline constexpr std::uint32_t kCollisionCodeSize = 0x104;
// Observed DMA3 BG3 haze copy, independent of the adjacent collision image.
inline constexpr std::uint32_t kHazeRam = 0x03001944;
inline constexpr std::uint32_t kHazeRomOffset = 0x0005D768;
inline constexpr std::uint32_t kHazeImageSize = 512;
inline constexpr std::uint32_t kHazeCodeSize = 0xB8;
// Complete 24-halfword stack install observed before the first strict miss.
inline constexpr std::uint32_t kStackRam = 0x03007D38;
inline constexpr std::uint32_t kStackRomOffset = 0x0000529C;
inline constexpr std::uint32_t kStackImageSize = 48;
inline constexpr std::uint32_t kStackReadRam = 0x03007D8C;
inline constexpr std::uint32_t kStackReadRomOffset = 0x000051D4;
inline constexpr std::uint32_t kStackReadImageSize = 36;
struct StackImage {
    std::uint32_t runtime, source_offset, size;
};
// Complete observed placements for native guarded dispatch. Every entry and
// interior instruction is authorized only by its whole currently installed image.
inline constexpr std::array<StackImage, 7> kStackImages{{
    {kStackRam, kStackRomOffset, kStackImageSize},
    {kStackReadRam, kStackReadRomOffset, kStackReadImageSize},
    {0x03007D90, kStackReadRomOffset, kStackReadImageSize},
    {0x03007D50, kStackRomOffset, kStackImageSize},
    {0x03007D18, kStackRomOffset, kStackImageSize},
    {0x03007D14, kStackRomOffset, kStackImageSize},
    {0x03007D08, kStackRomOffset, kStackImageSize},
}};
inline bool stack_image_matches(const StackImage& image,
    std::span<const std::uint8_t> rom, std::span<const std::uint8_t> ram) {
    const auto offset = image.runtime - 0x03000000;
    return rom.size() >= image.source_offset + image.size &&
           ram.size() >= offset + image.size &&
           std::memcmp(rom.data() + image.source_offset, ram.data() + offset, image.size) == 0;
}
inline int select_stack_image(std::uint32_t pc, bool thumb,
    std::span<const std::uint8_t> rom, std::span<const std::uint8_t> ram) {
    if (!thumb || (pc & 1u)) return -1;
    int selected = -1;
    for (int i = 0; i < static_cast<int>(kStackImages.size()); ++i) {
        const auto& image = kStackImages[i];
        // Both verified helpers terminate in BX followed by two padding bytes.
        if (pc - image.runtime >= image.size - 2 || !stack_image_matches(image, rom, ram)) continue;
        if (selected >= 0) return -1; // ambiguous identity is never native
        selected = i;
    }
    return selected;
}

inline bool guarded_code_pc(std::uint32_t pc) {
    return pc - kCollisionRam < kCollisionCodeSize ||
           pc - kHazeRam < kHazeCodeSize ||
           pc - kStackRam < kStackImageSize ||
           pc - kStackReadRam < kStackReadImageSize;
}

inline bool stack_read_code_requires_interpreter(
    std::uint32_t pc, bool thumb, std::span<const std::uint8_t> rom,
    std::span<const std::uint8_t> iwram) {
    if (pc - kStackReadRam >= kStackReadImageSize) return false;
    constexpr auto offset = kStackReadRam - 0x03000000;
    if (!thumb || (pc & 1u) || rom.size() < kStackReadRomOffset + kStackReadImageSize ||
        iwram.size() < offset + kStackReadImageSize) return true;
    return std::memcmp(rom.data() + kStackReadRomOffset,
                       iwram.data() + offset, kStackReadImageSize) != 0;
}

inline bool stack_code_requires_interpreter(
    std::uint32_t pc, bool thumb, std::span<const std::uint8_t> rom,
    std::span<const std::uint8_t> iwram) {
    if (pc - kStackRam >= kStackImageSize) return false;
    constexpr auto offset = kStackRam - 0x03000000;
    if (!thumb || (pc & 1u) || rom.size() < kStackRomOffset + kStackImageSize ||
        iwram.size() < offset + kStackImageSize) return true;
    return std::memcmp(rom.data() + kStackRomOffset,
                       iwram.data() + offset, kStackImageSize) != 0;
}

inline bool collision_code_requires_interpreter(
    std::uint32_t pc, bool thumb, std::span<const std::uint8_t> rom,
    std::span<const std::uint8_t> iwram) {
    if (pc - kCollisionRam >= kCollisionCodeSize) return false;
    constexpr auto ram_offset = kCollisionRam - 0x03000000;
    // Reject incomplete images and wrong mode; do not cache across writes,
    // DMA, pause transitions, state loads or instruction boundaries.
    if (!thumb || (pc & 1u) ||
        rom.size() < kCollisionRomOffset + kCollisionImageSize ||
        iwram.size() < ram_offset + kCollisionImageSize) return true;
    return std::memcmp(rom.data() + kCollisionRomOffset,
                       iwram.data() + ram_offset, kCollisionImageSize) != 0;
}

inline bool haze_code_requires_interpreter(
    std::uint32_t pc, bool thumb, std::span<const std::uint8_t> rom,
    std::span<const std::uint8_t> iwram) {
    if (pc - kHazeRam >= kHazeCodeSize) return false;
    constexpr auto offset = kHazeRam - 0x03000000;
    if (!thumb || (pc & 1u) || rom.size() < kHazeRomOffset + kHazeImageSize ||
        iwram.size() < offset + kHazeImageSize) return true;
    return std::memcmp(rom.data() + kHazeRomOffset, iwram.data() + offset, kHazeImageSize) != 0;
}

int copied_code_interpreter_guard(std::uint32_t pc, int thumb);
int copied_code_ram_dispatch(std::uint32_t pc, int thumb);
// Generated complete-image bodies; local artifacts, never distributed.
void stack_variant_0();
void stack_variant_1();
void stack_variant_2();
void stack_variant_3();
void stack_variant_4();
void stack_variant_5();
void stack_variant_6();
}  // namespace mzm
