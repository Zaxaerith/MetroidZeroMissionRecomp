// native_pipeline.h — Central registry and management for native function replacements.
//
// Origin: MetroidZeroMissionRecomp R5 Native Pipeline
// License: MIT License (Copyright 2025 YohannDR / MetroidZeroMissionRecomp contributors)

#pragma once

#include <cstdint>

namespace mzm::native {

enum class NativeFunctionId {
    FixedMultiplication, // 0x08000A4C (R4)
    SoftResetCheck,       // 0x080007D0 (R5)
    SamusUpdateAnimation, // 0x0800847C (R6)
    SamusTurningAroundGfx, // 0x080088F8 (R7)
    SamusStandingGfx,      // 0x0800881C (R8)
    SamusShootingGfx,      // 0x08008940 (R9)
    SamusShootingAndCrouchingGfx, // 0x08008C50 (R10)
    SamusTurningAroundAndCrouchingGfx, // 0x08008C24 (R11)
    SamusStartingSpinJumpGfx,          // 0x08008ED8 (R12)
    SamusTurningAroundMidAirGfx,       // 0x08008EB8 (R13)
    SamusMorphingGfx,                  // 0x08009230 (R14)
    SamusUnmorphingGfx,                // 0x08009508 (R14)
    SamusStartingWallJumpGfx,          // 0x080090E0 (R14)
    Count
};

// Per-function enable/disable switch
void set_function_enabled(NativeFunctionId id, bool enabled) noexcept;
bool is_function_enabled(NativeFunctionId id) noexcept;

// Observability hit counters
uint64_t get_function_hits(NativeFunctionId id) noexcept;
void reset_function_hits(NativeFunctionId id) noexcept;

// Batch controls
void set_all_functions_enabled(bool enabled) noexcept;

// Apply active state to framework hooks
void sync_all_hooks() noexcept;

// Session summary reporter
void print_session_summary() noexcept;

} // namespace mzm::native
