// test_samus_starting_wall_jump.cpp — Standalone unit test for R14 SamusStartingWallJumpGfx (0x080090E0).
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
#include "../../src/native/samus_starting_wall_jump.h"

// ============================================================================
// Guest Memory & Mock Infrastructure
// ============================================================================

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

// ============================================================================
// Mock SamusUpdateAnimation
// ============================================================================

namespace mzm::native {
static uint8_t g_mock_anim_state = 0u;
static uint32_t g_last_anim_pdata = 0u;
static uint8_t g_last_anim_slowed = 0xFFu;
static int g_anim_call_count = 0;

uint8_t samus_update_animation(uint32_t p_data_addr, uint8_t slowed) noexcept {
    g_anim_call_count++;
    g_last_anim_pdata = p_data_addr;
    g_last_anim_slowed = slowed;
    return g_mock_anim_state;
}
} // namespace mzm::native

// ============================================================================
// ARM CPU State & Thunk Definition (for ABI & CPSR verification)
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

/**
 * @brief Thunk for SamusStartingWallJumpGfx (0x080090E0).
 *
 * Implements the exact register and CPSR contract derived from Thumb disassembly:
 * - R0: return pose (SPOSE_MID_AIR_REQUEST=0xFE if anim_state==2, else SPOSE_NONE=0xFF)
 * - R1: caller LR (original function did `push {r4, lr}` ... `pop {r4}; pop {r1}; bx r1`)
 * - R4: preserved (callee-saved)
 * - PC: caller LR & ~1 (Thumb bit restored to CPSR T-bit)
 * - CPSR flags:
 *     cmp r0, #2 => N=0, Z=(anim_state==2), C=(anim_state>=2), V=0
 *     movs Rd, #imm8 sets N=0, Z=0 and preserves C and V.
 *     Therefore: N=0, Z=0, V=0, C=(anim_state >= 2).
 */
void thunk_samus_starting_wall_jump_gfx_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;

    // 1. Inputs: R0 = pData (pointer to SamusData in IWRAM)
    const uint32_t p_data = cpu->R[0];

    // 2. Execute readable native C++ logic
    uint8_t anim_state = 0u;
    const uint8_t result = samus_starting_wall_jump_gfx(p_data, &anim_state);

    // 3. Return value in R0
    cpu->R[0] = static_cast<uint32_t>(result);

    // 4. Mirror original Thumb epilogue (pop {r4}; pop {r1}; bx r1):
    // Caller R4 preserved.
    // In original function, LR was pushed on stack and popped into R1 before bx r1.
    cpu->R[1] = cpu->R[14];

    // 5. Flags:
    // Original Thumb: `cmp r0, #2` where r0 was anim_state.
    // Sets C = (anim_state >= 2), V = 0.
    // In both paths (ended movs r0, #254 and not-ended movs r0, #255),
    // movs Rd, #imm8 sets N = 0, Z = 0 and preserves C and V.
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

} // namespace mzm::native

using namespace mzm::native;

static const uint32_t TEST_PDATA = 0x030013D4u;

static void reset_test_state() {
    g_mock_mem.clear();
    g_mock_anim_state = 0u;
    g_last_anim_pdata = 0u;
    g_last_anim_slowed = 0xFFu;
    g_anim_call_count = 0;
    g_last_runtime_call_return_pc = 0u;

    // Initialize SamusData fields around forcedMovement (offset 0x04)
    mock_write_u8(TEST_PDATA + 0x03, 0xAA); // turning flag (preceding byte)
    mock_write_u8(TEST_PDATA + OFFSET_SAMUS_DATA_FORCED_MOVEMENT, 0x00); // forcedMovement = 0
    mock_write_u8(TEST_PDATA + 0x05, 0xBB); // speedboostingShinesparking (following byte)
}

