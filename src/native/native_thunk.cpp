// native_thunk.cpp — ABI adapters and GBARecomp mod plugin registrations.
//
// Origin: MetroidZeroMissionRecomp R4 & R5 Native Integration
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include "native_thunk.h"
#include "mod_function_hooks.h"
#include "mod_runtime.h"

#include <atomic>
#include <cstdio>

extern "C" int runtime_call_should_return(uint32_t target_pc);

namespace mzm::native {

namespace {
std::atomic<bool> g_fixed_mult_enabled{false};
std::atomic<uint64_t> g_fixed_mult_hits{0};

std::atomic<bool> g_soft_reset_enabled{false};
std::atomic<uint64_t> g_soft_reset_hits{0};

std::atomic<bool> g_samus_anim_enabled{false};
std::atomic<uint64_t> g_samus_anim_hits{0};

std::atomic<bool> g_samus_turn_enabled{false};
std::atomic<uint64_t> g_samus_turn_hits{0};

std::atomic<bool> g_samus_standing_enabled{false};
std::atomic<uint64_t> g_samus_standing_hits{0};

std::atomic<bool> g_samus_shooting_enabled{false};
std::atomic<uint64_t> g_samus_shooting_hits{0};

std::atomic<bool> g_samus_shooting_crouching_enabled{false};
std::atomic<uint64_t> g_samus_shooting_crouching_hits{0};

std::atomic<bool> g_samus_turning_crouching_enabled{false};
std::atomic<uint64_t> g_samus_turning_crouching_hits{0};

std::atomic<bool> g_samus_starting_spin_jump_enabled{false};
std::atomic<uint64_t> g_samus_starting_spin_jump_hits{0};

std::atomic<bool> g_samus_turning_midair_enabled{false};
std::atomic<uint64_t> g_samus_turning_midair_hits{0};
} // namespace

// ============================================================================
// Pipeline Registry Interface Implementation
// ============================================================================

void set_function_enabled(NativeFunctionId id, bool enabled) noexcept {
    switch (id) {
    case NativeFunctionId::FixedMultiplication:
        g_fixed_mult_enabled.store(enabled, std::memory_order_release);
        break;
    case NativeFunctionId::SoftResetCheck:
        g_soft_reset_enabled.store(enabled, std::memory_order_release);
        break;
    case NativeFunctionId::SamusUpdateAnimation:
        g_samus_anim_enabled.store(enabled, std::memory_order_release);
        break;
    case NativeFunctionId::SamusTurningAroundGfx:
        g_samus_turn_enabled.store(enabled, std::memory_order_release);
        break;
    case NativeFunctionId::SamusStandingGfx:
        g_samus_standing_enabled.store(enabled, std::memory_order_release);
        break;
    case NativeFunctionId::SamusShootingGfx:
        g_samus_shooting_enabled.store(enabled, std::memory_order_release);
        break;
    case NativeFunctionId::SamusShootingAndCrouchingGfx:
        g_samus_shooting_crouching_enabled.store(enabled, std::memory_order_release);
        break;
    case NativeFunctionId::SamusTurningAroundAndCrouchingGfx:
        g_samus_turning_crouching_enabled.store(enabled, std::memory_order_release);
        break;
    case NativeFunctionId::SamusStartingSpinJumpGfx:
        g_samus_starting_spin_jump_enabled.store(enabled, std::memory_order_release);
        break;
    case NativeFunctionId::SamusTurningAroundMidAirGfx:
        g_samus_turning_midair_enabled.store(enabled, std::memory_order_release);
        break;
    default:
        break;
    }
}

bool is_function_enabled(NativeFunctionId id) noexcept {
    switch (id) {
    case NativeFunctionId::FixedMultiplication:
        return g_fixed_mult_enabled.load(std::memory_order_acquire);
    case NativeFunctionId::SoftResetCheck:
        return g_soft_reset_enabled.load(std::memory_order_acquire);
    case NativeFunctionId::SamusUpdateAnimation:
        return g_samus_anim_enabled.load(std::memory_order_acquire);
    case NativeFunctionId::SamusTurningAroundGfx:
        return g_samus_turn_enabled.load(std::memory_order_acquire);
    case NativeFunctionId::SamusStandingGfx:
        return g_samus_standing_enabled.load(std::memory_order_acquire);
    case NativeFunctionId::SamusShootingGfx:
        return g_samus_shooting_enabled.load(std::memory_order_acquire);
    case NativeFunctionId::SamusShootingAndCrouchingGfx:
        return g_samus_shooting_crouching_enabled.load(std::memory_order_acquire);
    case NativeFunctionId::SamusTurningAroundAndCrouchingGfx:
        return g_samus_turning_crouching_enabled.load(std::memory_order_acquire);
    case NativeFunctionId::SamusStartingSpinJumpGfx:
        return g_samus_starting_spin_jump_enabled.load(std::memory_order_acquire);
    case NativeFunctionId::SamusTurningAroundMidAirGfx:
        return g_samus_turning_midair_enabled.load(std::memory_order_acquire);
    default:
        return false;
    }
}

uint64_t get_function_hits(NativeFunctionId id) noexcept {
    switch (id) {
    case NativeFunctionId::FixedMultiplication:
        return g_fixed_mult_hits.load(std::memory_order_relaxed);
    case NativeFunctionId::SoftResetCheck:
        return g_soft_reset_hits.load(std::memory_order_relaxed);
    case NativeFunctionId::SamusUpdateAnimation:
        return g_samus_anim_hits.load(std::memory_order_relaxed);
    case NativeFunctionId::SamusTurningAroundGfx:
        return g_samus_turn_hits.load(std::memory_order_relaxed);
    case NativeFunctionId::SamusStandingGfx:
        return g_samus_standing_hits.load(std::memory_order_relaxed);
    case NativeFunctionId::SamusShootingGfx:
        return g_samus_shooting_hits.load(std::memory_order_relaxed);
    case NativeFunctionId::SamusShootingAndCrouchingGfx:
        return g_samus_shooting_crouching_hits.load(std::memory_order_relaxed);
    case NativeFunctionId::SamusTurningAroundAndCrouchingGfx:
        return g_samus_turning_crouching_hits.load(std::memory_order_relaxed);
    case NativeFunctionId::SamusStartingSpinJumpGfx:
        return g_samus_starting_spin_jump_hits.load(std::memory_order_relaxed);
    case NativeFunctionId::SamusTurningAroundMidAirGfx:
        return g_samus_turning_midair_hits.load(std::memory_order_relaxed);
    default:
        return 0;
    }
}

void reset_function_hits(NativeFunctionId id) noexcept {
    switch (id) {
    case NativeFunctionId::FixedMultiplication:
        g_fixed_mult_hits.store(0, std::memory_order_relaxed);
        break;
    case NativeFunctionId::SoftResetCheck:
        g_soft_reset_hits.store(0, std::memory_order_relaxed);
        break;
    case NativeFunctionId::SamusUpdateAnimation:
        g_samus_anim_hits.store(0, std::memory_order_relaxed);
        break;
    case NativeFunctionId::SamusTurningAroundGfx:
        g_samus_turn_hits.store(0, std::memory_order_relaxed);
        break;
    case NativeFunctionId::SamusStandingGfx:
        g_samus_standing_hits.store(0, std::memory_order_relaxed);
        break;
    case NativeFunctionId::SamusShootingGfx:
        g_samus_shooting_hits.store(0, std::memory_order_relaxed);
        break;
    case NativeFunctionId::SamusShootingAndCrouchingGfx:
        g_samus_shooting_crouching_hits.store(0, std::memory_order_relaxed);
        break;
    case NativeFunctionId::SamusTurningAroundAndCrouchingGfx:
        g_samus_turning_crouching_hits.store(0, std::memory_order_relaxed);
        break;
    case NativeFunctionId::SamusStartingSpinJumpGfx:
        g_samus_starting_spin_jump_hits.store(0, std::memory_order_relaxed);
        break;
    case NativeFunctionId::SamusTurningAroundMidAirGfx:
        g_samus_turning_midair_hits.store(0, std::memory_order_relaxed);
        break;
    default:
        break;
    }
}

void set_all_functions_enabled(bool enabled) noexcept {
    set_function_enabled(NativeFunctionId::FixedMultiplication, enabled);
    set_function_enabled(NativeFunctionId::SoftResetCheck, enabled);
    set_function_enabled(NativeFunctionId::SamusUpdateAnimation, enabled);
    set_function_enabled(NativeFunctionId::SamusTurningAroundGfx, enabled);
    set_function_enabled(NativeFunctionId::SamusStandingGfx, enabled);
    set_function_enabled(NativeFunctionId::SamusShootingGfx, enabled);
    set_function_enabled(NativeFunctionId::SamusShootingAndCrouchingGfx, enabled);
    set_function_enabled(NativeFunctionId::SamusTurningAroundAndCrouchingGfx, enabled);
    set_function_enabled(NativeFunctionId::SamusStartingSpinJumpGfx, enabled);
    set_function_enabled(NativeFunctionId::SamusTurningAroundMidAirGfx, enabled);
}

void sync_all_hooks() noexcept {
    if (is_function_enabled(NativeFunctionId::FixedMultiplication)) {
        activate_native_fixed_mult_hook();
    } else {
        deactivate_native_fixed_mult_hook();
    }

    if (is_function_enabled(NativeFunctionId::SoftResetCheck)) {
        activate_native_soft_reset_hook();
    } else {
        deactivate_native_soft_reset_hook();
    }

    if (is_function_enabled(NativeFunctionId::SamusUpdateAnimation)) {
        activate_native_samus_anim_hook();
    } else {
        deactivate_native_samus_anim_hook();
    }

    if (is_function_enabled(NativeFunctionId::SamusTurningAroundGfx)) {
        activate_native_samus_turn_hook();
    } else {
        deactivate_native_samus_turn_hook();
    }

    if (is_function_enabled(NativeFunctionId::SamusStandingGfx)) {
        activate_native_samus_standing_gfx_hook();
    } else {
        deactivate_native_samus_standing_gfx_hook();
    }

    if (is_function_enabled(NativeFunctionId::SamusShootingGfx)) {
        activate_native_samus_shooting_gfx_hook();
    } else {
        deactivate_native_samus_shooting_gfx_hook();
    }

    if (is_function_enabled(NativeFunctionId::SamusShootingAndCrouchingGfx)) {
        activate_native_samus_shooting_crouching_gfx_hook();
    } else {
        deactivate_native_samus_shooting_crouching_gfx_hook();
    }

    if (is_function_enabled(NativeFunctionId::SamusTurningAroundAndCrouchingGfx)) {
        activate_native_samus_turning_crouching_gfx_hook();
    } else {
        deactivate_native_samus_turning_crouching_gfx_hook();
    }

    if (is_function_enabled(NativeFunctionId::SamusStartingSpinJumpGfx)) {
        activate_native_samus_starting_spin_jump_gfx_hook();
    } else {
        deactivate_native_samus_starting_spin_jump_gfx_hook();
    }

    if (is_function_enabled(NativeFunctionId::SamusTurningAroundMidAirGfx)) {
        activate_native_samus_turning_midair_gfx_hook();
    } else {
        deactivate_native_samus_turning_midair_gfx_hook();
    }
}

void print_session_summary() noexcept {
    const uint64_t mult_hits = get_function_hits(NativeFunctionId::FixedMultiplication);
    const uint64_t reset_hits = get_function_hits(NativeFunctionId::SoftResetCheck);
    const uint64_t anim_hits = get_function_hits(NativeFunctionId::SamusUpdateAnimation);
    const uint64_t turn_hits = get_function_hits(NativeFunctionId::SamusTurningAroundGfx);
    const uint64_t standing_hits = get_function_hits(NativeFunctionId::SamusStandingGfx);
    const uint64_t shooting_hits = get_function_hits(NativeFunctionId::SamusShootingGfx);
    const uint64_t shooting_crouching_hits = get_function_hits(NativeFunctionId::SamusShootingAndCrouchingGfx);
    const uint64_t turning_crouching_hits = get_function_hits(NativeFunctionId::SamusTurningAroundAndCrouchingGfx);
    const uint64_t starting_spin_jump_hits = get_function_hits(NativeFunctionId::SamusStartingSpinJumpGfx);
    const uint64_t turning_midair_hits = get_function_hits(NativeFunctionId::SamusTurningAroundMidAirGfx);
    const uint64_t framework_hits = gba_mod_function_hook_hits();

    std::printf("[mzm:native] Native Pipeline Summary:\n");
    std::printf("  - FixedMultiplication  (0x08000A4C): %llu hits [%s]\n",
                static_cast<unsigned long long>(mult_hits),
                is_function_enabled(NativeFunctionId::FixedMultiplication) ? "ENABLED" : "DISABLED");
    std::printf("  - SoftResetCheck       (0x080007D0): %llu hits [%s]\n",
                static_cast<unsigned long long>(reset_hits),
                is_function_enabled(NativeFunctionId::SoftResetCheck) ? "ENABLED" : "DISABLED");
    std::printf("  - SamusUpdateAnimation (0x0800847C): %llu hits [%s]\n",
                static_cast<unsigned long long>(anim_hits),
                is_function_enabled(NativeFunctionId::SamusUpdateAnimation) ? "ENABLED" : "DISABLED");
    std::printf("  - SamusTurningAroundGfx(0x080088F8): %llu hits [%s]\n",
                static_cast<unsigned long long>(turn_hits),
                is_function_enabled(NativeFunctionId::SamusTurningAroundGfx) ? "ENABLED" : "DISABLED");
    std::printf("  - SamusStandingGfx     (0x0800881C): %llu hits [%s]\n",
                static_cast<unsigned long long>(standing_hits),
                is_function_enabled(NativeFunctionId::SamusStandingGfx) ? "ENABLED" : "DISABLED");
    std::printf("  - SamusShootingGfx     (0x08008940): %llu hits [%s]\n",
                static_cast<unsigned long long>(shooting_hits),
                is_function_enabled(NativeFunctionId::SamusShootingGfx) ? "ENABLED" : "DISABLED");
    std::printf("  - SamusShootingAndCrouchingGfx(0x08008C50): %llu hits [%s]\n",
                static_cast<unsigned long long>(shooting_crouching_hits),
                is_function_enabled(NativeFunctionId::SamusShootingAndCrouchingGfx) ? "ENABLED" : "DISABLED");
    std::printf("  - SamusTurningAroundAndCrouchingGfx(0x08008C24): %llu hits [%s]\n",
                static_cast<unsigned long long>(turning_crouching_hits),
                is_function_enabled(NativeFunctionId::SamusTurningAroundAndCrouchingGfx) ? "ENABLED" : "DISABLED");
    std::printf("  - SamusStartingSpinJumpGfx(0x08008ED8): %llu hits [%s]\n",
                static_cast<unsigned long long>(starting_spin_jump_hits),
                is_function_enabled(NativeFunctionId::SamusStartingSpinJumpGfx) ? "ENABLED" : "DISABLED");
    std::printf("  - SamusTurningAroundMidAirGfx(0x08008EB8): %llu hits [%s]\n",
                static_cast<unsigned long long>(turning_midair_hits),
                is_function_enabled(NativeFunctionId::SamusTurningAroundMidAirGfx) ? "ENABLED" : "DISABLED");
    std::printf("  - Framework hook total calls: %llu\n",
                static_cast<unsigned long long>(framework_hits));
}

// ============================================================================
// FixedMultiplication (0x08000A4C, R4)
// ============================================================================

void set_native_fixed_mult_enabled(bool enabled) noexcept {
    set_function_enabled(NativeFunctionId::FixedMultiplication, enabled);
}

bool is_native_fixed_mult_enabled() noexcept {
    return is_function_enabled(NativeFunctionId::FixedMultiplication);
}

uint64_t get_native_fixed_mult_hits() noexcept {
    return get_function_hits(NativeFunctionId::FixedMultiplication);
}

void reset_native_fixed_mult_hits() noexcept {
    reset_function_hits(NativeFunctionId::FixedMultiplication);
}

void activate_native_fixed_mult_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.fixed_mult", 1);
    std::printf("[mzm:native] Hook ACTIVE: mzm.native.fixed_mult (0x08000A4C)\n");
}

