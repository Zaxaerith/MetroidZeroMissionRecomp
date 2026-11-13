// test_r12.cpp — Standalone unit test for R12 SamusStartingSpinJumpGfx (0x08008ED8).
//
// Origin: MetroidZeroMissionRecomp R12 Native Verification
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <map>
#include <vector>

#include "../../src/native/samus_animation.h"
#include "../../src/native/samus_starting_spin_jump.h"

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
    if (addr >= mzm::native::ADDR_SAMUS_ANIM_POINTERS_POWER_SUIT &&
        addr < mzm::native::ADDR_SAMUS_ANIM_POINTERS_POWER_SUIT + 0x1000) {
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

void thunk_samus_starting_spin_jump_gfx_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;

    // 1. Inputs: R0 = pData (pointer to SamusData in IWRAM)
    const uint32_t p_data = cpu->R[0];

    // 2. Execute readable native C++ logic
    uint8_t anim_state = 0u;
    const uint8_t result = samus_starting_spin_jump_gfx(p_data, &anim_state);

    // 3. Return value in R0
    cpu->R[0] = static_cast<uint32_t>(result);

    // 4. Mirror original Thumb epilogue (pop {r4}; pop {r1}; bx r1):
    cpu->R[1] = cpu->R[14];

    // 5. Flags:
    // Sets C = (anim_state >= 2), V = 0, N = 0, Z = 0
    cpu->cpsr &= ~(CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT | CPSR_C_BIT);
    if (anim_state >= 2u) {
        cpu->cpsr |= CPSR_C_BIT;
    }

    // 6. Architectural PC & Thumb mode return contract
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

int hook_samus_starting_spin_jump_gfx_mock(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept {
    if (addr != 0x08008ED8u || !thumb || !cpu) {
        return 0;
    }
    if (!s_mock_enabled) {
        return 0;
    }
    thunk_samus_starting_spin_jump_gfx_on_state(cpu);
    s_mock_hits++;
    return 1;
}

} // namespace mzm::native

using namespace mzm::native;

static const uint32_t P_DATA = 0x030013D4u;
static const uint32_t ANIM_BASE = 0x08253000u;
static const uint8_t POSE_STARTING_SPIN_JUMP = 11u;

static void setup_starting_spin_jump_animation(uint8_t frame_index, uint8_t duration_counter) {
    g_mock_mem.clear();

    // Register animation pointer in Power Suit table for pose 11
    mock_write_u32(ADDR_SAMUS_ANIM_POINTERS_POWER_SUIT + POSE_STARTING_SPIN_JUMP * 8, ANIM_BASE);

    // Frame 0: timer = 4
    mock_write_u8(ANIM_BASE + 0 * SIZEOF_SAMUS_ANIMATION_DATA + OFFSET_SAMUS_ANIMATION_DATA_TIMER, 4);
    // Frame 1: timer = 4
    mock_write_u8(ANIM_BASE + 1 * SIZEOF_SAMUS_ANIMATION_DATA + OFFSET_SAMUS_ANIMATION_DATA_TIMER, 4);
    // Frame 2: timer = 0 (sequence terminator)
    mock_write_u8(ANIM_BASE + 2 * SIZEOF_SAMUS_ANIMATION_DATA + OFFSET_SAMUS_ANIMATION_DATA_TIMER, 0);

    // Setup SamusData
    mock_write_u8(ADDR_G_EQUIPMENT + OFFSET_EQUIPMENT_SUIT_TYPE, SUIT_NORMAL);
    mock_write_u8(P_DATA + OFFSET_SAMUS_DATA_POSE, POSE_STARTING_SPIN_JUMP);
    mock_write_u8(P_DATA + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME, frame_index);
    mock_write_u8(P_DATA + OFFSET_SAMUS_DATA_ANIMATION_DURATION_COUNTER, duration_counter);
}

int main() {
    std::printf("============================================================\n");
    std::printf("Running R12 SamusStartingSpinJumpGfx Unit Test Suite\n");
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
    // Test 1: Path 1 Logic — Animation in progress returns SPOSE_NONE (0xFF)
    // -------------------------------------------------------------
    {
        setup_starting_spin_jump_animation(0, 1);
        uint8_t out_anim = 99;
        uint8_t ret = samus_starting_spin_jump_gfx(P_DATA, &out_anim);
        uint8_t final_pose = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_POSE);
        uint8_t final_frame = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME);
        run_test("Path 1 Logic: in progress returns SPOSE_NONE (0xFF) & anim=0, pose unchanged",
                 ret == SPOSE_NONE && out_anim == SAMUS_ANIM_STATE_NONE &&
                 final_pose == POSE_STARTING_SPIN_JUMP && final_frame == 0);
    }

    // -------------------------------------------------------------
    // Test 2: Path 1b Logic — Animation sub-ended advances frame, returns SPOSE_NONE
    // -------------------------------------------------------------
    {
        setup_starting_spin_jump_animation(0, 4);
        uint8_t out_anim = 99;
        uint8_t ret = samus_starting_spin_jump_gfx(P_DATA, &out_anim);
        uint8_t final_pose = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_POSE);
        uint8_t final_frame = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME);
        run_test("Path 1b Logic: sub-ended returns SPOSE_NONE (0xFF) & anim=1, pose unchanged, frame=1",
                 ret == SPOSE_NONE && out_anim == SAMUS_ANIM_STATE_SUB_ENDED &&
                 final_pose == POSE_STARTING_SPIN_JUMP && final_frame == 1);
    }

    // -------------------------------------------------------------
    // Test 3: Path 2 Logic — Animation ended transitions to SPOSE_SPINNING (12) & frame 0
    // -------------------------------------------------------------
    {
        // Frame 1 with duration 4 advances to Frame 2 (timer 0), triggering ended state
        setup_starting_spin_jump_animation(1, 4);
        uint8_t out_anim = 99;
        uint8_t ret = samus_starting_spin_jump_gfx(P_DATA, &out_anim);
        uint8_t final_pose = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_POSE);
        uint8_t final_frame = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME);
        run_test("Path 2 Logic: ended returns SPOSE_NONE, pose=SPOSE_SPINNING(12), frame=0",
                 ret == SPOSE_NONE && out_anim == SAMUS_ANIM_STATE_ENDED &&
                 final_pose == SPOSE_SPINNING && final_frame == 0);
    }

    // -------------------------------------------------------------
    // Test 4: Single Call Guarantee — Calls samus_update_animation exactly once
    // -------------------------------------------------------------
    {
        setup_starting_spin_jump_animation(1, 4);
        g_mock_anim_table_reads = 0;
        samus_starting_spin_jump_gfx(P_DATA, nullptr);
        run_test("Single Call Guarantee: animation table read exactly once",
                 g_mock_anim_table_reads == 1);
    }

    // -------------------------------------------------------------
    // Test 5: Defensive Null Pointer Check
    // -------------------------------------------------------------
    {
        uint8_t out_anim = 99;
        uint8_t ret = samus_starting_spin_jump_gfx(0u, &out_anim);
        run_test("Null Safety: returns SPOSE_NONE and anim=0 on null pData",
                 ret == SPOSE_NONE && out_anim == SAMUS_ANIM_STATE_NONE);
    }

    // -------------------------------------------------------------
    // Test 6: Thunk ABI Path 1 — In progress (anim_state = 0 < 2 -> C = 0)
    // -------------------------------------------------------------
    {
        setup_starting_spin_jump_animation(0, 1);
        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[1] = 0x11111111u;
        cpu.R[4] = 0xCAFEBABE;
        cpu.R[14] = 0x08001235u; // Thumb return address
        cpu.cpsr = CPSR_N_BIT | CPSR_Z_BIT | CPSR_C_BIT | CPSR_V_BIT; // initially set flags
        g_last_runtime_call_return_pc = 0;

        thunk_samus_starting_spin_jump_gfx_on_state(&cpu);

        bool r0_ok = (cpu.R[0] == SPOSE_NONE);
        bool r1_ok = (cpu.R[1] == 0x08001235u);
        bool r4_ok = (cpu.R[4] == 0xCAFEBABE);
        bool pc_ok = (cpu.R[15] == 0x08001234u);
        bool t_ok  = (cpu.cpsr & CPSR_T_BIT) != 0;
        bool c_ok  = (cpu.cpsr & CPSR_C_BIT) == 0; // anim_state < 2 -> C = 0
        bool nz_ok = (cpu.cpsr & (CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT)) == 0;
        bool ret_pc_ok = (g_last_runtime_call_return_pc == 0x08001234u);

        run_test("Thunk ABI: in-progress R0=0xFF, R1=LR, R4 intact, PC=LR&~1, C=0, N=Z=V=0",
                 r0_ok && r1_ok && r4_ok && pc_ok && t_ok && c_ok && nz_ok && ret_pc_ok);
    }

    // -------------------------------------------------------------
    // Test 7: Thunk ABI Path 1b — Sub-ended (anim_state = 1 < 2 -> C = 0)
    // -------------------------------------------------------------
    {
        setup_starting_spin_jump_animation(0, 4);
        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[1] = 0x11111111u;
        cpu.R[4] = 0xDEADBEEF;
        cpu.R[14] = 0x08009875u;
        cpu.cpsr = CPSR_N_BIT | CPSR_Z_BIT | CPSR_C_BIT | CPSR_V_BIT;

        thunk_samus_starting_spin_jump_gfx_on_state(&cpu);

        bool r0_ok = (cpu.R[0] == SPOSE_NONE);
        bool r1_ok = (cpu.R[1] == 0x08009875u);
        bool r4_ok = (cpu.R[4] == 0xDEADBEEF);
        bool pc_ok = (cpu.R[15] == 0x08009874u);
        bool c_ok  = (cpu.cpsr & CPSR_C_BIT) == 0; // anim_state = 1 < 2 -> C = 0
        bool nz_ok = (cpu.cpsr & (CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT)) == 0;

        run_test("Thunk ABI: sub-ended anim=1 -> C=0, N=Z=V=0, R0=0xFF",
                 r0_ok && r1_ok && r4_ok && pc_ok && c_ok && nz_ok);
    }

    // -------------------------------------------------------------
    // Test 8: Thunk ABI Path 2 — Ended (anim_state = 2 >= 2 -> C = 1)
    // -------------------------------------------------------------
    {
        setup_starting_spin_jump_animation(1, 4);
        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[1] = 0x11111111u;
        cpu.R[4] = 0xFEEDFACE;
        cpu.R[14] = 0x08005555u;
        cpu.cpsr = 0; // initially cleared flags

        thunk_samus_starting_spin_jump_gfx_on_state(&cpu);

        bool r0_ok = (cpu.R[0] == SPOSE_NONE);
        bool r1_ok = (cpu.R[1] == 0x08005555u);
        bool r4_ok = (cpu.R[4] == 0xFEEDFACE);
        bool pc_ok = (cpu.R[15] == 0x08005554u);
        bool t_ok  = (cpu.cpsr & CPSR_T_BIT) != 0;
        bool c_ok  = (cpu.cpsr & CPSR_C_BIT) != 0; // anim_state = 2 >= 2 -> C = 1
        bool nz_ok = (cpu.cpsr & (CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT)) == 0;
        uint8_t final_pose = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_POSE);
        uint8_t final_frame = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_CURRENT_ANIMATION_FRAME);

        run_test("Thunk ABI: ended R0=0xFF, C=1, N=Z=V=0, pose=12, frame=0",
                 r0_ok && r1_ok && r4_ok && pc_ok && t_ok && c_ok && nz_ok &&
                 final_pose == SPOSE_SPINNING && final_frame == 0);
    }

    // -------------------------------------------------------------
    // Test 9: Hook Filter & Enable/Disable
    // -------------------------------------------------------------
    {
        setup_starting_spin_jump_animation(0, 1);
        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[14] = 0x08001111u;

        s_mock_enabled = false;
        s_mock_hits = 0;

        int res1 = hook_samus_starting_spin_jump_gfx_mock(0x08008ED8u, 1, &cpu);
        run_test("Hook Filter: disabled returns 0, no hits", res1 == 0 && s_mock_hits == 0);

        s_mock_enabled = true;
        int res2 = hook_samus_starting_spin_jump_gfx_mock(0x08008ED9u, 1, &cpu); // bad addr
        int res3 = hook_samus_starting_spin_jump_gfx_mock(0x08008ED8u, 0, &cpu); // arm mode
        int res4 = hook_samus_starting_spin_jump_gfx_mock(0x08008ED8u, 1, nullptr); // null cpu
        run_test("Hook Filter: invalid address/mode/cpu rejected",
                 res2 == 0 && res3 == 0 && res4 == 0 && s_mock_hits == 0);

        int res5 = hook_samus_starting_spin_jump_gfx_mock(0x08008ED8u, 1, &cpu);
        run_test("Hook Filter: enabled valid call returns 1, increments hits",
                 res5 == 1 && s_mock_hits == 1 && cpu.R[0] == SPOSE_NONE);
    }

    std::printf("\n------------------------------------------------------------\n");
    std::printf("Summary: %d / %d tests passed\n", pass_count, total_tests);
    std::printf("------------------------------------------------------------\n");

    return (pass_count == total_tests) ? 0 : 1;
}
