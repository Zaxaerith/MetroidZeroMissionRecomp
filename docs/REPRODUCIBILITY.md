# Reproducible Experimental Preview

This is a source integration repository, following the GBARecomp pattern:
verified user ROM/BIOS → pinned framework generator → local generated C++ →
native runtime/host. Generated game/BIOS source is never maintained or uploaded.

## Prerequisites and cold build

Windows x64; Git; CMake/Ninja on PATH; MinGW-w64 GCC; Python3.11+.
Supply the USA ROM under the filename in game.toml and the real BIOS matching
baserom.md. Set $mingwPath and $biosPath to your own installations/files.
Do not copy ROM/BIOS/build artifacts from this checkout into a public repository.

```powershell
.\tools\setup-deps.ps1
.\tools\build-host.ps1 -Toolchain $mingwPath -Bios $biosPath -Regenerate
.\build\host\MetroidZeroMissionRecomp.exe --rom '.\Metroid - Zero Mission (USA).gba' --bios $biosPath --window
```

setup-deps.ps1 acquires/checks exact source revisions from FRAMEWORK_PIN.json
and ROM_IDENTITY.json. Existing mismatched or non-Git directories fail rather
than overwrite local work. build-host.ps1 applies hash-pinned framework patches,
builds SDL and the generator, invokes regen.ps1 for cartridge/BIOS, and builds
host plus the required stack helper generation. No old generated/build/cache
files are inputs. There is no dependency on sibling project build outputs.

After deleting build/, always repeat setup-deps.ps1: pinned toml++ source is
provisioned beneath build/framework/_deps. Other checkouts live beneath ignored
reference/. The decomp is a semantic reference, not a linked build dependency. Its optional
checkout is acquired only with `tools/setup-deps.ps1 -IncludeDecomp`.
Network access is required when pinned dependencies are absent. No MSYS shell,
whole-decomp rebuild, symbol export or independent emulator is required.

## Minimal smoke and persistence

The established native hash manifests require a local32KiB empty-slot SRAM
fixture SHA25638e4ce507ef6db5a73a647443fb4b410873c94246b88f28231144f3af4d48a4a
at saves/metroid_zero_mission_usa.sav. It is not shipped; ROM/BIOS alone are enough
to build/play, but not enough to reproduce fixture-specific regression hashes.
Do not overwrite a player's save to satisfy these manifests.

```powershell
.\tools\test-smoke.ps1 -Bios $biosPath
.\tools\test-save-load.ps1 -Bios $biosPath
```

Only the existing rooms8100-step, save-station25500-step and reload3200-step
routes remain. They are the shortest previously qualified routes for the stated
checks; the station traversal is needed to write a real in-game save. Each phase
boots from reset. An isolated, flushed SRAM crosses the process boundary; no
savestate, healing, oracle or strict sweep is used. The source fixture stays
unchanged. RGB/full PCM/SRAM hashes qualify native regression, not full-game or
independent emulator fidelity. Physical device/listening tests are deferred.

## Final repository boundary

Track hand-written src/tools, three smoke routes/manifests, small guard tests,
configuration, verified integration metadata, patches and license/documentation.
Ignore ROM/BIOS/save/reference/generated/build/cache/log/dump material.
After successful cold validation, generated/build/cache and dependency checkouts
can be deleted again. setup-deps/build-host recreate them. Keep only the latest
host build log, a compact cold report and the three current result JSONs locally;
no screenshots/WAV/state dumps. Small ignored runtime coverage/miss audit reports
are retained as local metadata.

## Canonical ROM and optional decomp filename

Only the project-root `Metroid - Zero Mission (USA).gba` is canonical for regen,
build and play. The identical decomp/reference copy was removed. Setup does not
create a second permanent ROM. If an explicitly requested decomp command needs
`mzm_us_baserom.gba`, use:

```powershell
.\tools\with-decomp-rom.ps1 -Action { param($decompRoot)
    # Run the required synchronous decomp command here, using $decompRoot.
}
```

The wrapper checks canonical SHA-1/SHA-256, refuses to overwrite an existing
decomp ROM, temporarily copies it, and removes the copy in finally on completion
or ordinary failure. An abrupt host/process termination may require manual
removal of that temporary copy. It never modifies the canonical ROM.
The wrapper was source-reviewed only; no decomp build or test was run.
