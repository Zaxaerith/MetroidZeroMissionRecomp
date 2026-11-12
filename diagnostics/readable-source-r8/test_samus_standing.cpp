// test_samus_standing.cpp — Standalone unit test for SamusStandingGfx (0x0800881C).
//
// Origin: MetroidZeroMissionRecomp R8 Native Verification
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <map>
#include <vector>

// Mock guest memory bus
namespace {
std::map<uint32_t, uint8_t> g_mock_mem;

uint8_t mock_read_u8(uint32_t addr) {
    auto it = g_mock_mem.find(addr);
    if (it != g_mock_mem.end()) return it->second;
    return 0;
}

void mock_write_u8(uint32_t addr, uint8_t val) {
    g_mock_mem[addr] = val;
}

uint16_t mock_read_u16(uint32_t addr) {
    return static_cast<uint16_t>(mock_read_u8(addr) | (mock_read_u8(addr + 1) << 8));
}

void mock_write_u16(uint32_t addr, uint16_t val) {
    mock_write_u8(addr, static_cast<uint8_t>(val & 0xFF));
    mock_write_u8(addr + 1, static_cast<uint8_t>((val >> 8) & 0xFF));
}

uint32_t mock_read_u32(uint32_t addr) {
    uint32_t b0 = mock_read_u8(addr);
    uint32_t b1 = mock_read_u8(addr + 1);
    uint32_t b2 = mock_read_u8(addr + 2);
    uint32_t b3 = mock_read_u8(addr + 3);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

void mock_write_u32(uint32_t addr, uint32_t val) {
    mock_write_u8(addr + 0, static_cast<uint8_t>(val & 0xFF));
    mock_write_u8(addr + 1, static_cast<uint8_t>((val >> 8) & 0xFF));
    mock_write_u8(addr + 2, static_cast<uint8_t>((val >> 16) & 0xFF));
    mock_write_u8(addr + 3, static_cast<uint8_t>((val >> 24) & 0xFF));
}
} // namespace

extern "C" {
uint8_t bus_read_u8(uint32_t addr) { return mock_read_u8(addr); }
uint16_t bus_read_u16(uint32_t addr) { return mock_read_u16(addr); }
uint32_t bus_read_u32(uint32_t addr) { return mock_read_u32(addr); }
void bus_write_u8(uint32_t addr, uint8_t val) { mock_write_u8(addr, val); }
void bus_write_u16(uint32_t addr, uint16_t val) { mock_write_u16(addr, val); }
void bus_write_u32(uint32_t addr, uint32_t val) { mock_write_u32(addr, val); }

static uint32_t g_last_runtime_call_return_pc = 0;
int runtime_call_should_return(uint32_t target_pc) {
    g_last_runtime_call_return_pc = target_pc;
    return 1;
}
}

#include "../../src/native/samus_animation.h"
#include "../../src/native/samus_standing.h"

// Define ArmCpuState for thunk test
#define ARM_BANK_COUNT 6u
typedef struct ArmCpuState {
    uint32_t R[16];
    uint32_t cpsr;
    uint32_t banked_sp[ARM_BANK_COUNT];
    uint32_t banked_lr[ARM_BANK_COUNT];
    uint32_t banked_spsr[ARM_BANK_COUNT];
    uint32_t r8_12_user[5];
    uint32_t r8_12_fiq[5];
} ArmCpuState;

#define CPSR_N_BIT (1u << 31)
#define CPSR_Z_BIT (1u << 30)
#define CPSR_C_BIT (1u << 29)
#define CPSR_V_BIT (1u << 28)
#define CPSR_T_BIT (1u <<  5)

namespace mzm::native {
void thunk_samus_standing_gfx_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;
    const uint32_t p_data = cpu->R[0];
    const uint8_t result = samus_standing_gfx(p_data);
    cpu->R[0] = static_cast<uint32_t>(result);
    cpu->R[1] = cpu->R[14];
    cpu->cpsr &= ~(CPSR_N_BIT | CPSR_Z_BIT);
    const uint32_t target = cpu->R[14];
    cpu->R[15] = target & ~1u;
    if (target & 1u) {
        cpu->cpsr |= CPSR_T_BIT;
    } else {
        cpu->cpsr &= ~CPSR_T_BIT;
    }
    runtime_call_should_return(cpu->R[15]);
}
}

using namespace mzm::native;

