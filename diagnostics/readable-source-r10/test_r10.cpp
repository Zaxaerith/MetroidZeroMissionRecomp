// test_r10.cpp — Standalone unit test for R10 SamusShootingAndCrouchingGfx (0x08008C50).
//
// Origin: MetroidZeroMissionRecomp R10 Native Verification
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <map>
#include <vector>

#include "../../src/native/samus_animation.h"
#include "../../src/native/samus_shooting_crouching.h"

// Mock guest memory bus
namespace {
std::map<uint32_t, uint8_t> g_mock_mem;
int g_mock_anim_table_reads = 0;

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
    if (addr >= mzm::native::ADDR_SAMUS_ANIM_POINTERS_POWER_SUIT && addr < mzm::native::ADDR_SAMUS_ANIM_POINTERS_POWER_SUIT + 0x1000) {
        g_mock_anim_table_reads++;
    }
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

void thunk_samus_shooting_crouching_gfx_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;
    const uint32_t p_data = cpu->R[0];
    uint8_t anim_state = 0u;
    const uint8_t result = samus_shooting_and_crouching_gfx(p_data, &anim_state);
    cpu->R[0] = static_cast<uint32_t>(result);
    cpu->R[1] = cpu->R[14];
    cpu->cpsr &= ~(CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT | CPSR_C_BIT);
    if (anim_state >= 2u) {
        cpu->cpsr |= CPSR_C_BIT;
    }
    const uint32_t target = cpu->R[14];
    cpu->R[15] = target & ~1u;
    if (target & 1u) {
        cpu->cpsr |= CPSR_T_BIT;
    } else {
        cpu->cpsr &= ~CPSR_T_BIT;
    }
    runtime_call_should_return(cpu->R[15]);
}

static bool s_mock_enabled = false;
static uint64_t s_mock_hits = 0;

int hook_samus_shooting_crouching_gfx_mock(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept {
    if (addr != 0x08008C50u || !thumb || !cpu) {
        return 0;
    }
    if (!s_mock_enabled) {
        return 0;
    }
    thunk_samus_shooting_crouching_gfx_on_state(cpu);
    s_mock_hits++;
    return 1;
}

} // namespace mzm::native

using namespace mzm::native;

static const uint32_t P_DATA = 0x030013D4u;
static const uint32_t ANIM_BASE = 0x08250000u;
static const uint8_t POSE_CROUCH_SHOOT = 6u; // SPOSE_SHOOTING_AND_CROUCHING

static void setup_crouch_shoot_animation(uint8_t frame_index, uint8_t duration_counter) {
    g_mock_mem.clear();

    // Register animation pointer in Power Suit table for pose 6
    mock_write_u32(ADDR_SAMUS_ANIM_POINTERS_POWER_SUIT + POSE_CROUCH_SHOOT * 8, ANIM_BASE);

    // Frame 0: timer = 4
    mock_write_u8(ANIM_BASE + 0 * SIZEOF_SAMUS_ANIMATION_DATA + OFFSET_SAMUS_ANIMATION_DATA_TIMER, 4);
    // Frame 1: timer = 4
    mock_write_u8(ANIM_BASE + 1 * SIZEOF_SAMUS_ANIMATION_DATA + OFFSET_SAMUS_ANIMATION_DATA_TIMER, 4);
    // Frame 2: timer = 0 (sequence terminator)
    mock_write_u8(ANIM_BASE + 2 * SIZEOF_SAMUS_ANIMATION_DATA + OFFSET_SAMUS_ANIMATION_DATA_TIMER, 0);

    // Setup SamusData
    mock_write_u8(ADDR_G_EQUIPMENT + OFFSET_EQUIPMENT_SUIT_TYPE, SUIT_NORMAL);
    mock_write_u8(P_DATA + OFFSET_SAMUS_DATA_POSE, POSE_CROUCH_SHOOT);
    mock_write_u8(P_DATA + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME, frame_index);
    mock_write_u8(P_DATA + OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER, duration_counter);
}