void deactivate_native_fixed_mult_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.fixed_mult", 0);
    std::printf("[mzm:native] Hook INACTIVE: mzm.native.fixed_mult (0x08000A4C)\n");
}

void thunk_fixed_multiplication_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;

    const int16_t a = static_cast<int16_t>(cpu->R[0] & 0xFFFFu);
    const int16_t b = static_cast<int16_t>(cpu->R[1] & 0xFFFFu);

    const int16_t result = fixed_multiplication(a, b);

    const int32_t extended = static_cast<int32_t>(result);
    cpu->R[0] = static_cast<uint32_t>(extended);
    cpu->R[1] = cpu->R[14];

    const int32_t prod = static_cast<int32_t>(a) * static_cast<int32_t>(b);
    const int32_t biased_prod = (prod < 0) ? (prod + 255) : prod;
    const uint32_t r0_before_asr = static_cast<uint32_t>(biased_prod) << 8u;
    const uint32_t carry_bit = (r0_before_asr >> 15u) & 1u;

    cpu->cpsr &= ~(CPSR_N_BIT | CPSR_Z_BIT | CPSR_C_BIT | CPSR_V_BIT);
    if (extended < 0) {
        cpu->cpsr |= CPSR_N_BIT;
    } else if (extended == 0) {
        cpu->cpsr |= CPSR_Z_BIT;
    }
    if (carry_bit) {
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

int hook_fixed_multiplication(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept {
    if (addr != 0x08000A4Cu || !thumb || !cpu) {
        return 0;
    }

    if (!is_native_fixed_mult_enabled()) {
        return 0;
    }

    thunk_fixed_multiplication_on_state(cpu);
    g_fixed_mult_hits.fetch_add(1, std::memory_order_relaxed);
    return 1;
}

// ============================================================================
// SoftResetCheck (0x080007D0, R5)
// ============================================================================

void set_native_soft_reset_enabled(bool enabled) noexcept {
    set_function_enabled(NativeFunctionId::SoftResetCheck, enabled);
}

bool is_native_soft_reset_enabled() noexcept {
    return is_function_enabled(NativeFunctionId::SoftResetCheck);
}

uint64_t get_native_soft_reset_hits() noexcept {
    return get_function_hits(NativeFunctionId::SoftResetCheck);
}

void reset_native_soft_reset_hits() noexcept {
    reset_function_hits(NativeFunctionId::SoftResetCheck);
}

void activate_native_soft_reset_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.soft_reset", 1);
    std::printf("[mzm:native] Hook ACTIVE: mzm.native.soft_reset (0x080007D0)\n");
}

void deactivate_native_soft_reset_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.soft_reset", 0);
    std::printf("[mzm:native] Hook INACTIVE: mzm.native.soft_reset (0x080007D0)\n");
}

