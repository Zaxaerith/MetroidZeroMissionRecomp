// native_thunk.h — ABI adapter connecting GBARecomp CPU state to native FixedMultiplication.
//
// Origin: MetroidZeroMissionRecomp R4 Integration
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>
#include "fixed_multiplication.h"

#if __has_include("runtime_arm_types.h")
#include "runtime_arm_types.h"
#elif __has_include("runtime_arm.h")
#include "runtime_arm.h"
#else
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
#endif

namespace mzm::native {

/**
 * @brief Core ABI adapter operating directly on a given ArmCpuState instance.
 */
void thunk_fixed_multiplication_on_state(ArmCpuState* cpu) noexcept;

/**
 * @brief Mod function hook callback matching GBAModFunctionEntryCallback signature.
 * 
 * Returns 1 if handled (native code executed, original body skipped).
 * Returns 0 if declined / disabled (falls back to recompiled AOT body).
 */
int hook_fixed_multiplication(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept;

/**
 * @brief Explicit experimental feature switch controls.
 */
void set_native_fixed_mult_enabled(bool enabled) noexcept;
bool is_native_fixed_mult_enabled() noexcept;

/**
 * @brief Observability counters.
 */
uint64_t get_native_fixed_mult_hits() noexcept;
void reset_native_fixed_mult_hits() noexcept;

/**
 * @brief Explicit activation / deactivation helpers.
 */
void activate_native_fixed_mult_hook() noexcept;
void deactivate_native_fixed_mult_hook() noexcept;

} // namespace mzm::native