int main() {
    std::printf("============================================================\n");
    std::printf("Running R10 SamusShootingAndCrouchingGfx Unit Test Suite\n");
    std::printf("============================================================\n\n");

    int pass_count = 0;
    int total_tests = 0;

    auto run_test = [&](const char* name, bool cond) {
        total_tests++;
        if (cond) {
            std::printf("  [PASS] Test %2d: %s\n", total_tests, name);
            pass_count++;
        } else {
            std::printf("  [FAIL] Test %2d: %s\n", total_tests, name);
        }
    };

    // -------------------------------------------------------------
    // Test 1: Logic - Animation in progress (counter < timer) returns SPOSE_NONE (0xFF)
    // -------------------------------------------------------------
    {
        setup_crouch_shoot_animation(0, 1); // frame 0, counter 1 < 4
        uint8_t out_anim = 99;
        uint8_t pose = samus_shooting_and_crouching_gfx(P_DATA, &out_anim);
        run_test("Animation in progress returns SPOSE_NONE (0xFF) & anim=0",
                 pose == SPOSE_NONE && out_anim == SAMUS_ANIM_STATE_NONE);
    }

    // -------------------------------------------------------------
    // Test 2: Logic - Animation sub-ended (advances frame) returns SPOSE_NONE (0xFF)
    // -------------------------------------------------------------
    {
        setup_crouch_shoot_animation(0, 4); // frame 0 -> frame 1 (timer=4)
        uint8_t out_anim = 99;
        uint8_t pose = samus_shooting_and_crouching_gfx(P_DATA, &out_anim);
        uint8_t frame_after = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME);
        run_test("Animation sub-ended advances frame to 1, returns SPOSE_NONE & anim=1",
                 pose == SPOSE_NONE && out_anim == SAMUS_ANIM_STATE_SUB_ENDED && frame_after == 1);
    }

    // -------------------------------------------------------------
    // Test 3: Logic - Animation ended (sequence terminator) returns SPOSE_CROUCHING (4)
    // -------------------------------------------------------------
    {
        setup_crouch_shoot_animation(1, 4); // frame 1 -> frame 2 (timer=0, ended!)
        uint8_t out_anim = 99;
        uint8_t pose = samus_shooting_and_crouching_gfx(P_DATA, &out_anim);
        run_test("Animation ended returns SPOSE_CROUCHING (4) & anim=2",
                 pose == SPOSE_CROUCHING && out_anim == SAMUS_ANIM_STATE_ENDED);
    }

    // -------------------------------------------------------------
    // Test 4: Logic - Null pointer defense returns SPOSE_NONE (0xFF)
    // -------------------------------------------------------------
    {
        uint8_t out_anim = 99;
        uint8_t pose = samus_shooting_and_crouching_gfx(0u, &out_anim);
        run_test("Null pointer defense returns SPOSE_NONE (0xFF) & anim=0",
                 pose == SPOSE_NONE && out_anim == SAMUS_ANIM_STATE_NONE);
    }

    // -------------------------------------------------------------
    // Test 5: Single Call Guarantee (verifies exactly one call to animation engine)
    // -------------------------------------------------------------
    {
        setup_crouch_shoot_animation(0, 4);
        g_mock_anim_table_reads = 0;
        uint8_t out_anim = 0;
        uint8_t pose = samus_shooting_and_crouching_gfx(P_DATA, &out_anim);
        uint8_t frame_after = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME);
        uint8_t counter_after = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER);
        run_test("Single Call Guarantee: animation engine queried exactly once (reads=1, frame 0->1, counter->0)",
                 g_mock_anim_table_reads == 1 && frame_after == 1 && counter_after == 0 && out_anim == SAMUS_ANIM_STATE_SUB_ENDED);
    }

    // -------------------------------------------------------------
    // Test 6: Thunk ABI - In progress (anim_state = 0) -> R0=0xFF, R1=caller LR, C=0, V=0, N=0, Z=0
    // -------------------------------------------------------------
    {
        setup_crouch_shoot_animation(0, 1);
        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[1] = 0xDEADBEEFu;
        cpu.R[13] = 0x03007F00u;
        cpu.R[14] = 0x08008C23u; // caller LR with Thumb bit
        for (int i = 4; i <= 11; ++i) cpu.R[i] = 0xCAFEBABE + i;
        cpu.cpsr = CPSR_N_BIT | CPSR_Z_BIT | CPSR_C_BIT | CPSR_V_BIT; // dirty flags

        thunk_samus_shooting_crouching_gfx_on_state(&cpu);

        bool reg_ok = (cpu.R[0] == SPOSE_NONE) &&
                      (cpu.R[1] == 0x08008C23u) &&
                      (cpu.R[15] == 0x08008C22u) &&
                      (cpu.cpsr & CPSR_T_BIT);
        bool canaries_ok = true;
        for (int i = 4; i <= 11; ++i) {
            if (cpu.R[i] != 0xCAFEBABE + i) canaries_ok = false;
        }
        bool flags_ok = ((cpu.cpsr & (CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT | CPSR_C_BIT)) == 0);
        bool shadow_ok = (g_last_runtime_call_return_pc == 0x08008C22u);

        run_test("Thunk ABI in progress: R0=0xFF, R1=caller LR, CPSR N=Z=C=V=0, canaries preserved",
                 reg_ok && canaries_ok && flags_ok && shadow_ok);
    }

    // -------------------------------------------------------------
    // Test 7: Thunk ABI - Sub-ended (anim_state = 1) -> R0=0xFF, R1=caller LR, C=0, V=0, N=0, Z=0
    // -------------------------------------------------------------
    {
        setup_crouch_shoot_animation(0, 4);
        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[1] = 0xDEADBEEFu;
        cpu.R[13] = 0x03007F00u;
        cpu.R[14] = 0x08008C23u;
        for (int i = 4; i <= 11; ++i) cpu.R[i] = 0xCAFEBABE + i;
        cpu.cpsr = CPSR_N_BIT | CPSR_Z_BIT | CPSR_C_BIT | CPSR_V_BIT;

        thunk_samus_shooting_crouching_gfx_on_state(&cpu);

        bool reg_ok = (cpu.R[0] == SPOSE_NONE) &&
                      (cpu.R[1] == 0x08008C23u) &&
                      (cpu.R[15] == 0x08008C22u) &&
                      (cpu.cpsr & CPSR_T_BIT);
        bool flags_ok = ((cpu.cpsr & (CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT | CPSR_C_BIT)) == 0);

        run_test("Thunk ABI sub-ended: R0=0xFF, R1=caller LR, CPSR C=0 (anim_state=1 < 2)",
                 reg_ok && flags_ok);
    }

    // -------------------------------------------------------------
    // Test 8: Thunk ABI - Ended (anim_state = 2) -> R0=4, R1=caller LR, C=1, V=0, N=0, Z=0
    // -------------------------------------------------------------
    {
        setup_crouch_shoot_animation(1, 4);
        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[1] = 0xDEADBEEFu;
        cpu.R[13] = 0x03007F00u;
        cpu.R[14] = 0x08008C23u;
        for (int i = 4; i <= 11; ++i) cpu.R[i] = 0x11223340 + i;
        cpu.cpsr = CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT; // C=0, dirty N/Z/V

        thunk_samus_shooting_crouching_gfx_on_state(&cpu);

        bool reg_ok = (cpu.R[0] == SPOSE_CROUCHING) &&
                      (cpu.R[1] == 0x08008C23u) &&
                      (cpu.R[15] == 0x08008C22u) &&
                      (cpu.cpsr & CPSR_T_BIT);
        bool canaries_ok = true;
        for (int i = 4; i <= 11; ++i) {
            if (cpu.R[i] != 0x11223340 + i) canaries_ok = false;
        }
        // cmp r0, #2 with r0=2 sets C=1, V=0; movs r0, #4 sets N=0, Z=0
        bool flags_ok = ((cpu.cpsr & (CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT)) == 0) &&
                        ((cpu.cpsr & CPSR_C_BIT) != 0);

        run_test("Thunk ABI ended: R0=4 (SPOSE_CROUCHING), R1=caller LR, CPSR C=1, N=Z=V=0",
                 reg_ok && canaries_ok && flags_ok);
    }

    // -------------------------------------------------------------
    // Test 9: Hook Filtering - Wrong Address Fallback to AOT
    // -------------------------------------------------------------
    {
        ArmCpuState cpu{};
        s_mock_enabled = true;
        s_mock_hits = 0;
        int res = hook_samus_shooting_crouching_gfx_mock(0x08008C52u, 1, &cpu);
        run_test("Hook filtering: wrong address falls back to AOT (res=0, hits=0)",
                 res == 0 && s_mock_hits == 0);
    }

    // -------------------------------------------------------------
    // Test 10: Hook Filtering - ARM mode Fallback to AOT
    // -------------------------------------------------------------
    {
        ArmCpuState cpu{};
        s_mock_enabled = true;
        s_mock_hits = 0;
        int res = hook_samus_shooting_crouching_gfx_mock(0x08008C50u, 0, &cpu);
        run_test("Hook filtering: ARM mode (thumb=0) falls back to AOT (res=0, hits=0)",
                 res == 0 && s_mock_hits == 0);
    }

    // -------------------------------------------------------------
    // Test 11: Hook Filtering - Disabled Hook Fallback to AOT
    // -------------------------------------------------------------
    {
        ArmCpuState cpu{};
        s_mock_enabled = false;
        s_mock_hits = 0;
        int res = hook_samus_shooting_crouching_gfx_mock(0x08008C50u, 1, &cpu);
        run_test("Hook filtering: disabled hook falls back to AOT (res=0, hits=0)",
                 res == 0 && s_mock_hits == 0);
    }

    // -------------------------------------------------------------
    // Test 12: Hook Interception - Enabled Hook at 0x08008C50
    // -------------------------------------------------------------
    {
        setup_crouch_shoot_animation(1, 4);
        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[14] = 0x08008C23u;
        s_mock_enabled = true;
        s_mock_hits = 0;
        int res = hook_samus_shooting_crouching_gfx_mock(0x08008C50u, 1, &cpu);
        run_test("Hook interception: enabled hook at 0x08008C50 intercepted (res=1, hits=1, R0=4)",
                 res == 1 && s_mock_hits == 1 && cpu.R[0] == SPOSE_CROUCHING);
    }

    std::printf("\n============================================================\n");
    std::printf("Result: %d / %d tests passed (%.1f%%)\n",
                pass_count, total_tests, (100.0 * pass_count) / total_tests);
    std::printf("============================================================\n");

    return (pass_count == total_tests) ? 0 : 1;
}
