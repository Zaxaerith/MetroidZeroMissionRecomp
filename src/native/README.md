# Native C++ Function Replacements (GBARecomp Mod Hooks)

This directory contains human-readable C++ reimplementations of original Game Boy Advance ARM/Thumb functions for *Metroid: Zero Mission (USA, BMXE, Rev 0)*, integrated into the runtime using [GBARecomp](https://github.com/mstan/gbarecomp) mod function hooks.

---

## Architecture & Integration Pipeline

All native functions connect to the recompiled binary via a non-invasive, runtime-switchable hook architecture:

- **`native_pipeline.h`**: Central enumerated registry (`enum class NativeFunctionId`), per-function enablement switches, hit counters, and batch controls.
- **`native_thunk.h / native_thunk.cpp`**: Architecture-exact ABI adapters handling GBA CPU register mapping (`ArmCpuState`), callee-saved register restoration, shadow return stack balancing, and CPSR flags ($N, Z, C, V, T$).
- **`game.toml`**: Static hook registration (`[[mod_function_hook]]`) declaring each function's ROM Thumb address.
- **`src/main.cpp`**: Command-line arguments and environment variable controls for toggling individual or all native functions at startup.

---

## Native Functions Catalog (13 Verified Functions)

| Round | Function Name | ROM Entry | Span | Upstream Decomp Ref | Source Files | Description & Side Effects |
| :---: | :--- | :---: | :---: | :--- | :--- | :--- |
| **R4** | `FixedMultiplication` | `0x08000A4C` | 24B | `fixed_multiplication` | `fixed_multiplication.h/.cpp` | Q8.8 fixed-point signed multiplication with truncation towards zero. |
| **R5** | `SoftResetCheck` | `0x080007D0` | 48B | `CheckSoftReset` | `soft_reset_check.h/.cpp` | Button combo polling (A+B+Start+Select) to trigger soft reset transitions. |
| **R6** | `SamusUpdateAnimation` | `0x0800847C` | 136B | `SamusUpdateAnimation` | `samus_animation.h/.cpp` | Primary animation engine advancing frames, timers, and duration counters based on suit type. |
| **R7** | `SamusTurningAroundGfx` | `0x080088F8` | 76B | `SamusTurningAroundGfx` | `samus_turning.h/.cpp` | Standing turn animation with suit type checks and pose transitions. |
| **R8** | `SamusStandingGfx` | `0x0800881C` | 32B | `SamusStandingGfx` | `samus_standing.h/.cpp` | Idle standing loop; resets frame counter to 0 upon animation completion. |
| **R9** | `SamusShootingGfx` | `0x08008940` | 28B | `SamusShootingGfx` | `samus_shooting.h/.cpp` | Standing shooting recovery; transitions to `SPOSE_STANDING` on ended. |
| **R10** | `SamusShootingAndCrouchingGfx` | `0x08008C50` | 28B | `SamusShootingAndCrouchingGfx` | `samus_shooting_crouching.h/.cpp` | Crouching shot recovery; transitions to `SPOSE_CROUCHING` on ended. |
| **R11** | `SamusTurningAroundAndCrouchingGfx` | `0x08008C24` | 44B | `SamusTurningAroundAndCrouchingGfx` | `samus_turning_crouching.h/.cpp` | Crouching turn animation with Suitless/Power suit pose resolution. |
| **R12** | `SamusStartingSpinJumpGfx` | `0x08008ED8` | 36B | `SamusStartingSpinJumpGfx` | `samus_starting_spin_jump.h/.cpp` | Spin jump takeoff; sets `pose = SPOSE_SPINNING` on animation finish. |
| **R13** | `SamusTurningAroundMidAirGfx` | `0x08008EB8` | 32B | `SamusTurningAroundMidAirGfx` | `samus_turning_midair.h/.cpp` | Mid-air turn; sets `forcedMovement = 2` on animation finish. |
| **R14** | `SamusMorphingGfx` | `0x08009230` | 28B | `SamusMorphingGfx` | `samus_morphing.h/.cpp` | Morph Ball entry; returns `SPOSE_MORPH_BALL` on finish; preserves caller `R4`. |
| **R15** | `SamusUnmorphingGfx` | `0x08009508` | 36B | `SamusUnmorphingGfx` | `samus_unmorphing.h/.cpp` | Morph Ball exit; writes `unmorphPaletteTimer = 15`, returns `SPOSE_CROUCHING`. |
| **R16** | `SamusStartingWallJumpGfx` | `0x080090E0` | 36B | `SamusStartingWallJumpGfx` | `samus_starting_wall_jump.h/.cpp` | Wall jump push-off; sets `forcedMovement = 1`, returns `SPOSE_MID_AIR_REQUEST`. |

---

## Runtime Controls

### Command Line Options

```bash
# Enable all 13 native C++ functions
./MetroidZeroMissionRecomp.exe --native-all

# Disable all native functions (100% original recompiled AOT code)
./MetroidZeroMissionRecomp.exe --no-native-all

# Target specific functions
./MetroidZeroMissionRecomp.exe --native-samus-morphing-gfx --native-samus-unmorphing-gfx
```

### Environment Variables

Each function can also be toggled via environment variables:
- `MZM_NATIVE_ALL=1`
- `MZM_NATIVE_FIXED_MULT=1`
- `MZM_NATIVE_SOFT_RESET=1`
- `MZM_NATIVE_SAMUS_ANIM=1`
- `MZM_NATIVE_SAMUS_TURN=1`
- `MZM_NATIVE_SAMUS_STANDING=1`
- `MZM_NATIVE_SAMUS_SHOOTING=1`
- `MZM_NATIVE_SAMUS_SHOOTING_CROUCHING=1`
- `MZM_NATIVE_SAMUS_TURNING_CROUCHING=1`
- `MZM_NATIVE_SAMUS_STARTING_SPIN_JUMP=1`
- `MZM_NATIVE_SAMUS_TURNING_MIDAIR=1`
- `MZM_NATIVE_SAMUS_MORPHING=1`
- `MZM_NATIVE_SAMUS_UNMORPHING=1`
- `MZM_NATIVE_SAMUS_STARTING_WALL_JUMP=1`

---

## Verification & Testing

Every native function has a standalone test suite under `diagnostics/readable-source-r*/` verifying:
1. Exact ROM Thumb disassembly mapping.
2. Single-call animation update guarantee.
3. CPSR condition flag preservation ($N=0, Z=0, V=0, C=(anim\_state \ge 2)$).
4. Callee-saved register integrity ($R4-R11$).
5. Defensive null pointer checks.
