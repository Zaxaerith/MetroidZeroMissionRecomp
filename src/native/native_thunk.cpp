// native_thunk.cpp — ABI adapters and GBARecomp mod plugin registrations.
//
// Origin: MetroidZeroMissionRecomp R4 & R5 Native Integration
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include "native_thunk.h"
#include "mod_function_hooks.h"
#include "mod_runtime.h"

#include <atomic>
#include <cstdio>

namespace mzm::native {

namespace {
std::atomic<bool> g_fixed_mult_enabled{false};
std::atomic<uint64_t> g_fixed_mult_hits{0};

std::atomic<bool> g_soft_reset_enabled{false};
std::atomic<uint64_t> g_soft_reset_hits{0};
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
    default:
        break;
    }
}

void set_all_functions_enabled(bool enabled) noexcept {
    set_function_enabled(NativeFunctionId::FixedMultiplication, enabled);
    set_function_enabled(NativeFunctionId::SoftResetCheck, enabled);
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
}

void print_session_summary() noexcept {
    const uint64_t mult_hits = get_function_hits(NativeFunctionId::FixedMultiplication);
    const uint64_t reset_hits = get_function_hits(NativeFunctionId::SoftResetCheck);
    const uint64_t framework_hits = gba_mod_function_hook_hits();

    std::printf("[mzm:native] Native Pipeline Summary:\n");
    std::printf("  - FixedMultiplication (0x08000A4C): %llu hits [%s]\n",
                static_cast<unsigned long long>(mult_hits),
                is_function_enabled(NativeFunctionId::FixedMultiplication) ? "ENABLED" : "DISABLED");
    std::printf("  - SoftResetCheck      (0x080007D0): %llu hits [%s]\n",
                static_cast<unsigned long long>(reset_hits),
                is_function_enabled(NativeFunctionId::SoftResetCheck) ? "ENABLED" : "DISABLED");
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

    // 3. Register global reset callback to re-sync hooks across soft resets
    gba_mod_register_reset_callback(on_mod_sync_callback);
}

} // namespace mzm::native