void thunk_soft_reset_check_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;

    // 1. Execute readable native C++ logic (uses bus_read / bus_write)
    soft_reset_check();

    // 2. Mirror original Thumb epilogue (pop {r0}; bx r0):
    // Original function pushed LR at entry, popped it into R0 before bx r0.
    const uint32_t target = cpu->R[14];
    cpu->R[0] = target;

    // 3. Architectural PC & Thumb mode return contract
    cpu->R[15] = target & ~1u;
    if (target & 1u) {
        cpu->cpsr |= CPSR_T_BIT;
    } else {
        cpu->cpsr &= ~CPSR_T_BIT;
    }
    runtime_call_should_return(cpu->R[15]);
}

int hook_soft_reset_check(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept {
    if (addr != 0x080007D0u || !thumb || !cpu) {
        return 0;
    }

    if (!is_native_soft_reset_enabled()) {
        return 0;
    }

    thunk_soft_reset_check_on_state(cpu);
    g_soft_reset_hits.fetch_add(1, std::memory_order_relaxed);
    return 1;
}

// ============================================================================
// SamusUpdateAnimation (0x0800847C, R6)
// ============================================================================

void set_native_samus_anim_enabled(bool enabled) noexcept {
    set_function_enabled(NativeFunctionId::SamusUpdateAnimation, enabled);
}

bool is_native_samus_anim_enabled() noexcept {
    return is_function_enabled(NativeFunctionId::SamusUpdateAnimation);
}

uint64_t get_native_samus_anim_hits() noexcept {
    return get_function_hits(NativeFunctionId::SamusUpdateAnimation);
}

void reset_native_samus_anim_hits() noexcept {
    reset_function_hits(NativeFunctionId::SamusUpdateAnimation);
}

void activate_native_samus_anim_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_anim", 1);
    std::printf("[mzm:native] Hook ACTIVE: mzm.native.samus_anim (0x0800847C)\n");
}

