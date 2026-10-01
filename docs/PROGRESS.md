# Project progress — Experimental Preview

## Current strategy — Checkpoint021 / 2026-10-01

Decomp-first playable native integration → minimal acceptance → cleanup/source
publication. No subjective progress percentage, zero-miss release prerequisite,
long strict sweep, oracle/frame research or whole-game automatic validation.
Current execution is generated native+guarded RAM helpers+explicit fallback.
Decomp C/ASM, boundaries/callback/game-state knowledge assist the integration;
no decomp C/ASM implementation is currently linked. Verified MIT function-level
reuse is allowed for real bugs. Whole-decomp rebuild is not a preview blocker.

## Checkpoint020 — state/log audit and bounded three-entry batch

Read PROGRESS/game.toml/coverage/master proposals. Root input15 misses originated
from prior gameplay/oracle route;4-route union61 Thumb ROM PCs had callback
characteristics. Decomp C references and aligned ROM pointers were audited.
No fragment auto-merge. Exactly3 actual New Game blockers were seeded:
GunshipPart08046098 → BeforeLandingShip0805CC94 → unk_5bdc8 at0805BDC8.
Last rooms/save strict first08009FF0 at completed-step2396; reload080203D0 at1691.
Title/file-select prefix gates passed; later gates NOT_RUN. These are historical
facts, not current coverage-clearing tasks. Idle3000 passed;3500 first080084DC.

Generated20035 ARM94/Thumb19941,151495 aliases/17808 hosts+7 customRAM bodies.
4 original reset routes/23RGB/fullPCM/SRAM/fresh-process continuity/physical-frame
counters passed. Interpreted rooms/demo/save/reload2456739/4423406/7262422/299821;
missedPCs19/40/35/5,healed0. Only bounded10/180 oracle logging check was run;
full historical independent fidelity remains unqualified. No framework changes.
Initial cleanup removed5405 files/1468961374 bytes from logs; current proofs,
latest successful build and compact unresolved evidence retained. Inventory,
plans and results are local logs/cp020-*. Historical records were preserved
under ignored diagnostics/retired-research rather than published as active goals.

## Checkpoint021 — cleanup and minimal preview acceptance

- Cleaned optional CMake research target and game-owned debug printf/counters;
  retained production full-image guards, IRQ context and iterative stack dispatch.
- Retired11 investigation/oracle/strict/experiment scripts and the obsolete parser
  test into ignored diagnostics/retired-research. Active smoke has a small shared
  TCP/PNG/hash module; no oracle/strict dependency or long trace target remains.
- Removed obsolete debug build outputs, binary backups, repeated WAVs and old
  per-access oracle stdout. Kept production generated/build/cache/coverage files.
- Initial clean build exposed a missed reporting helper during cleanup; fixed,
  reran --clean-first,95-step build PASS. Existing framework/generated warnings
  remain; no source-generation implementation or guest code was hand-edited.
- Cleanup-build representative rooms and fresh-process reload PASS:13RGB, full
  PCM, SRAM and13 physical-frame counters match020. Previous020 save-station
  fixture063d3506... crosses into new process; reload6e8a5d69... unchanged.
  No broad game/oracle/strict verification was added after minimal acceptance.
- Reviewed MZM MIT and pinned framework/ARM-core/SDL/toml licenses; full texts and
  framework attribution copied to licenses/. Original source MIT, framework and
  patches retain PolyForm Noncommercial/third-party file-level terms.
- README now Experimental Preview with explicit untested campaign/device/fidelity
  scope, decomp policy and portable pinned setup/build instructions.
- Local Git initialized on main. Public source audit passed (66 files); initial source commit
  excludes ROM/BIOS/save/generated/binaries/logs/local archives.
  GitHub CLI existing credential invalid; no remote repository/push attempted.

### Hard status

| Metric | Result / limit |
| --- | --- |
| Build | clean retry PASS; EXE SHA256 53e4fc735815ed6f13dfed711a2d25002b26f53be63170611b4243de4e021e57 |
| Boot / Title | PASS representative reset smoke |
| New Game / Gameplay | PASS representative route |
| Room transition | PASS two transitions |
| Combat | shots/enemy contact covered; kills/bosses untested |
| Audio | completePCM/native signal PASS; listening deferred |
| Save-load | station/flush020 PASS; new-build fresh-process load021 PASS |
| Smoke test | representative rooms + reload PASS; stop expanding validation |
| AOT | 20035 generated functions+7 guarded stack bodies; executed count unavailable |
| healed / interpreted | 0 / 2456739 on canonical rooms8100 route |
| dispatch misses | 19 distinctPCs / 9096 bridge invocations, NOT_STATIC |
| strict first miss | last020 rooms/save08009FF0 Thumb; not a release gate/not rerun021 |

Canonical coverage/fragment identifies the preview rooms8100 route in
COVERAGE_CURRENT.json. It differs from the original shorter oracle-route input;
15→19 across different routes is not a like-for-like regression. Source fixture
38e4ce50... unchanged. EXE local size 31937479 bytes.
Evidence:logs/cp020-summary.json,cp021-summary.json,cp021-clean-build-repaired.log,
cp021-preview-gameplay/result.json,cp021-preview-reload/result.json.

### Cleanup size audit

Logical file bytes (includes dependencies/build and local Git metadata):

| Scope | Before | After |
| --- | ---: | ---: |
| logs/ | 1499.45 MiB / 5590 files | 8.11 MiB / 353 files |
| Entire project | 2454.33 MiB | 881.39 MiB |

Measured before this final documentation/commit; small metadata additions may
change totals. Detailed inventory and deletion manifests remain local in logs/.
Protected generated/build/recomp_cache/coverage/master files retained.

### Next

Local source Git preparation complete. Restore GitHub authentication before remote
creation/push; no credentials in files. Players test broad compatibility. For a
reproducible bug, use decomp first and the shortest affected route, not a campaign
or coverage research sweep. Manual devices/listening remain user-deferred.
