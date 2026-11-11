// native_thunk.cpp — ABI adapter implementation and official Mod Function Hook registration.
//
// Origin: MetroidZeroMissionRecomp R4 Integration
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#include "native_thunk.h"
#include "mod_function_hooks.h"
#include "mod_runtime.h"

#include <atomic>
#include <cstdio>

namespace mzm::native {

namespace {
std::atomic<bool> g_native_enabled{false};
std::atomic<uint64_t> g_native_hits{0};
} // namespace

void set_native_fixed_mult_enabled(bool enabled) noexcept {
    g_native_enabled.store(enabled, std::memory_order_release);
}

bool is_native_fixed_mult_enabled() noexcept {
    return g_native_enabled.load(std::memory_order_acquire);
}

uint64_t get_native_fixed_mult_hits() noexcept {
    return g_native_hits.load(std::memory_order_relaxed);
}

void reset_native_fixed_mult_hits() noexcept {
    g_native_hits.store(0, std::memory_order_relaxed);
}

void thunk_fixed_multiplication_on_state(ArmCpuState* cpu) noexcept {
    if (!cpu) return;

    // 1. Extract 16-bit signed parameters from AAPCS argument registers R0 and R1.
    const int16_t a = static_cast<int16_t>(cpu->R[0] & 0xFFFFu);
    const int16_t b = static_cast<int16_t>(cpu->R[1] & 0xFFFFu);

    // 2. Call the human-readable native C++ function.
    const int16_t result = fixed_multiplication(a, b);

    // 3. Write back the return value into R0 with 32-bit sign extension (AAPCS 5.4).
    const int32_t extended = static_cast<int32_t>(result);
    cpu->R[0] = static_cast<uint32_t>(extended);

    // 4. Scratch R1 to mirror original Thumb pop {r1} behavior.
    cpu->R[1] = cpu->R[14];

    // 5. Update CPSR condition flags (N, Z, C, V):
    //    - cmp r0, #0: subtraction of zero cannot overflow s32 -> sets V=0.
    //    - adds r1, #255: negative product + 255 cannot overflow s32 -> sets V=0.
    //    - lsls and asrs do not modify V.
    //    Therefore V is unconditionally cleared to 0.
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

    // 6. Architectural PC & Thumb mode contract (ARMv4T BX + GBARecomp contract)
    const uint32_t target = cpu->R[14];
    cpu->R[15] = target & ~1u;
    if (target & 1u) {
        cpu->cpsr |= CPSR_T_BIT;
    } else {
        cpu->cpsr &= ~CPSR_T_BIT;
    }
}

int hook_fixed_multiplication(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept {
    // Verified guard: target address must match 0x08000A4C and thumb mode.
    if (addr != 0x08000A4Cu || !thumb || !cpu) {
        return 0; // Decline call, fallback to recompiled AOT
    }

    // If disabled by experimental switch, decline invocation
    if (!is_native_fixed_mult_enabled()) {
        return 0; // Fallback to original recompiled AOT
    }

    thunk_fixed_multiplication_on_state(cpu);
    g_native_hits.fetch_add(1, std::memory_order_relaxed);
    return 1; // Handled: skip original body
}

void activate_native_fixed_mult_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.fixed_mult", 1);
    std::printf("[mzm:native] FixedMultiplication hook ACTIVE (id: mzm.native.fixed_mult)\n");
}

void deactivate_native_fixed_mult_hook() noexcept {
    gba_mod_set_function_hook_enabled("mzm.native.fixed_mult", 0);
    std::printf("[mzm:native] FixedMultiplication hook INACTIVE (id: mzm.native.fixed_mult)\n");
}

static void on_mod_activation_callback(void) {
    if (is_native_fixed_mult_enabled()) {
        activate_native_fixed_mult_hook();
    } else {
        deactivate_native_fixed_mult_hook();
    }
}

// Constructor registration following official GBARecomp mod plugin conventions
GBA_MOD_CONSTRUCTOR(register_mzm_native_fixed_mult) {
    // 1. Register function entry hook (starts disabled by default)
    gba_mod_register_function_entry_plugin(
        "mzm.native.fixed_mult",
        0x08000A4Cu,
        1,
        hook_fixed_multiplication
    );

    // 2. Register activation plugin callback
    gba_mod_register_activation_plugin(
        "mzm.native.fixed_mult",
        on_mod_activation_callback
    );

    // 3. Register reset callback (called during mod_runtime_activate_plugins())
    gba_mod_register_reset_callback(
        on_mod_activation_callback
    );
}

} // namespace mzm::native