void deactivate_native_samus_anim_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_anim", 0);
    std::printf("[mzm:native] Hook INACTIVE: mzm.native.samus_anim (0x0800847C)\n");
}

void thunk_samus_update_animation_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;

    // 1. Inputs: R0 = pData, R1 = slowed (u8)
    const uint32_t p_data = cpu->R[0];
    const uint8_t slowed = static_cast<uint8_t>(cpu->R[1] & 0xFFu);

    // 2. Execute readable native C++ logic
    const uint8_t result = samus_update_animation(p_data, slowed);

    // 3. Return value in R0
    cpu->R[0] = static_cast<uint32_t>(result);

    // 4. Mirror original Thumb epilogue (pop {r4}; pop {r1}; bx r1):
    // In original function, LR was pushed on stack and popped into R1 before bx r1.
    cpu->R[1] = cpu->R[14];

    // 5. Flags: original movs r0, #imm sets N=0, Z=(result == 0)
    cpu->cpsr &= ~(CPSR_N_BIT | CPSR_Z_BIT);
    if (result == 0u) {
        cpu->cpsr |= CPSR_Z_BIT;
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

int hook_samus_update_animation(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept {
    if (addr != 0x0800847Cu || !thumb || !cpu) {
        return 0;
    }

    if (!is_native_samus_anim_enabled()) {
        return 0;
    }

    thunk_samus_update_animation_on_state(cpu);
    g_samus_anim_hits.fetch_add(1, std::memory_order_relaxed);
    return 1;
}

// ============================================================================
// SamusTurningAroundGfx (0x080088F8, R7)
// ============================================================================

void set_native_samus_turn_enabled(bool enabled) noexcept {
    set_function_enabled(NativeFunctionId::SamusTurningAroundGfx, enabled);
}

bool is_native_samus_turn_enabled() noexcept {
    return is_function_enabled(NativeFunctionId::SamusTurningAroundGfx);
}

uint64_t get_native_samus_turn_hits() noexcept {
    return get_function_hits(NativeFunctionId::SamusTurningAroundGfx);
}

void reset_native_samus_turn_hits() noexcept {
    reset_function_hits(NativeFunctionId::SamusTurningAroundGfx);
}

void activate_native_samus_turn_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_turn", 1);
    std::printf("[mzm:native] Hook ACTIVE: mzm.native.samus_turn (0x080088F8)\n");
}

void deactivate_native_samus_turn_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_turn", 0);
    std::printf("[mzm:native] Hook INACTIVE: mzm.native.samus_turn (0x080088F8)\n");
}

