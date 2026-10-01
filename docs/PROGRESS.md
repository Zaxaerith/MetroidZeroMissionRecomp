# Experimental Preview — Final cleanup

Checkpoint022 / 2026-10-01. Minimum gameplay acceptance is complete. Development,
strict-static/coverage research and broad gameplay testing have stopped. This
checkpoint only cleans the repository and verifies reproducibility. No new
features, production integration changes or non-blocking bug fixes were made.

## Cold verification

Deleted generated/build/runtime cache before setup. Verified pinned dependency
sources, fetched pinned toml++ afresh, regenerated cartridge/BIOS and seven stack
helper bodies, rebuilt SDL/framework/host from source. Existing source checkouts
were reused; no prior objects, generated C++ or executable were reused.
Toolchain: Windows x64, MinGW-w64 GCC16.1.0, CMake4.4.3, Ninja1.13.2, Python3.14.3.
Clean host build PASS; EXE31937479 bytes, SHA25624e5d5abd04ccd8b76aee32ecf5a4caf8f40682677addc94c4a4f4e57624c106.
This is functional cold-build reproducibility, not a bit-identical binary claim.

Only three existing qualified routes ran: rooms8100, save-station25500 and
fresh-process reload3200 completed VBlank steps. All19 RGB endpoints and three
complete PCM segments match native manifests. Real in-game station save was
flushed on clean exit; only SRAM crossed into the new process. Saved room and
equipment restoration PASS. Source32KiB fixture remained unchanged. No savestate,
healing, strict sweep, oracle, campaign or additional gameplay validation.

| Metric | Result / limit |
| --- | --- |
| Build / Boot / Title / New Game / Gameplay | PASS |
| Room transition | PASS representative route |
| Combat | shots/enemy contact covered; defeat/bosses untested |
| Audio | complete PCM PASS; physical listening deferred |
| Save-load / Smoke | PASS cold build, real station write, exit/restart/load |
| AOT | 20035 generated functions+7 guarded stack bodies; executed count unavailable |
| healed / interpreted | 0 / 2456739 on canonical rooms8100 route |
| dispatch misses | 19 distinct PCs / 9096 bridge invocations; NOT_STATIC |
| strict first miss | historical020 rooms/save08009FF0 Thumb; not rerun/not a release gate |

## Final source boundary

Generated/build/cache and all dependency checkouts deleted again after PASS.
Pinned setup/patch/regen/build scripts recreate them. MZM decomp download is
optional via setup-deps.ps1 -IncludeDecomp; it is not a host build dependency.
Both user ROM copies preserved; the decomp copy moved to ignored
roms/decomp-mzm_us_baserom.gba. Source SRAM remains in ignored saves/. BIOS is
external and untouched. Old baseline, research archives, unused references,
old demo/gate/debug routes, screenshots, WAV/state captures and duplicate logs
removed. Production live-image/IRQ/stack guards and required metadata remain.
Small root runtime coverage/miss audit reports remain ignored locally.

Logs retain only latest cold host build, compact cold report and three current
smoke result JSONs. Trace path fields in historical metadata identify retired
provenance; they are not build inputs. Public files contain hand-written source,
configuration, pinned patches, final smoke routes, metadata, licenses and docs.
No ROM/BIOS/save/generated C++/binary/cache/log material is tracked.

[Rebuild procedure](REPRODUCIBILITY.md), [size/classification audit](CLEANUP_AUDIT.json)
and [canonical coverage](COVERAGE_CURRENT.json) record this checkpoint.
Local evidence:logs/cold-host-build.log, logs/cold-validation.json and three
logs/*-smoke/result.json. Independent emulator/full-game fidelity remains
unqualified; physical input/listening remain user-deferred.

GitHub source preparation is complete after final Git audit. No remote creation,
upload or push. Existing GitHub credential requires restoration for a future
user-approved upload. Stop here and await the user's confirmation.

## Size and Git audit

Initial directory924216358 bytes (881.40 MiB). Final cleanup approximately17 MiB,
including both8 MiB user ROM copies, ignored acceptance evidence and Git metadata.
Largest remaining directory roms/8 MiB. Before/after classification is recorded
in CLEANUP_AUDIT.json; commit adds only small Git metadata. Public source audit
passed:59 tracked text files; no user assets or generated/build/cache material.
Route and framework patch staged byte hashes match their pinned manifests.

## Canonical ROM cleanup — Checkpoint023

Both root canonical ROM and roms/decomp-mzm_us_baserom.gba were8388608 bytes,
byte-identical: SHA1 5de8536afe1f0078ee6fe1089f890e8c7aa0a6e8;
SHA256 fc94f65380b65b870a30b9b04b39cca1dc63d6e46a4a373d3904adc0912ebc37.
Only the canonical root ROM is retained and ignored. The duplicate was deleted.
Optional with-decomp-rom.ps1 supplies/removes a temporary filename-compatible
copy around a synchronous action; setup itself does not duplicate ROMs.
No game source changes, build, gameplay, coverage or validation runs.
Origin is the user-provided HTTPS repository; no push. Existing GitHub credential
is invalid and must be restored before a future user-approved upload.

## Source publication — Checkpoint024 / 2026-10-01

GitHub CLI connected to the user-authorized Zaxaerith account. Confirmed the
provided repository was empty/public and normally pushed main. Repository:
https://github.com/Zaxaerith/MetroidZeroMissionRecomp . Only tracked source and
metadata uploaded; no ROM/BIOS/save/generated/binary/cache/log assets.
No build, gameplay, coverage or game validation ran during publication.
