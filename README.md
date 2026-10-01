# Metroid Zero Mission Recomp — Experimental Preview

A Windows x64 **decomp-assisted GBARecomp native integration** for
Metroid: Zero Mission (USA, BMXE revision0). This preview boots to the title,
starts New Game, supports a representative movement/fire/jump/room-transition
route, and has tested SRAM save/restart/load. It is experimental and has not
been tested through the complete game. Broad compatibility testing is left to
players; keyboard/controller device testing and listening remain user-deferred.

You must supply your own supported ROM and real GBA BIOS. This repository
contains neither game/BIOS assets nor generated ROM-derived C++ or binaries.
Identity hashes and supported region are in [baserom.md](baserom.md).

## Build and play

Requirements: Windows x64, Git, CMake, Ninja, MinGW-w64 GCC, Python3.11+.
Set `$biosPath` to your BIOS file and `$mingwPath` to your MinGW installation.
Place the supported ROM in the project root under the filename recorded in
`game.toml`. Provision pinned source dependencies, then build locally:

```powershell
.\tools\setup-deps.ps1
.\tools\build-host.ps1 -Toolchain $mingwPath -Bios $biosPath -Regenerate
.\build\host\MetroidZeroMissionRecomp.exe --rom '.\Metroid - Zero Mission (USA).gba' --bios $biosPath --window
```

build-host.ps1 builds SDL/framework tools before regenerating cartridge/BIOS
source from the verified files you provide. A checkout intentionally has no
`generated/`, `build/` or runtime cache. Re-run both commands after deleting
`build/`, because pinned toml++ headers are provisioned beneath it. See
[reproducibility and cleanup](docs/REPRODUCIBILITY.md).
The host uses faithful240x160 rendering and the original game's SRAM chip.
Saves default to `saves/metroid_zero_mission_usa.sav`; use `--save-path` for an
isolated save. Back up meaningful player saves before preview testing.

## Decomp-assisted development

For optional local decomp reading, run `tools/setup-deps.ps1 -IncludeDecomp`;
it is not downloaded by the normal build setup.

The pinned [metroidret/mzm](https://github.com/metroidret/mzm) decomp supplies
C/ASM answers, function boundaries, callback relationships, linker/game-state
knowledge. Consult it first for bugs or unknown behavior. Verified decomp C/ASM
may be integrated directly after checking the actual USA function, guest ABI,
bus access and hardware timing. Current execution uses generated native guest
code, seven guarded moving-stack helper bodies and explicit interpreter fallback;
**no decomp C/ASM implementation is currently linked**. Whole-decomp exact ROM
reconstruction and global symbol import have not been verified. These facts do
not prevent this limited preview from being playable.

See [integration policy](docs/DECOMP_INTEGRATION.md),
[progress](docs/PROGRESS.md) and [current coverage](docs/COVERAGE_CURRENT.json).
Coverage is recorded honestly; zero misses/complete strict-static coverage is
not a release requirement. No trajectory sweeps, per-instruction tracing or
whole-game automated validation are part of the preview acceptance workflow.

## Minimal developer acceptance

Clean build → boot/title → representative real gameplay → basic input/audio
check → save/exit/fresh-process load. The existing deterministic regression
scripts use isolated saves and RGB/complete PCM/SRAM hashes, with healing off.
Recorded native manifests require the locally held empty-slot fixture described
in docs/SMOKE_TESTS.md; the fixture is not distributed. Use `--no-wav` to avoid
retaining repeated WAVs. Runtime sampling cannot replace listening or physical
controller tests. Once this small acceptance set passes, stop expanding tests.

Unqualified: Europe/Japan, full campaign/endings, all bosses/enemy defeat,
all save slots/corrupt saves, controller devices and independent emulator
fidelity. Earlier mGBA samples diverged; no full-game faithfulness is claimed.
Report a bug with preview version, ROM hash, reproducible actions and a short
log. Do not upload ROMs, BIOS, saves, generated code or full traces.

## Licensing and publication

Original integration source uses [MIT](LICENSE). GBARecomp and its patches use
PolyForm Noncommercial; other components retain their own licenses. See
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and `licenses/`. Game/BIOS
copyright is unaffected. GitHub preparation is source-only; generated/build/
cache/reference/save/log material is excluded. Local compiled game/BIOS code is
not part of a public source release.