int main() {
    std::printf("============================================================\n");
    std::printf("Running R14 SamusStartingWallJumpGfx Unit Test Suite\n");
    std::printf("Target Function: SamusStartingWallJumpGfx (ROM 0x080090E0)\n");
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

    // ------------------------------------------------------------------------
    // Test 1: Logic — anim_state == 0 (animation in progress)
    // ------------------------------------------------------------------------
    {
        reset_test_state();
        g_mock_anim_state = SAMUS_ANIM_STATE_NONE; // 0

        uint8_t out_anim = 99;
        uint8_t ret = samus_starting_wall_jump_gfx(TEST_PDATA, &out_anim);
        uint8_t final_forced = mock_read_u8(TEST_PDATA + OFFSET_SAMUS_DATA_FORCED_MOVEMENT);

        run_test("Logic: anim_state==0 (in progress) -> returns SPOSE_NONE (0xFF), anim=0, forcedMovement untouched (0)",
                 ret == SPOSE_NONE && out_anim == 0 && final_forced == 0);
    }

    // ------------------------------------------------------------------------
    // Test 2: Logic — anim_state == 1 (sub-animation ended)
    // ------------------------------------------------------------------------
    {
        reset_test_state();
        g_mock_anim_state = SAMUS_ANIM_STATE_SUB_ENDED; // 1

        uint8_t out_anim = 99;
        uint8_t ret = samus_starting_wall_jump_gfx(TEST_PDATA, &out_anim);
        uint8_t final_forced = mock_read_u8(TEST_PDATA + OFFSET_SAMUS_DATA_FORCED_MOVEMENT);

        run_test("Logic: anim_state==1 (sub-ended) -> returns SPOSE_NONE (0xFF), anim=1, forcedMovement untouched (0)",
                 ret == SPOSE_NONE && out_anim == 1 && final_forced == 0);
    }

    // ------------------------------------------------------------------------
    // Test 3: Logic — anim_state == 2 (SAMUS_ANIM_STATE_ENDED)
    // ------------------------------------------------------------------------
    {
        reset_test_state();
        g_mock_anim_state = SAMUS_ANIM_STATE_ENDED; // 2

        uint8_t out_anim = 99;
        uint8_t ret = samus_starting_wall_jump_gfx(TEST_PDATA, &out_anim);
        uint8_t final_forced = mock_read_u8(TEST_PDATA + OFFSET_SAMUS_DATA_FORCED_MOVEMENT);

        run_test("Logic: anim_state==2 (ended) -> returns SPOSE_MID_AIR_REQUEST (0xFE), anim=2, forcedMovement=1",
                 ret == SPOSE_MID_AIR_REQUEST && out_anim == 2 && final_forced == FORCED_MOVEMENT_MID_AIR_JUMP);
    }

    // ------------------------------------------------------------------------
    // Test 4: Logic — anim_state == 3 (edge case anim_state >= 2 but != 2)
    // ------------------------------------------------------------------------
    {
        reset_test_state();
        g_mock_anim_state = 3u; // edge case >= 2

        uint8_t out_anim = 99;
        uint8_t ret = samus_starting_wall_jump_gfx(TEST_PDATA, &out_anim);
        uint8_t final_forced = mock_read_u8(TEST_PDATA + OFFSET_SAMUS_DATA_FORCED_MOVEMENT);

        run_test("Logic: anim_state==3 (edge case >= 2) -> returns SPOSE_NONE (0xFF), anim=3, forcedMovement untouched (0)",
                 ret == SPOSE_NONE && out_anim == 3 && final_forced == 0);
    }

    // ------------------------------------------------------------------------
    // Test 5: Defensive Null Pointer Check
    // ------------------------------------------------------------------------
    {
        reset_test_state();
        uint8_t out_anim = 99;
        uint8_t ret = samus_starting_wall_jump_gfx(0u, &out_anim);

        run_test("Null Safety: pData==0 -> returns SPOSE_NONE (0xFF), anim=0, no animation call",
                 ret == SPOSE_NONE && out_anim == SAMUS_ANIM_STATE_NONE && g_anim_call_count == 0);
    }

    // ------------------------------------------------------------------------
    // Test 6: Single Call & Argument Guarantee
    // ------------------------------------------------------------------------
    {
        reset_test_state();
        g_mock_anim_state = SAMUS_ANIM_STATE_ENDED;
        samus_starting_wall_jump_gfx(TEST_PDATA, nullptr);

        run_test("Single Call Guarantee: SamusUpdateAnimation called exactly once with slowed=0 and correct pData",
                 g_anim_call_count == 1 && g_last_anim_slowed == 0u && g_last_anim_pdata == TEST_PDATA);
    }

    // ------------------------------------------------------------------------
    // Test 7: Memory Boundary Protection (no adjacent byte corruption)
    // ------------------------------------------------------------------------
    {
        reset_test_state();
        g_mock_anim_state = SAMUS_ANIM_STATE_ENDED;
        samus_starting_wall_jump_gfx(TEST_PDATA, nullptr);

        uint8_t pre_byte = mock_read_u8(TEST_PDATA + 0x03);
        uint8_t target_byte = mock_read_u8(TEST_PDATA + OFFSET_SAMUS_DATA_FORCED_MOVEMENT);
        uint8_t post_byte = mock_read_u8(TEST_PDATA + 0x05);

        run_test("Memory Boundary: writes strictly pData+0x04=1; pData+0x03 and pData+0x05 unaltered",
                 pre_byte == 0xAA && target_byte == 1 && post_byte == 0xBB);
    }

    // ------------------------------------------------------------------------
    // Test 8: Thunk ABI — anim_state == 0 (in progress: C=0, Z=0, N=0, V=0)
    // ------------------------------------------------------------------------
    {
        reset_test_state();
        g_mock_anim_state = 0u;

        ArmCpuState cpu{};
        cpu.R[0] = TEST_PDATA;
        cpu.R[1] = 0xAAAAAAAAu;
        cpu.R[4] = 0xCAFEBABE;
        cpu.R[14] = 0x08001235u; // Thumb return address (odd)
        cpu.cpsr = CPSR_N_BIT | CPSR_Z_BIT | CPSR_C_BIT | CPSR_V_BIT; // pre-pollute all flags

        thunk_samus_starting_wall_jump_gfx_on_state(&cpu);

        bool r0_ok = (cpu.R[0] == SPOSE_NONE); // 255
        bool r1_ok = (cpu.R[1] == 0x08001235u); // popped LR into R1
        bool r4_ok = (cpu.R[4] == 0xCAFEBABE);  // R4 preserved
        bool pc_ok = (cpu.R[15] == 0x08001234u); // LR & ~1
        bool t_ok  = (cpu.cpsr & CPSR_T_BIT) != 0; // Thumb mode preserved
        bool c_ok  = (cpu.cpsr & CPSR_C_BIT) == 0; // C = 0 because 0 < 2
        bool nz_ok = (cpu.cpsr & (CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT)) == 0; // N=0, Z=0, V=0
        bool ret_pc_ok = (g_last_runtime_call_return_pc == 0x08001234u);

        run_test("Thunk ABI: anim==0 -> R0=0xFF, R1=LR, R4 intact, PC=LR&~1, C=0, N=0, Z=0, V=0",
                 r0_ok && r1_ok && r4_ok && pc_ok && t_ok && c_ok && nz_ok && ret_pc_ok);
    }

    // ------------------------------------------------------------------------
    // Test 9: Thunk ABI — anim_state == 1 (sub-ended: C=0, Z=0, N=0, V=0)
    // ------------------------------------------------------------------------
    {
        reset_test_state();
        g_mock_anim_state = 1u;

        ArmCpuState cpu{};
        cpu.R[0] = TEST_PDATA;
        cpu.R[1] = 0xBBBBBBBBu;
        cpu.R[4] = 0xFEEDFACE;
        cpu.R[14] = 0x08004567u; // Thumb return address
        cpu.cpsr = CPSR_N_BIT | CPSR_Z_BIT | CPSR_C_BIT | CPSR_V_BIT;

        thunk_samus_starting_wall_jump_gfx_on_state(&cpu);

        bool r0_ok = (cpu.R[0] == SPOSE_NONE); // 255
        bool r1_ok = (cpu.R[1] == 0x08004567u);
        bool r4_ok = (cpu.R[4] == 0xFEEDFACE);
        bool pc_ok = (cpu.R[15] == 0x08004566u);
        bool t_ok  = (cpu.cpsr & CPSR_T_BIT) != 0;
        bool c_ok  = (cpu.cpsr & CPSR_C_BIT) == 0; // C = 0 because 1 < 2
        bool nz_ok = (cpu.cpsr & (CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT)) == 0;

        run_test("Thunk ABI: anim==1 -> R0=0xFF, R1=LR, R4 intact, PC=LR&~1, C=0, N=0, Z=0, V=0",
                 r0_ok && r1_ok && r4_ok && pc_ok && t_ok && c_ok && nz_ok);
    }

    // ------------------------------------------------------------------------
    // Test 10: Thunk ABI — anim_state == 2 (ended: C=1, Z=0, N=0, V=0)
    // ------------------------------------------------------------------------
    {
        reset_test_state();
        g_mock_anim_state = 2u;

        ArmCpuState cpu{};
        cpu.R[0] = TEST_PDATA;
        cpu.R[1] = 0xCCCCCCCCu;
        cpu.R[4] = 0xDEADBEEF;
        cpu.R[14] = 0x08009875u;
        cpu.cpsr = CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT; // C initially 0

        thunk_samus_starting_wall_jump_gfx_on_state(&cpu);

        bool r0_ok = (cpu.R[0] == SPOSE_MID_AIR_REQUEST); // 254
        bool r1_ok = (cpu.R[1] == 0x08009875u);
        bool r4_ok = (cpu.R[4] == 0xDEADBEEF);
        bool pc_ok = (cpu.R[15] == 0x08009874u);
        bool t_ok  = (cpu.cpsr & CPSR_T_BIT) != 0;
        bool c_ok  = (cpu.cpsr & CPSR_C_BIT) != 0; // C = 1 because 2 >= 2
        bool nz_ok = (cpu.cpsr & (CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT)) == 0; // N=0, Z=0, V=0

        run_test("Thunk ABI: anim==2 -> R0=0xFE, R1=LR, R4 intact, PC=LR&~1, C=1, N=0, Z=0, V=0",
                 r0_ok && r1_ok && r4_ok && pc_ok && t_ok && c_ok && nz_ok);
    }

    // ------------------------------------------------------------------------
    // Test 11: Thunk ABI — anim_state == 3 (edge case >= 2: C=1, Z=0, N=0, V=0)
    // ------------------------------------------------------------------------
    {
        reset_test_state();
        g_mock_anim_state = 3u;

        ArmCpuState cpu{};
        cpu.R[0] = TEST_PDATA;
        cpu.R[1] = 0xDDDDDDDDu;
        cpu.R[4] = 0x12345678;
        cpu.R[14] = 0x08003333u;
        cpu.cpsr = CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT; // C initially 0

        thunk_samus_starting_wall_jump_gfx_on_state(&cpu);

        bool r0_ok = (cpu.R[0] == SPOSE_NONE); // 255 (not 2)
        bool r1_ok = (cpu.R[1] == 0x08003333u);
        bool r4_ok = (cpu.R[4] == 0x12345678);
        bool pc_ok = (cpu.R[15] == 0x08003332u);
        bool t_ok  = (cpu.cpsr & CPSR_T_BIT) != 0;
        bool c_ok  = (cpu.cpsr & CPSR_C_BIT) != 0; // C = 1 because 3 >= 2
        bool nz_ok = (cpu.cpsr & (CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT)) == 0; // N=0, Z=0, V=0

        run_test("Thunk ABI: anim==3 (edge case >= 2) -> R0=0xFF, R1=LR, R4 intact, PC=LR&~1, C=1, N=0, Z=0, V=0",
                 r0_ok && r1_ok && r4_ok && pc_ok && t_ok && c_ok && nz_ok);
    }

    // ------------------------------------------------------------------------
    // Test 12: Thunk ABI — ARM Mode Return (LR with bit 0 clear)
    // ------------------------------------------------------------------------
    {
        reset_test_state();
        g_mock_anim_state = 2u;

        ArmCpuState cpu{};
        cpu.R[0] = TEST_PDATA;
        cpu.R[14] = 0x08002000u; // Bit 0 is 0 (ARM mode return address)
        cpu.cpsr = CPSR_T_BIT; // Start in Thumb mode

        thunk_samus_starting_wall_jump_gfx_on_state(&cpu);

        bool pc_ok = (cpu.R[15] == 0x08002000u);
        bool t_cleared = (cpu.cpsr & CPSR_T_BIT) == 0; // T-bit cleared for ARM mode

        run_test("Thunk ABI: ARM Mode Return -> clears CPSR T-bit when LR bit 0 is 0",
                 pc_ok && t_cleared);
    }

    // ------------------------------------------------------------------------
    // Summary
    // ------------------------------------------------------------------------
    std::printf("\n------------------------------------------------------------\n");
    std::printf("Result: %d / %d tests passed (%.1f%%)\n", pass_count, total_tests,
                (static_cast<float>(pass_count) / static_cast<float>(total_tests)) * 100.0f);
    std::printf("============================================================\n");

    return (pass_count == total_tests) ? 0 : 1;
}