void thunk_samus_turning_around_gfx_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;

    // 1. Inputs: R0 = pData (pointer to SamusData in IWRAM)
    const uint32_t p_data = cpu->R[0];

    // 2. Execute readable native C++ logic
    const uint8_t result = samus_turning_around_gfx(p_data);

    // 3. Return value in R0
    cpu->R[0] = static_cast<uint32_t>(result);

    // 4. Mirror original Thumb epilogue (pop {r4}; pop {r1}; bx r1):
    // In original function, LR was pushed on stack and popped into R1 before bx r1.
    cpu->R[1] = cpu->R[14];

    // 5. Flags: original movs r0, #imm sets N=0, Z=(result == 0)
    cpu->cpsr &= ~(CPSR_N_BIT | CPSR_Z_BIT);
    if (result == 0u) {
        cpu->cpsr |= CPSR_Z_BIT;
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

int hook_samus_turning_around_gfx(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept {
    if (addr != 0x080088F8u || !thumb || !cpu) {
        return 0;
    }

    if (!is_native_samus_turn_enabled()) {
        return 0;
    }

    thunk_samus_turning_around_gfx_on_state(cpu);
    g_samus_turn_hits.fetch_add(1, std::memory_order_relaxed);
    return 1;
}

// ============================================================================
// SamusStandingGfx (0x0800881C, R8)
// ============================================================================

void set_native_samus_standing_gfx_enabled(bool enabled) noexcept {
    set_function_enabled(NativeFunctionId::SamusStandingGfx, enabled);
}

bool is_native_samus_standing_gfx_enabled() noexcept {
    return is_function_enabled(NativeFunctionId::SamusStandingGfx);
}

uint64_t get_native_samus_standing_gfx_hits() noexcept {
    return get_function_hits(NativeFunctionId::SamusStandingGfx);
}

void reset_native_samus_standing_gfx_hits() noexcept {
    reset_function_hits(NativeFunctionId::SamusStandingGfx);
}

void activate_native_samus_standing_gfx_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_standing", 1);
    std::printf("[mzm:native] Hook ACTIVE: mzm.native.samus_standing (0x0800881C)\n");
}

void deactivate_native_samus_standing_gfx_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_standing", 0);
    std::printf("[mzm:native] Hook INACTIVE: mzm.native.samus_standing (0x0800881C)\n");
}

void thunk_samus_standing_gfx_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;

    // 1. Inputs: R0 = pData (pointer to SamusData in IWRAM)
    const uint32_t p_data = cpu->R[0];

    // 2. Execute readable native C++ logic
    uint8_t anim_state = 0u;
    const uint8_t result = samus_standing_gfx(p_data, &anim_state);

    // 3. Return value in R0 (SPOSE_NONE = 0xFF = 255)
    cpu->R[0] = static_cast<uint32_t>(result);

    // 4. Mirror original Thumb epilogue (pop {r4}; pop {r1}; bx r1):
    // In original function, LR was pushed on stack and popped into R1 before bx r1.
    cpu->R[1] = cpu->R[14];

    // 5. Flags: cmp r0, #2 sets C=(anim_state >= 2), V=0. movs r0, #255 sets N=0, Z=0.
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

int hook_samus_standing_gfx(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept {
    if (addr != 0x0800881Cu || !thumb || !cpu) {
        return 0;
    }

    if (!is_native_samus_standing_gfx_enabled()) {
        return 0;
    }

    thunk_samus_standing_gfx_on_state(cpu);
    g_samus_standing_hits.fetch_add(1, std::memory_order_relaxed);
    return 1;
}

// ============================================================================
// SamusShootingGfx (0x08008940, R9)
// ============================================================================

void set_native_samus_shooting_gfx_enabled(bool enabled) noexcept {
    set_function_enabled(NativeFunctionId::SamusShootingGfx, enabled);
}

bool is_native_samus_shooting_gfx_enabled() noexcept {
    return is_function_enabled(NativeFunctionId::SamusShootingGfx);
}

uint64_t get_native_samus_shooting_gfx_hits() noexcept {
    return get_function_hits(NativeFunctionId::SamusShootingGfx);
}

void reset_native_samus_shooting_gfx_hits() noexcept {
    reset_function_hits(NativeFunctionId::SamusShootingGfx);
}

void activate_native_samus_shooting_gfx_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_shooting", 1);
    std::printf("[mzm:native] Hook ACTIVE: mzm.native.samus_shooting (0x08008940)\n");
}

