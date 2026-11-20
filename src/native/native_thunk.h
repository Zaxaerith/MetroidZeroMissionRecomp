// native_thunk.h — ABI adapters connecting GBARecomp CPU state to native functions.
//
// Origin: MetroidZeroMissionRecomp R4 & R5 Native Integration
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>
#include "fixed_multiplication.h"
#include "soft_reset_check.h"
#include "samus_animation.h"
#include "samus_turning.h"
#include "samus_standing.h"
#include "samus_shooting.h"
#include "samus_shooting_crouching.h"
#include "samus_turning_crouching.h"
#include "samus_starting_spin_jump.h"
#include "samus_turning_midair.h"
#include "native_pipeline.h"

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

// --- FixedMultiplication (0x08000A4C, R4) ---
void thunk_fixed_multiplication_on_state(ArmCpuState* cpu) noexcept;
int hook_fixed_multiplication(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept;

void set_native_fixed_mult_enabled(bool enabled) noexcept;
bool is_native_fixed_mult_enabled() noexcept;
uint64_t get_native_fixed_mult_hits() noexcept;
void reset_native_fixed_mult_hits() noexcept;
void activate_native_fixed_mult_hook() noexcept;
void deactivate_native_fixed_mult_hook() noexcept;

// --- SoftResetCheck (0x080007D0, R5) ---
void thunk_soft_reset_check_on_state(ArmCpuState* cpu) noexcept;
int hook_soft_reset_check(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept;

void set_native_soft_reset_enabled(bool enabled) noexcept;
bool is_native_soft_reset_enabled() noexcept;
uint64_t get_native_soft_reset_hits() noexcept;
void reset_native_soft_reset_hits() noexcept;
void activate_native_soft_reset_hook() noexcept;
void deactivate_native_soft_reset_hook() noexcept;

// --- SamusUpdateAnimation (0x0800847C, R6) ---
void thunk_samus_update_animation_on_state(ArmCpuState* cpu) noexcept;
int hook_samus_update_animation(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept;

void set_native_samus_anim_enabled(bool enabled) noexcept;
bool is_native_samus_anim_enabled() noexcept;
uint64_t get_native_samus_anim_hits() noexcept;
void reset_native_samus_anim_hits() noexcept;
void activate_native_samus_anim_hook() noexcept;
void deactivate_native_samus_anim_hook() noexcept;

// --- SamusTurningAroundGfx (0x080088F8, R7) ---
void thunk_samus_turning_around_gfx_on_state(ArmCpuState* cpu) noexcept;
int hook_samus_turning_around_gfx(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept;

void set_native_samus_turn_enabled(bool enabled) noexcept;
bool is_native_samus_turn_enabled() noexcept;
uint64_t get_native_samus_turn_hits() noexcept;
void reset_native_samus_turn_hits() noexcept;
void activate_native_samus_turn_hook() noexcept;
void deactivate_native_samus_turn_hook() noexcept;

// --- SamusStandingGfx (0x0800881C, R8) ---
void thunk_samus_standing_gfx_on_state(ArmCpuState* cpu) noexcept;
int hook_samus_standing_gfx(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept;

void set_native_samus_standing_gfx_enabled(bool enabled) noexcept;
bool is_native_samus_standing_gfx_enabled() noexcept;
uint64_t get_native_samus_standing_gfx_hits() noexcept;
void reset_native_samus_standing_gfx_hits() noexcept;
void activate_native_samus_standing_gfx_hook() noexcept;
void deactivate_native_samus_standing_gfx_hook() noexcept;

// --- SamusShootingGfx (0x08008940, R9) ---
void thunk_samus_shooting_gfx_on_state(ArmCpuState* cpu) noexcept;
int hook_samus_shooting_gfx(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept;

void set_native_samus_shooting_gfx_enabled(bool enabled) noexcept;
bool is_native_samus_shooting_gfx_enabled() noexcept;
uint64_t get_native_samus_shooting_gfx_hits() noexcept;
void reset_native_samus_shooting_gfx_hits() noexcept;
void activate_native_samus_shooting_gfx_hook() noexcept;
void deactivate_native_samus_shooting_gfx_hook() noexcept;

// --- SamusShootingAndCrouchingGfx (0x08008C50, R10) ---
void thunk_samus_shooting_crouching_gfx_on_state(ArmCpuState* cpu) noexcept;
int hook_samus_shooting_crouching_gfx(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept;

void set_native_samus_shooting_crouching_gfx_enabled(bool enabled) noexcept;
bool is_native_samus_shooting_crouching_gfx_enabled() noexcept;
uint64_t get_native_samus_shooting_crouching_gfx_hits() noexcept;
void reset_native_samus_shooting_crouching_gfx_hits() noexcept;
void activate_native_samus_shooting_crouching_gfx_hook() noexcept;
void deactivate_native_samus_shooting_crouching_gfx_hook() noexcept;

// --- SamusTurningAroundAndCrouchingGfx (0x08008C24, R11) ---
void thunk_samus_turning_around_and_crouching_gfx_on_state(ArmCpuState* cpu) noexcept;
int hook_samus_turning_around_and_crouching_gfx(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept;

void set_native_samus_turning_crouching_gfx_enabled(bool enabled) noexcept;
bool is_native_samus_turning_crouching_gfx_enabled() noexcept;
uint64_t get_native_samus_turning_crouching_gfx_hits() noexcept;
void reset_native_samus_turning_crouching_gfx_hits() noexcept;
void activate_native_samus_turning_crouching_gfx_hook() noexcept;
void deactivate_native_samus_turning_crouching_gfx_hook() noexcept;

// --- SamusStartingSpinJumpGfx (0x08008ED8, R12) ---
void thunk_samus_starting_spin_jump_gfx_on_state(ArmCpuState* cpu) noexcept;
int hook_samus_starting_spin_jump_gfx(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept;

void set_native_samus_starting_spin_jump_gfx_enabled(bool enabled) noexcept;
bool is_native_samus_starting_spin_jump_gfx_enabled() noexcept;
uint64_t get_native_samus_starting_spin_jump_gfx_hits() noexcept;
void reset_native_samus_starting_spin_jump_gfx_hits() noexcept;
void activate_native_samus_starting_spin_jump_gfx_hook() noexcept;
void deactivate_native_samus_starting_spin_jump_gfx_hook() noexcept;

// --- SamusTurningAroundMidAirGfx (0x08008EB8, R13) ---
void thunk_samus_turning_around_mid_air_gfx_on_state(ArmCpuState* cpu) noexcept;
int hook_samus_turning_around_mid_air_gfx(uint32_t addr, int thumb, ArmCpuState* cpu) noexcept;

void set_native_samus_turning_midair_gfx_enabled(bool enabled) noexcept;
bool is_native_samus_turning_midair_gfx_enabled() noexcept;
uint64_t get_native_samus_turning_midair_gfx_hits() noexcept;
void reset_native_samus_turning_midair_gfx_hits() noexcept;
void activate_native_samus_turning_midair_gfx_hook() noexcept;
void deactivate_native_samus_turning_midair_gfx_hook() noexcept;

} // namespace mzm::native
