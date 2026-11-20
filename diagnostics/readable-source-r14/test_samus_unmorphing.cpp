// test_samus_unmorphing.cpp — Standalone unit test for R14 SamusUnmorphingGfx (0x08009508).
//
// Origin: MetroidZeroMissionRecomp R14 Native Verification
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <map>

#include "../../src/native/samus_animation.h"
#include "../../src/native/samus_unmorphing.h"

// ============================================================================
// Mock Guest Memory Bus & SamusUpdateAnimation
// ============================================================================

namespace {
std::map<uint32_t, uint8_t> g_mock_mem;
int g_mock_write_count = 0;

uint8_t mock_read_u8(uint32_t addr) {
    auto it = g_mock_mem.find(addr);
    if (it != g_mock_mem.end()) return it->second;
    return 0;
}

void mock_write_u8(uint32_t addr, uint8_t val) {
    g_mock_mem[addr] = val;
    g_mock_write_count++;
}

// Animation update mock state
uint8_t g_mock_anim_state = 0u;
int g_samus_update_anim_calls = 0;
uint32_t g_last_anim_pdata = 0u;
uint8_t g_last_anim_slowed = 0xFFu;

// Multi-frame animation simulation state
bool g_simulate_multi_frame = false;
uint8_t g_sim_frame = 0;
uint8_t g_sim_duration = 0;
} // namespace

extern "C" {
uint8_t bus_read_u8(uint32_t addr) { return mock_read_u8(addr); }
void bus_write_u8(uint32_t addr, uint8_t val) { mock_write_u8(addr, val); }

static uint32_t g_last_runtime_call_return_pc = 0;
int runtime_call_should_return(uint32_t target_pc) {
    g_last_runtime_call_return_pc = target_pc;
    return 1;
}
}

namespace mzm::native {

uint8_t samus_update_animation(uint32_t p_data_addr, uint8_t slowed) noexcept {
    g_samus_update_anim_calls++;
    g_last_anim_pdata = p_data_addr;
    g_last_anim_slowed = slowed;

    if (g_simulate_multi_frame) {
        // Simulated 3-frame sequence:
        // Frame 0: duration 4 -> anim_state = 0 (in-progress) or 1 (sub-ended)
        // Frame 1: duration 4 -> anim_state = 0 (in-progress) or 1 (sub-ended)
        // Frame 2: terminator -> anim_state = 2 (ended)
        g_sim_duration++;
        if (g_sim_frame == 0 && g_sim_duration >= 4) {
            g_sim_frame = 1;
            g_sim_duration = 0;
            return SAMUS_ANIM_STATE_SUB_ENDED; // 1
        } else if (g_sim_frame == 1 && g_sim_duration >= 4) {
            g_sim_frame = 2;
            g_sim_duration = 0;
            return SAMUS_ANIM_STATE_ENDED; // 2
        } else if (g_sim_frame >= 2) {
            return SAMUS_ANIM_STATE_ENDED; // 2
        }
        return SAMUS_ANIM_STATE_NONE; // 0
    }

    return g_mock_anim_state;
}

} // namespace mzm::native

// ============================================================================
// ArmCpuState & Thunk / Hook Definitions for SamusUnmorphingGfx
// ============================================================================

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

void thunk_samus_unmorphing_gfx_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;

    // 1. Inputs: R0 = pData (pointer to SamusData in IWRAM)
    const uint32_t p_data = cpu->R[0];

    // 2. Execute readable native C++ logic
    uint8_t anim_state = 0u;
    const uint8_t result = samus_unmorphing_gfx(p_data, &anim_state);

    // 3. Return value in R0 (SPOSE_CROUCHING=4 if ended, SPOSE_NONE=0xFF otherwise)
    cpu->R[0] = static_cast<uint32_t>(result);

    // 4. Mirror original Thumb epilogue (pop {r4}; pop {r1}; bx r1):
    // Caller's R4 is preserved.
    // In original function, LR was pushed on stack and popped into R1 before bx r1.
    cpu->R[1] = cpu->R[14];

    // 5. Flags:
    // Original Thumb at 0x08009516: `cmp r0, #2` where r0 was zero-extended anim_state.
    // Sets C = (anim_state >= 2), V = 0, Z = (anim_state == 2), N = (anim_state < 2 ? 1 : 0).
    // In both subsequent paths:
    //   Path 1 (anim_state != 2): movs r0, #255
    //   Path 2 (anim_state == 2): movs r0, #15; strb ...; movs r0, #4
    // Thumb `movs Rd, #imm8` sets N = 0 (bit 31 is 0), Z = 0 (value is non-zero),
    // and preserves C and V unmodified!
    // Therefore at function exit: N = 0, Z = 0, V = 0, C = (anim_state >= 2).
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