void deactivate_native_samus_shooting_gfx_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_shooting", 0);
    std::printf("[mzm:native] Hook INACTIVE: mzm.native.samus_shooting (0x08008940)\n");
}

void thunk_samus_shooting_gfx_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;

    // 1. Inputs: R0 = pData (pointer to SamusData in IWRAM)
    const uint32_t p_data = cpu->R[0];

    // 2. Execute readable native C++ logic
    uint8_t anim_state = 0u;
    const uint8_t result = samus_shooting_gfx(p_data, &anim_state);

    // 3. Return value in R0 (SPOSE_STANDING=1 if ended, SPOSE_NONE=0xFF otherwise)
    cpu->R[0] = static_cast<uint32_t>(result);

    // 4. Mirror original Thumb epilogue (pop {r1}; bx r1):
    // In original function, LR was pushed on stack and popped into R1 before bx r1.
    cpu->R[1] = cpu->R[14];

    // 5. Flags: cmp r0, #2 sets C=(anim_state >= 2), V=0. movs r0, #imm sets N=0, Z=0.
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

int hook_samus_shooting_gfx(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept {
    if (addr != 0x08008940u || !thumb || !cpu) {
        return 0;
    }

    if (!is_native_samus_shooting_gfx_enabled()) {
        return 0;
    }

    thunk_samus_shooting_gfx_on_state(cpu);
    g_samus_shooting_hits.fetch_add(1, std::memory_order_relaxed);
    return 1;
}

// ============================================================================
// SamusShootingAndCrouchingGfx (0x08008C50, R10)
// ============================================================================

void set_native_samus_shooting_crouching_gfx_enabled(bool enabled) noexcept {
    set_function_enabled(NativeFunctionId::SamusShootingAndCrouchingGfx, enabled);
}

bool is_native_samus_shooting_crouching_gfx_enabled() noexcept {
    return is_function_enabled(NativeFunctionId::SamusShootingAndCrouchingGfx);
}

uint64_t get_native_samus_shooting_crouching_gfx_hits() noexcept {
    return get_function_hits(NativeFunctionId::SamusShootingAndCrouchingGfx);
}

void reset_native_samus_shooting_crouching_gfx_hits() noexcept {
    reset_function_hits(NativeFunctionId::SamusShootingAndCrouchingGfx);
}

void activate_native_samus_shooting_crouching_gfx_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_shooting_crouching", 1);
    std::printf("[mzm:native] Hook ACTIVE: mzm.native.samus_shooting_crouching (0x08008C50)\n");
}

void deactivate_native_samus_shooting_crouching_gfx_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_shooting_crouching", 0);
    std::printf("[mzm:native] Hook INACTIVE: mzm.native.samus_shooting_crouching (0x08008C50)\n");
}

void thunk_samus_shooting_crouching_gfx_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;

    // 1. Inputs: R0 = pData (pointer to SamusData in IWRAM)
    const uint32_t p_data = cpu->R[0];

    // 2. Execute readable native C++ logic
    uint8_t anim_state = 0u;
    const uint8_t result = samus_shooting_and_crouching_gfx(p_data, &anim_state);

    // 3. Return value in R0 (SPOSE_CROUCHING=4 if ended, SPOSE_NONE=0xFF otherwise)
    cpu->R[0] = static_cast<uint32_t>(result);

    // 4. Mirror original Thumb epilogue (pop {r1}; bx r1):
    // In original function, LR was pushed on stack and popped into R1 before bx r1.
    cpu->R[1] = cpu->R[14];

    // 5. Flags: cmp r0, #2 sets C=(anim_state >= 2), V=0. movs r0, #imm sets N=0, Z=0.
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

int hook_samus_shooting_crouching_gfx(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept {
    if (addr != 0x08008C50u || !thumb || !cpu) {
        return 0;
    }

    if (!is_native_samus_shooting_crouching_gfx_enabled()) {
        return 0;
    }

    thunk_samus_shooting_crouching_gfx_on_state(cpu);
    g_samus_shooting_crouching_hits.fetch_add(1, std::memory_order_relaxed);
    return 1;
}

// ============================================================================
// SamusTurningAroundAndCrouchingGfx (0x08008C24, R11)
// ============================================================================

void set_native_samus_turning_crouching_gfx_enabled(bool enabled) noexcept {
    set_function_enabled(NativeFunctionId::SamusTurningAroundAndCrouchingGfx, enabled);
}

bool is_native_samus_turning_crouching_gfx_enabled() noexcept {
    return is_function_enabled(NativeFunctionId::SamusTurningAroundAndCrouchingGfx);
}

uint64_t get_native_samus_turning_crouching_gfx_hits() noexcept {
    return get_function_hits(NativeFunctionId::SamusTurningAroundAndCrouchingGfx);
}

void reset_native_samus_turning_crouching_gfx_hits() noexcept {
    reset_function_hits(NativeFunctionId::SamusTurningAroundAndCrouchingGfx);
}

void activate_native_samus_turning_crouching_gfx_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_turn_crouch", 1);
    std::printf("[mzm:native] Hook ACTIVE: mzm.native.samus_turn_crouch (0x08008C24)\n");
}