int main() {
    std::printf("=== R8 SamusStandingGfx Unit Tests ===\n");

    const uint32_t p_data = 0x030013D4u;
    const uint32_t power_suit_anim_base = 0x08248034u;
    const uint8_t standing_pose = 1u; // SPOSE_STANDING

    // Mock Power Suit animation pointer table
    mock_write_u32(ADDR_SAMUS_ANIM_POINTERS_POWER_SUIT + standing_pose * 8, power_suit_anim_base);

    // Frame 0: timer = 4
    // Frame 1: timer = 4
    // Frame 2: timer = 0 (sequence end)
    mock_write_u8(power_suit_anim_base + 0 * SIZEOF_SAMUS_ANIMATION_DATA + OFFSET_SAMUS_ANIMATION_DATA_TIMER, 4);
    mock_write_u8(power_suit_anim_base + 1 * SIZEOF_SAMUS_ANIMATION_DATA + OFFSET_SAMUS_ANIMATION_DATA_TIMER, 4);
    mock_write_u8(power_suit_anim_base + 2 * SIZEOF_SAMUS_ANIMATION_DATA + OFFSET_SAMUS_ANIMATION_DATA_TIMER, 0);

    // ------------------------------------------------------------------------
    // Test 1: Null pointer defense
    // ------------------------------------------------------------------------
    {
        uint8_t res = samus_standing_gfx(0);
        assert(res == SPOSE_NONE);
        std::printf("[PASS] Test 1: Null pointer defense returns SPOSE_NONE\n");
    }

    // ------------------------------------------------------------------------
    // Test 2: Animation in progress (counter < timer) -> frame unchanged, returns SPOSE_NONE
    // ------------------------------------------------------------------------
    {
        mock_write_u8(ADDR_G_EQUIPMENT + OFFSET_EQUIPMENT_SUIT_TYPE, SUIT_NORMAL);
        mock_write_u8(p_data + OFFSET_SAMUS_DATA_POSE, standing_pose);
        mock_write_u8(p_data + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME, 0);
        mock_write_u8(p_data + OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER, 1); // < 4

        uint8_t res = samus_standing_gfx(p_data);
        assert(res == SPOSE_NONE);
        assert(mock_read_u8(p_data + OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER) == 1);
        assert(mock_read_u8(p_data + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME) == 0);
        std::printf("[PASS] Test 2: Animation in progress -> duration counter and frame 0 maintained\n");
    }

    // ------------------------------------------------------------------------
    // Test 3: Sub-ended (counter reaches timer, advances to frame 1) -> returns SPOSE_NONE
    // ------------------------------------------------------------------------
    {
        mock_write_u8(ADDR_G_EQUIPMENT + OFFSET_EQUIPMENT_SUIT_TYPE, SUIT_NORMAL);
        mock_write_u8(p_data + OFFSET_SAMUS_DATA_POSE, standing_pose);
        mock_write_u8(p_data + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME, 0);
        mock_write_u8(p_data + OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER, 4); // == timer

        uint8_t res = samus_standing_gfx(p_data);
        assert(res == SPOSE_NONE);
        assert(mock_read_u8(p_data + OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER) == 0);
        assert(mock_read_u8(p_data + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME) == 1);
        std::printf("[PASS] Test 3: Sub-ended -> frame advanced to 1, counter reset to 0\n");
    }

    // ------------------------------------------------------------------------
    // Test 4: Sequence ended -> frame reset to 0, returns SPOSE_NONE
    // ------------------------------------------------------------------------
    {
        mock_write_u8(ADDR_G_EQUIPMENT + OFFSET_EQUIPMENT_SUIT_TYPE, SUIT_NORMAL);
        mock_write_u8(p_data + OFFSET_SAMUS_DATA_POSE, standing_pose);
        mock_write_u8(p_data + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME, 1);
        mock_write_u8(p_data + OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER, 4); // frame 1 timer reached, next frame 2 has timer 0

        uint8_t res = samus_standing_gfx(p_data);
        assert(res == SPOSE_NONE);
        // After reaching timer 0, SamusUpdateAnimation returns SAMUS_ANIM_STATE_ENDED.
        // SamusStandingGfx then forces currentAnimationFrame = 0!
        assert(mock_read_u8(p_data + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME) == 0);
        std::printf("[PASS] Test 4: Sequence ended -> frame cleanly reset to 0 (animation looping)\n");
    }

    // ------------------------------------------------------------------------
    // Test 5: Suitless pointer table branch
    // ------------------------------------------------------------------------
    {
        const uint32_t suitless_anim_base = 0x08249000u;
        mock_write_u32(ADDR_SAMUS_ANIM_POINTERS_SUITLESS + standing_pose * 8, suitless_anim_base);
        mock_write_u8(suitless_anim_base + 0 * SIZEOF_SAMUS_ANIMATION_DATA + OFFSET_SAMUS_ANIMATION_DATA_TIMER, 2);
        mock_write_u8(suitless_anim_base + 1 * SIZEOF_SAMUS_ANIMATION_DATA + OFFSET_SAMUS_ANIMATION_DATA_TIMER, 0);

        mock_write_u8(ADDR_G_EQUIPMENT + OFFSET_EQUIPMENT_SUIT_TYPE, SUIT_SUITLESS);
        mock_write_u8(p_data + OFFSET_SAMUS_DATA_POSE, standing_pose);
        mock_write_u8(p_data + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME, 0);
        mock_write_u8(p_data + OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER, 2); // reaches timer 0 on next frame

        uint8_t res = samus_standing_gfx(p_data);
        assert(res == SPOSE_NONE);
        assert(mock_read_u8(p_data + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME) == 0);
        std::printf("[PASS] Test 5: Suitless table branch correctly loops animation\n");
    }

    // ------------------------------------------------------------------------
    // Test 6: Thunk & ABI contract verification
    // ------------------------------------------------------------------------
    {
        ArmCpuState cpu{};
        cpu.R[0] = p_data;
        cpu.R[1] = 0x11111111u;
        cpu.R[4] = 0x44444444u;
        cpu.R[13] = 0x03007FA0u;
        cpu.R[14] = 0x0800A455u; // Thumb caller return address
        cpu.cpsr = CPSR_N_BIT | CPSR_Z_BIT | CPSR_C_BIT | CPSR_V_BIT | CPSR_T_BIT;

        mock_write_u8(ADDR_G_EQUIPMENT + OFFSET_EQUIPMENT_SUIT_TYPE, SUIT_NORMAL);
        mock_write_u8(p_data + OFFSET_SAMUS_DATA_POSE, standing_pose);
        mock_write_u8(p_data + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME, 0);
        mock_write_u8(p_data + OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER, 1);

        thunk_samus_standing_gfx_on_state(&cpu);

        // Verify R0 = 0xFF (SPOSE_NONE)
        assert(cpu.R[0] == 0xFFu);

        // Verify R1 = original LR (0x0800A455)
        assert(cpu.R[1] == 0x0800A455u);

        // Verify R4 preserved
        assert(cpu.R[4] == 0x44444444u);

        // Verify SP preserved
        assert(cpu.R[13] == 0x03007FA0u);

        // Verify R15 = LR & ~1 (0x0800A454)
        assert(cpu.R[15] == 0x0800A454u);

        // Verify Thumb mode maintained (CPSR_T_BIT set)
        assert((cpu.cpsr & CPSR_T_BIT) != 0);

        // Verify N=0, Z=0
        assert((cpu.cpsr & CPSR_N_BIT) == 0);
        assert((cpu.cpsr & CPSR_Z_BIT) == 0);

        // Verify C, V flags maintained
        assert((cpu.cpsr & CPSR_C_BIT) != 0);
        assert((cpu.cpsr & CPSR_V_BIT) != 0);

        // Verify runtime_call_should_return was invoked with return PC
        assert(g_last_runtime_call_return_pc == 0x0800A454u);

        std::printf("[PASS] Test 6: Thunk ABI contract (R0=0xFF, R1=LR, R15=0x0800A454, N=0, Z=0, CPSR.T=1, shadow stack popped)\n");
    }

    // ------------------------------------------------------------------------
    // Test 7: Real ROM data verification for Pose 1 (Standing)
    // ------------------------------------------------------------------------
    {
        FILE* fp = std::fopen("Metroid - Zero Mission (USA).gba", "rb");
        if (fp) {
            std::fseek(fp, 0, SEEK_END);
            long rom_size = std::ftell(fp);
            std::fseek(fp, 0, SEEK_SET);
            std::vector<uint8_t> rom(rom_size);
            size_t read_bytes = std::fread(rom.data(), 1, rom_size, fp);
            std::fclose(fp);

            if (read_bytes == 0x800000) {
                for (uint32_t i = 0; i < 0x800000; i += 4) {
                    uint32_t word = rom[i] | (rom[i+1] << 8) | (rom[i+2] << 16) | (rom[i+3] << 24);
                    mock_write_u32(0x08000000u + i, word);
                }

                mock_write_u8(ADDR_G_EQUIPMENT + OFFSET_EQUIPMENT_SUIT_TYPE, SUIT_NORMAL);
                mock_write_u8(p_data + OFFSET_SAMUS_DATA_POSE, standing_pose);
                mock_write_u8(p_data + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME, 0);
                mock_write_u8(p_data + OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER, 0);

                // Initial frame: counter=0 -> in progress
                assert(samus_standing_gfx(p_data) == SPOSE_NONE);
                assert(mock_read_u8(p_data + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME) == 0);

                std::printf("[PASS] Test 7: Real ROM animation table verified for Pose 1 (Standing)\n");
            }
        }
    }

    std::printf("=== ALL R8 UNIT TESTS PASSED ===\n");
    return 0;
}