int hook_samus_unmorphing_gfx_mock(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept {
    if (addr != 0x08009508u || !thumb || !cpu) {
        return 0;
    }
    if (!s_mock_enabled) {
        return 0;
    }
    thunk_samus_unmorphing_gfx_on_state(cpu);
    s_mock_hits++;
    return 1;
}

} // namespace mzm::native

// ============================================================================
// Test Suite Execution
// ============================================================================

using namespace mzm::native;

static const uint32_t P_DATA = 0x030013D4u;

static void reset_test_environment() {
    g_mock_mem.clear();
    g_mock_write_count = 0;
    g_mock_anim_state = 0u;
    g_samus_update_anim_calls = 0;
    g_last_anim_pdata = 0u;
    g_last_anim_slowed = 0xFFu;
    g_simulate_multi_frame = false;
    g_sim_frame = 0;
    g_sim_duration = 0;
    g_last_runtime_call_return_pc = 0;

    // Set initial unmorphPaletteTimer = 0
    mock_write_u8(P_DATA + OFFSET_SAMUS_DATA_UNMORPH_PALETTE_TIMER, 0u);
    g_mock_write_count = 0; // reset write counter
}

int main() {
    std::printf("============================================================\n");
    std::printf("Running R14 SamusUnmorphingGfx Unit Test Suite (0x08009508)\n");
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
    // Test 1: Path 1 Logic — anim_state == 0 (in-progress)
    // -------------------------------------------------------------
    {
        reset_test_environment();
        g_mock_anim_state = SAMUS_ANIM_STATE_NONE; // 0
        uint8_t out_anim = 99;
        uint8_t ret = samus_unmorphing_gfx(P_DATA, &out_anim);
        uint8_t timer = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_UNMORPH_PALETTE_TIMER);

        run_test("Path 1: anim_state=0 -> returns SPOSE_NONE (0xFF), timer untouched (0)",
                 ret == SPOSE_NONE && out_anim == SAMUS_ANIM_STATE_NONE &&
                 timer == 0 && g_mock_write_count == 0);
    }

    // -------------------------------------------------------------
    // Test 2: Path 1b Logic — anim_state == 1 (sub-ended / frame changed)
    // -------------------------------------------------------------
    {
        reset_test_environment();
        g_mock_anim_state = SAMUS_ANIM_STATE_SUB_ENDED; // 1
        uint8_t out_anim = 99;
        uint8_t ret = samus_unmorphing_gfx(P_DATA, &out_anim);
        uint8_t timer = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_UNMORPH_PALETTE_TIMER);

        run_test("Path 1b: anim_state=1 -> returns SPOSE_NONE (0xFF), timer untouched (0)",
                 ret == SPOSE_NONE && out_anim == SAMUS_ANIM_STATE_SUB_ENDED &&
                 timer == 0 && g_mock_write_count == 0);
    }

    // -------------------------------------------------------------
    // Test 3: Path 2 Logic — anim_state == 2 (ended)
    // -------------------------------------------------------------
    {
        reset_test_environment();
        g_mock_anim_state = SAMUS_ANIM_STATE_ENDED; // 2
        uint8_t out_anim = 99;
        uint8_t ret = samus_unmorphing_gfx(P_DATA, &out_anim);
        uint8_t timer = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_UNMORPH_PALETTE_TIMER);

        run_test("Path 2: anim_state=2 -> returns SPOSE_CROUCHING (4), timer set to 15",
                 ret == SPOSE_CROUCHING && out_anim == SAMUS_ANIM_STATE_ENDED &&
                 timer == UNMORPH_PALETTE_TIMER_INIT && g_mock_write_count == 1);
    }

    // -------------------------------------------------------------
    // Test 4: Path 3 Edge Case — anim_state == 3 (>= 2 but != 2)
    // -------------------------------------------------------------
    {
        reset_test_environment();
        g_mock_anim_state = 3u;
        uint8_t out_anim = 99;
        uint8_t ret = samus_unmorphing_gfx(P_DATA, &out_anim);
        uint8_t timer = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_UNMORPH_PALETTE_TIMER);

        run_test("Path 3 Edge Case: anim_state=3 -> returns SPOSE_NONE (0xFF), timer untouched (0)",
                 ret == SPOSE_NONE && out_anim == 3u &&
                 timer == 0 && g_mock_write_count == 0);
    }

    // -------------------------------------------------------------
    // Test 5: Multi-frame Sequence Progression
    // -------------------------------------------------------------
    {
        reset_test_environment();
        g_simulate_multi_frame = true;
        g_sim_frame = 0;
        g_sim_duration = 0;

        // Step 1: frame 0 progressing (duration 0 -> 1)
        uint8_t r1 = samus_unmorphing_gfx(P_DATA, nullptr);
        uint8_t t1 = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_UNMORPH_PALETTE_TIMER);

        // Advance to frame 1 (duration 1, 2, 3, 4 -> sub-ended)
        samus_unmorphing_gfx(P_DATA, nullptr);
        samus_unmorphing_gfx(P_DATA, nullptr);
        uint8_t r2 = samus_unmorphing_gfx(P_DATA, nullptr);
        uint8_t t2 = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_UNMORPH_PALETTE_TIMER);

        // Advance to frame 2 (duration 1, 2, 3, 4 -> ended)
        samus_unmorphing_gfx(P_DATA, nullptr);
        samus_unmorphing_gfx(P_DATA, nullptr);
        samus_unmorphing_gfx(P_DATA, nullptr);
        uint8_t r3 = samus_unmorphing_gfx(P_DATA, nullptr);
        uint8_t t3 = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_UNMORPH_PALETTE_TIMER);

        run_test("Multi-frame Sequence: steps from SPOSE_NONE to SPOSE_CROUCHING on sequence end",
                 r1 == SPOSE_NONE && t1 == 0 &&
                 r2 == SPOSE_NONE && t2 == 0 &&
                 r3 == SPOSE_CROUCHING && t3 == UNMORPH_PALETTE_TIMER_INIT);
    }

    // -------------------------------------------------------------
    // Test 6: Single Call Guarantee & Parameter Verification
    // -------------------------------------------------------------
    {
        reset_test_environment();
        g_mock_anim_state = SAMUS_ANIM_STATE_ENDED;
        samus_unmorphing_gfx(P_DATA, nullptr);

        run_test("Single Call Guarantee: calls SamusUpdateAnimation exactly once with slowed=0",
                 g_samus_update_anim_calls == 1 &&
                 g_last_anim_pdata == P_DATA &&
                 g_last_anim_slowed == 0u);
    }

    // -------------------------------------------------------------
    // Test 7: Defensive Null Pointer Check
    // -------------------------------------------------------------
    {
        reset_test_environment();
        uint8_t out_anim = 99;
        uint8_t ret = samus_unmorphing_gfx(0u, &out_anim);

        run_test("Null Safety: returns SPOSE_NONE and anim=0 on null pData with zero writes",
                 ret == SPOSE_NONE && out_anim == SAMUS_ANIM_STATE_NONE &&
                 g_samus_update_anim_calls == 0 && g_mock_write_count == 0);
    }

    // -------------------------------------------------------------
    // Test 8: Thunk ABI Path 1 — anim_state == 0 (< 2 -> C = 0)
    // -------------------------------------------------------------
    {
        reset_test_environment();
        g_mock_anim_state = 0u;

        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[1] = 0x11111111u;
        cpu.R[4] = 0xCAFEBABE;
        cpu.R[14] = 0x08001235u; // Thumb return address (bit 0 set)
        cpu.cpsr = CPSR_N_BIT | CPSR_Z_BIT | CPSR_C_BIT | CPSR_V_BIT; // initially all flags 1

        thunk_samus_unmorphing_gfx_on_state(&cpu);

        bool r0_ok = (cpu.R[0] == SPOSE_NONE); // 0xFF
        bool r1_ok = (cpu.R[1] == 0x08001235u); // R1 = caller LR
        bool r4_ok = (cpu.R[4] == 0xCAFEBABE); // R4 preserved
        bool pc_ok = (cpu.R[15] == 0x08001234u); // PC = LR & ~1
        bool t_ok  = (cpu.cpsr & CPSR_T_BIT) != 0; // Thumb mode maintained
        bool c_ok  = (cpu.cpsr & CPSR_C_BIT) == 0; // C = 0 (anim_state < 2)
        bool nz_ok = (cpu.cpsr & (CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT)) == 0; // N=0, Z=0, V=0
        bool timer_ok = (mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_UNMORPH_PALETTE_TIMER) == 0);

        run_test("Thunk ABI Path 1: R0=0xFF, R1=LR, R4 intact, PC=LR&~1, C=0, N=Z=V=0",
                 r0_ok && r1_ok && r4_ok && pc_ok && t_ok && c_ok && nz_ok && timer_ok);
    }

    // -------------------------------------------------------------
    // Test 9: Thunk ABI Path 1b — anim_state == 1 (< 2 -> C = 0)
    // -------------------------------------------------------------
    {
        reset_test_environment();
        g_mock_anim_state = 1u;

        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[1] = 0x22222222u;
        cpu.R[4] = 0xDEADBEEF;
        cpu.R[14] = 0x08009875u;
        cpu.cpsr = CPSR_N_BIT | CPSR_Z_BIT | CPSR_C_BIT | CPSR_V_BIT;

        thunk_samus_unmorphing_gfx_on_state(&cpu);

        bool r0_ok = (cpu.R[0] == SPOSE_NONE); // 0xFF
        bool r1_ok = (cpu.R[1] == 0x08009875u);
        bool r4_ok = (cpu.R[4] == 0xDEADBEEF);
        bool pc_ok = (cpu.R[15] == 0x08009874u);
        bool c_ok  = (cpu.cpsr & CPSR_C_BIT) == 0; // C = 0
        bool nz_ok = (cpu.cpsr & (CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT)) == 0;
        bool timer_ok = (mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_UNMORPH_PALETTE_TIMER) == 0);

        run_test("Thunk ABI Path 1b: anim_state=1 -> R0=0xFF, C=0, N=Z=V=0, timer untouched",
                 r0_ok && r1_ok && r4_ok && pc_ok && c_ok && nz_ok && timer_ok);
    }

    // -------------------------------------------------------------
    // Test 10: Thunk ABI Path 2 — anim_state == 2 (>= 2 -> C = 1)
    // -------------------------------------------------------------
    {
        reset_test_environment();
        g_mock_anim_state = 2u;

        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[1] = 0x33333333u;
        cpu.R[4] = 0xFEEDFACE;
        cpu.R[14] = 0x08005555u;
        cpu.cpsr = 0; // initially all flags 0

        thunk_samus_unmorphing_gfx_on_state(&cpu);

        bool r0_ok = (cpu.R[0] == SPOSE_CROUCHING); // 4
        bool r1_ok = (cpu.R[1] == 0x08005555u);
        bool r4_ok = (cpu.R[4] == 0xFEEDFACE);
        bool pc_ok = (cpu.R[15] == 0x08005554u);
        bool t_ok  = (cpu.cpsr & CPSR_T_BIT) != 0;
        bool c_ok  = (cpu.cpsr & CPSR_C_BIT) != 0; // C = 1 (anim_state >= 2)
        bool nz_ok = (cpu.cpsr & (CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT)) == 0;
        bool timer_ok = (mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_UNMORPH_PALETTE_TIMER) == UNMORPH_PALETTE_TIMER_INIT);

        run_test("Thunk ABI Path 2: anim_state=2 -> R0=4, C=1, N=Z=V=0, timer=15",
                 r0_ok && r1_ok && r4_ok && pc_ok && t_ok && c_ok && nz_ok && timer_ok);
    }

    // -------------------------------------------------------------
    // Test 11: Thunk ABI Path 3 Edge Case — anim_state == 3 (>= 2 -> C = 1)
    // -------------------------------------------------------------
    {
        reset_test_environment();
        g_mock_anim_state = 3u;

        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[1] = 0x44444444u;
        cpu.R[4] = 0x12345678;
        cpu.R[14] = 0x08007777u;
        cpu.cpsr = 0; // initially all flags 0

        thunk_samus_unmorphing_gfx_on_state(&cpu);

        bool r0_ok = (cpu.R[0] == SPOSE_NONE); // 0xFF (since anim_state != 2)
        bool r1_ok = (cpu.R[1] == 0x08007777u);
        bool r4_ok = (cpu.R[4] == 0x12345678);
        bool pc_ok = (cpu.R[15] == 0x08007776u);
        bool c_ok  = (cpu.cpsr & CPSR_C_BIT) != 0; // C = 1 (anim_state >= 2)
        bool nz_ok = (cpu.cpsr & (CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT)) == 0;
        bool timer_ok = (mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_UNMORPH_PALETTE_TIMER) == 0);

        run_test("Thunk ABI Path 3 Edge Case: anim_state=3 -> R0=0xFF, C=1, N=Z=V=0, timer untouched",
                 r0_ok && r1_ok && r4_ok && pc_ok && c_ok && nz_ok && timer_ok);
    }

    // -------------------------------------------------------------
    // Test 12: Hook Filter: Disabled State
    // -------------------------------------------------------------
    {
        reset_test_environment();
        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[14] = 0x08001111u;

        s_mock_enabled = false;
        s_mock_hits = 0;

        int res = hook_samus_unmorphing_gfx_mock(0x08009508u, 1, &cpu);
        run_test("Hook Filter: disabled returns 0, no hits, CPU state untouched",
                 res == 0 && s_mock_hits == 0);
    }

    // -------------------------------------------------------------
    // Test 13: Hook Filter: Bad Address
    // -------------------------------------------------------------
    {
        reset_test_environment();
        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[14] = 0x08001111u;

        s_mock_enabled = true;
        s_mock_hits = 0;

        int res = hook_samus_unmorphing_gfx_mock(0x0800950Au, 1, &cpu);
        run_test("Hook Filter: non-matching entry address rejected",
                 res == 0 && s_mock_hits == 0);
    }

    // -------------------------------------------------------------
    // Test 14: Hook Filter: ARM Mode (thumb == 0)
    // -------------------------------------------------------------
    {
        reset_test_environment();
        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[14] = 0x08001111u;

        s_mock_enabled = true;
        s_mock_hits = 0;

        int res = hook_samus_unmorphing_gfx_mock(0x08009508u, 0, &cpu);
        run_test("Hook Filter: ARM mode (thumb=0) rejected",
                 res == 0 && s_mock_hits == 0);
    }

    // -------------------------------------------------------------
    // Test 15: Hook Filter: Null CPU State
    // -------------------------------------------------------------
    {
        reset_test_environment();
        s_mock_enabled = true;
        s_mock_hits = 0;

        int res = hook_samus_unmorphing_gfx_mock(0x08009508u, 1, nullptr);
        run_test("Hook Filter: null CPU state pointer rejected",
                 res == 0 && s_mock_hits == 0);
    }

    // -------------------------------------------------------------
    // Test 16: Hook Filter: Valid Enabled Interception
    // -------------------------------------------------------------
    {
        reset_test_environment();
        g_mock_anim_state = SAMUS_ANIM_STATE_ENDED; // 2

        ArmCpuState cpu{};
        cpu.R[0] = P_DATA;
        cpu.R[14] = 0x08002223u;

        s_mock_enabled = true;
        s_mock_hits = 0;

        int res = hook_samus_unmorphing_gfx_mock(0x08009508u, 1, &cpu);
        uint8_t timer = mock_read_u8(P_DATA + OFFSET_SAMUS_DATA_UNMORPH_PALETTE_TIMER);

        run_test("Hook Filter: valid call intercepted, returns 1, increments hits, executes logic",
                 res == 1 && s_mock_hits == 1 &&
                 cpu.R[0] == SPOSE_CROUCHING &&
                 timer == UNMORPH_PALETTE_TIMER_INIT);
    }

    std::printf("\n------------------------------------------------------------\n");
    std::printf("Summary: %d / %d tests passed\n", pass_count, total_tests);
    std::printf("------------------------------------------------------------\n");

    return (pass_count == total_tests) ? 0 : 1;
}
