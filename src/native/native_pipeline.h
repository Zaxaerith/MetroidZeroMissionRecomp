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