void deactivate_native_samus_turning_crouching_gfx_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_turn_crouch", 0);
    std::printf("[mzm:native] Hook INACTIVE: mzm.native.samus_turn_crouch (0x08008C24)\n");
}

void thunk_samus_turning_around_and_crouching_gfx_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;

    // 1. Inputs: R0 = pData (pointer to SamusData in IWRAM)
    const uint32_t p_data = cpu->R[0];

    // 2. Execute readable native C++ logic
    uint8_t anim_state = 0u;
    uint8_t suit_type = 0u;
    const uint8_t result = samus_turning_around_and_crouching_gfx(p_data, &anim_state, &suit_type);

    // 3. Return value in R0
    cpu->R[0] = static_cast<uint32_t>(result);

    // 4. Mirror original Thumb epilogue (pop {r1}; bx r1):
    // In original function, LR was pushed on stack and popped into R1 before bx r1.
    cpu->R[1] = cpu->R[14];

    // 5. Flags:
    // If anim_state != 2 (SAMUS_ANIM_STATE_ENDED):
    //   CPSR set by `cmp r0, #2` against anim_state: C = (anim_state >= 2), V = 0.
    // If anim_state == 2:
    //   CPSR set by second `cmp r0, #2` against suitType: C = (suit_type >= 2), V = 0.
    // In all 3 paths, movs r0, #imm sets N = 0, Z = 0 and preserves C and V.
    cpu->cpsr &= ~(CPSR_N_BIT | CPSR_Z_BIT | CPSR_V_BIT | CPSR_C_BIT);
    if (anim_state != SAMUS_ANIM_STATE_ENDED) {
        if (anim_state >= 2u) {
            cpu->cpsr |= CPSR_C_BIT;
        }
    } else {
        if (suit_type >= 2u) {
            cpu->cpsr |= CPSR_C_BIT;
        }
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

int hook_samus_turning_around_and_crouching_gfx(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept {
    if (addr != 0x08008C24u || !thumb || !cpu) {
        return 0;
    }

    if (!is_native_samus_turning_crouching_gfx_enabled()) {
        return 0;
    }

    thunk_samus_turning_around_and_crouching_gfx_on_state(cpu);
    g_samus_turning_crouching_hits.fetch_add(1, std::memory_order_relaxed);
    return 1;
}

// ============================================================================
// SamusStartingSpinJumpGfx (0x08008ED8, R12)
// ============================================================================

void set_native_samus_starting_spin_jump_gfx_enabled(bool enabled) noexcept {
    set_function_enabled(NativeFunctionId::SamusStartingSpinJumpGfx, enabled);
}

bool is_native_samus_starting_spin_jump_gfx_enabled() noexcept {
    return is_function_enabled(NativeFunctionId::SamusStartingSpinJumpGfx);
}

uint64_t get_native_samus_starting_spin_jump_gfx_hits() noexcept {
    return get_function_hits(NativeFunctionId::SamusStartingSpinJumpGfx);
}

void reset_native_samus_starting_spin_jump_gfx_hits() noexcept {
    reset_function_hits(NativeFunctionId::SamusStartingSpinJumpGfx);
}

void activate_native_samus_starting_spin_jump_gfx_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_starting_spin_jump", 1);
    std::printf("[mzm:native] Hook ACTIVE: mzm.native.samus_starting_spin_jump (0x08008ED8)\n");
}

void deactivate_native_samus_starting_spin_jump_gfx_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_starting_spin_jump", 0);
    std::printf("[mzm:native] Hook INACTIVE: mzm.native.samus_starting_spin_jump (0x08008ED8)\n");
}

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
    // Caller's R4 is preserved.
    // In original function, LR was pushed on stack and popped into R1 before bx r1.
    cpu->R[1] = cpu->R[14];

    // 5. Flags:
    // Original Thumb: `cmp r0, #2` where r0 was anim_state.
    // Sets C = (anim_state >= 2), V = 0.
    // In both paths (ended and in-progress), `movs r0, #255` sets N = 0, Z = 0 and preserves C and V.
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

int hook_samus_starting_spin_jump_gfx(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept {
    if (addr != 0x08008ED8u || !thumb || !cpu) {
        return 0;
    }

    if (!is_native_samus_starting_spin_jump_gfx_enabled()) {
        return 0;
    }

    thunk_samus_starting_spin_jump_gfx_on_state(cpu);
    g_samus_starting_spin_jump_hits.fetch_add(1, std::memory_order_relaxed);
    return 1;
}

// ============================================================================
// SamusTurningAroundMidAirGfx (0x08008EB8, R13)
// ============================================================================

void set_native_samus_turning_midair_gfx_enabled(bool enabled) noexcept {
    set_function_enabled(NativeFunctionId::SamusTurningAroundMidAirGfx, enabled);
}

bool is_native_samus_turning_midair_gfx_enabled() noexcept {
    return is_function_enabled(NativeFunctionId::SamusTurningAroundMidAirGfx);
}

uint64_t get_native_samus_turning_midair_gfx_hits() noexcept {
    return get_function_hits(NativeFunctionId::SamusTurningAroundMidAirGfx);
}

void reset_native_samus_turning_midair_gfx_hits() noexcept {
    reset_function_hits(NativeFunctionId::SamusTurningAroundMidAirGfx);
}

void activate_native_samus_turning_midair_gfx_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_turning_midair", 1);
    std::printf("[mzm:native] Hook ACTIVE: mzm.native.samus_turning_midair (0x08008EB8)\n");
}

void deactivate_native_samus_turning_midair_gfx_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.samus_turning_midair", 0);
    std::printf("[mzm:native] Hook INACTIVE: mzm.native.samus_turning_midair (0x08008EB8)\n");
}

void thunk_samus_turning_around_mid_air_gfx_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;

    // 1. Inputs: R0 = pData (pointer to SamusData in IWRAM)
    const uint32_t p_data = cpu->R[0];

    // 2. Execute readable native C++ logic
    uint8_t anim_state = 0u;
    const uint8_t result = samus_turning_around_mid_air_gfx(p_data, &anim_state);

    // 3. Return value in R0 (SPOSE_MID_AIR_REQUEST=0xFE if ended, SPOSE_NONE=0xFF otherwise)
    cpu->R[0] = static_cast<uint32_t>(result);

    // 4. Mirror original Thumb epilogue (pop {r4}; pop {r1}; bx r1):
    // Caller's R4 is preserved.
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

int hook_samus_turning_around_mid_air_gfx(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept {
    if (addr != 0x08008EB8u || !thumb || !cpu) {
        return 0;
    }

    if (!is_native_samus_turning_midair_gfx_enabled()) {
        return 0;
    }

    thunk_samus_turning_around_mid_air_gfx_on_state(cpu);
    g_samus_turning_midair_hits.fetch_add(1, std::memory_order_relaxed);
    return 1;
}

// ============================================================================
// Lifecycle & Plugin Registration
// ============================================================================

static void on_mod_sync_callback(void) {
    sync_all_hooks();
}

GBA_MOD_CONSTRUCTOR(register_mzm_native_hooks) {
    // 1. FixedMultiplication (0x08000A4C, Thumb)
    gba_mod_register_function_entry_plugin(
        "mzm.native.fixed_mult",
        0x08000A4Cu,
        1,
        hook_fixed_multiplication
    );
    gba_mod_register_activation_plugin("mzm.native.fixed_mult", on_mod_sync_callback);

    // 2. SoftResetCheck (0x080007D0, Thumb)
    gba_mod_register_function_entry_plugin(
        "mzm.native.soft_reset",
        0x080007D0u,
        1,
        hook_soft_reset_check
    );
    gba_mod_register_activation_plugin("mzm.native.soft_reset", on_mod_sync_callback);

    // 3. SamusUpdateAnimation (0x0800847C, Thumb)
    gba_mod_register_function_entry_plugin(
        "mzm.native.samus_anim",
        0x0800847Cu,
        1,
        hook_samus_update_animation
    );
    gba_mod_register_activation_plugin("mzm.native.samus_anim", on_mod_sync_callback);

    // 4. SamusTurningAroundGfx (0x080088F8, Thumb)
    gba_mod_register_function_entry_plugin(
        "mzm.native.samus_turn",
        0x080088F8u,
        1,
        hook_samus_turning_around_gfx
    );
    gba_mod_register_activation_plugin("mzm.native.samus_turn", on_mod_sync_callback);

    // 5. SamusStandingGfx (0x0800881C, Thumb)
    gba_mod_register_function_entry_plugin(
        "mzm.native.samus_standing",
        0x0800881Cu,
        1,
        hook_samus_standing_gfx
    );
    gba_mod_register_activation_plugin("mzm.native.samus_standing", on_mod_sync_callback);

    // 6. SamusShootingGfx (0x08008940, Thumb)
    gba_mod_register_function_entry_plugin(
        "mzm.native.samus_shooting",
        0x08008940u,
        1,
        hook_samus_shooting_gfx
    );
    gba_mod_register_activation_plugin("mzm.native.samus_shooting", on_mod_sync_callback);

    // 7. SamusShootingAndCrouchingGfx (0x08008C50, Thumb)
    gba_mod_register_function_entry_plugin(
        "mzm.native.samus_shooting_crouching",
        0x08008C50u,
        1,
        hook_samus_shooting_crouching_gfx
    );
    gba_mod_register_activation_plugin("mzm.native.samus_shooting_crouching", on_mod_sync_callback);

    // 8. SamusTurningAroundAndCrouchingGfx (0x08008C24, Thumb)
    gba_mod_register_function_entry_plugin(
        "mzm.native.samus_turn_crouch",
        0x08008C24u,
        1,
        hook_samus_turning_around_and_crouching_gfx
    );
    gba_mod_register_activation_plugin("mzm.native.samus_turn_crouch", on_mod_sync_callback);

    // 9. SamusStartingSpinJumpGfx (0x08008ED8, Thumb)
    gba_mod_register_function_entry_plugin(
        "mzm.native.samus_starting_spin_jump",
        0x08008ED8u,
        1,
        hook_samus_starting_spin_jump_gfx
    );
    gba_mod_register_activation_plugin("mzm.native.samus_starting_spin_jump", on_mod_sync_callback);

    // 10. SamusTurningAroundMidAirGfx (0x08008EB8, Thumb)
    gba_mod_register_function_entry_plugin(
        "mzm.native.samus_turning_midair",
        0x08008EB8u,
        1,
        hook_samus_turning_around_mid_air_gfx
    );
    gba_mod_register_activation_plugin("mzm.native.samus_turning_midair", on_mod_sync_callback);

    // 11. Register global reset callback to re-sync hooks across soft resets
    gba_mod_register_reset_callback(on_mod_sync_callback);
}

} // namespace mzm::native
