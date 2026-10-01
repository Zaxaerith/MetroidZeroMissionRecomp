# Observed RAM executable copies

ROM identity is the USA SHA-1 recorded in ROM_IDENTITY.json. These mappings
come from live runtime bytes and the original cartridge, not from decomp
symbols. The exact-decomp symbol gate remains closed.

Run `python tools/inspect_ram_copies.py` against the local trace and complete
IWRAM dump captured in Checkpoint 003. The script checks the ROM identity,
24 consecutive stack halfword writes, and every byte in the fixed copy spans.
It never merges metadata or outputs cartridge bytes. Evidence is saved locally
in logs/ram-copy-evidence.json.

| Runtime entry | Original ROM | Verified bytes | Observed mode |
|---|---|---:|---|
| 0x03000C7C | 0x08000104 | 512 | ARM |
| 0x03003B90 | 0x08004464 | 1624 | Thumb |
| 0x030041EC | 0x08004310 | 164 | Thumb |
| 0x03004294 | 0x080043B4 | 176 | Thumb |

The first is the IRQ callback reached from the BIOS; the other entries are
frequently executed RAM audio routines. Names describe their observed use,
not imported symbols. game.toml provides the fixed code-copy spans and entries.
The byte-match lengths are bounds verified in the frame-600 dump; longer bytes
are not assumed. Future routes must verify that these images remain invariant.

`static_resume_all=true` uses the existing framework capability to cover
asynchronous returns inside generated functions. It changes static coverage,
not hardware behavior, and must be checked against the previous rendered route.

## Historical Checkpoint 003: overlapping stack images

The first strict miss at 0x03007D38 is a different case. The trace proves
ROM 0x0800529C..0x080052CB was copied to stack 0x03007D38..0x03007D67,
then called through odd Thumb pointer 0x03007D39. Specifically, 0x03007D50
received halfword 0x4281. At frame 600 the same address contains 0xB530,
and 48 bytes from that address match a later placement of the same ROM helper.
This proves overlapping code images at a single guest PC.

An unconditional static binding at 0x03007D38 would make an instruction at
0x03007D50 mean the old helper's interior at one time and the later helper's
entry at another. A transient experimental seed was generated/built but removed
before acceptance testing. No such stack seed remains in the final config.
These helpers retain observable fallback pending code-image-aware dispatch
or another verified solution. Adding more PC seeds cannot solve the collision.

This finding is a limitation of using fixed metadata for mutable RAM. It does
not prove that the existing unconfigured runtime fallback has incorrect
semantics. No shared or project-local framework source was patched.

## Checkpoint 008: guarded gameplay collision image

Native DMA3 observation proves a 320-halfword transfer from ROM 0x08057F7C
to IWRAM 0x030016C4. All 640 live bytes match the verified cartridge image,
SHA-256 2f5f652e653ddd1533980ae266aeb41507e12cb7643366dec99aa6b4fd2bf9ee.
Native Thumb dispatch observed the entry. OBSERVED_RAM_IMAGES.json records
this provenance; reference C suggests collision handling but supplies no
imported addresses or executable source.

A reset-based 8600-step room/pause/resume route samples every16 steps:
539 images and12 DMA-watch events in each of the old/new builds. The sampled
image first matches at2320, is replaced by2624, matches again at3888,
is overwritten by pause at8224 and reinstalled by8432. These are sampled
transition bounds, not exact write times. All observations and DMA events
match between builds. Pause8350/resumed8600 RGB hashes also match.

The actual static mapping is only0x104 bytes, through the observed routine's
POP/BX epilogue (ROM0x0805807A/0x0805807C), including alignment padding.
The entire DMA copy is longer than this function. An experimental mapping
of all640 bytes discovered26 functions and crashed the demo at a downstream
bridge; it was rejected. The final bounded mapping adds only2 functions.
The precise failure mechanism of the rejected wider mapping is not qualified.
Do not infer executable extent from transfer size or bind its neighbors.

src/copied_code.cpp installs the existing runtime force-interpreter predicate.
It compares all640 bytes against the loaded, hash-gated ROM at dispatch and
at every generated instruction, including interior resumes and direct calls.
It reads backing arrays, does not charge guest cycles or alter guest state,
and caches no image identity. Wrong mode, missing/truncated buffers or any
byte mismatch select the existing live interpreter. Strict-static rejection
explicitly invokes the normal strict miss gate, avoiding silent forced fallback.
This predicate is scoped to the bounded collision routine only.

Synthetic ROM-free guard tests reject an uninstalled image, wrong mode,
tail overwrite with identical entry bytes, truncated buffers and a stale
interior resume; reinstall restores acceptance. Neighbor addresses are inert.
Build/run: cmake --build build/host --target copied_code_guard_tests;
ctest --test-dir build/host -R '^copied_code_guard_tests$' --output-on-failure.
Reset room/demo/save/reload routes retain all original RGB/PCM/SRAM hashes.
Strict startup still fails at0x03007D38; stack helpers remain unbound.

Reproduce the diagnostic with tools/inspect_gameplay_ram.py --bios <BIOS>
--route <ordinary input CSV> --out logs/<run> --frames 8600 --interval 16.
Optional --exe selects the previous local build; --rgb-checkpoints 8350,8600
collects pause/resume hashes. Local evidence is logs/cp008-ram-before/after;
no raw ROM/RAM bytes are exported by the report. Sampling does not qualify
full-game lifetime or independent fidelity. No framework source changed.

## Checkpoint 009: two guarded first stack placements

The earlier unbound-stack conclusions above describe Checkpoints 003/008.
The current config maps two first placements, guarded by complete live bytes:

| Runtime entry | ROM source | Image bytes | Proven CPU halfword writes |
| --- | --- | ---: | ---: |
| 0x03007D38 | 0x0800529C | 48 | 24 |
| 0x03007D8C | 0x080051D4 | 36 | 18 |

The first trace proves the complete install and odd Thumb call. After guarding
it, strict stops at 0x03007D8C. Its trace and immediate live IWRAM dump prove
all 18 writes and all 36 bytes. Identical bytes also occur at ROM0x087DCDB0;
the CPU-write provenance identifies 0x080051D4 as the actual copy source.
Hashes and evidence paths are in OBSERVED_RAM_IMAGES.json.

Reproduce the second image proof with:
`python tools/inspect_ram_copies.py --trace logs/cp009-next-miss.log --ram logs/cp009-next-miss.bin --stack-runtime 0x03007D8C --stack-source 0x080051D4 --stack-size 36 --stack-only --verify-stack-image`.
The stack-only option excludes audio copies that have not yet been installed
at this early strict failure. Default options still validate the earlier trace.

The existing force-interpreter hook now checks all three qualified ranges.
It preserves guest RAM PCs and runtime timing; it never redirects PC to ROM.
Overlapping later installs at 0x03007D50/0x03007D90 remain live fallback.
Synthetic overwrite/reinstall tests verify stale root and interior rejection.
Strict now aborts at 0x03007D90 Thumb, preceded by live-image rejection at that
address: the old interior alias cannot run against the replacement image.
This is NOT a strict-static pass or a general multi-image dispatch solution.

All four unchanged native regressions PASS. Each reports two rejected image
predicate checks separately from ordinary dispatch misses. These checks are
not guest instructions or native call counts. The bridge instruction counter
does not include every forced single-step interpreter path. No framework or
generated source was edited. Evidence: logs/cp009-* and save smoke logs.

## Checkpoint 010: replacement image and strict negative control

A reset trace proves another18 halfword CPU writes from ROM0x080051D4 to
RAM0x03007D90. All36 live bytes match the same image as the first placement at
0x03007D8C. The old36-byte image at0x03007D8C no longer matches. The observed
odd pointer0x03007D91 enters Thumb mode; its call requests the complete32KiB
SRAM read, compared to the earlier16-byte read. Copy identity alone does not
make the old alias executable: the generated table still maps0x03007D90 to
an interior resume of gf_tfunc_03007D8C, offset4, rather than the new root.

`python tools/probe_stack_overlap.py --bios <BIOS>` repeats this qualification
from reset with isolated, hash-checked initial SRAM, LLE, healing disabled,
no state injection and a60-second timeout. It validates the generated alias,
all18 source/destination/value writes, complete live image, old-image mismatch,
actual Thumb exchange, first guard rejection and Windows strict abort exit.
Inherited GBARECOMP settings are cleared; output is restricted to project logs.
Stale dumps are removed before launch, so failed launches cannot reuse evidence.

Two fixture-based runs produce the same qualified overlap and EXE identity.
The script returns success only for `guard_status=PASS`; `strict_static=NOT_PASS`
is separate and explicit. An ordinary exit, another failure or changed alias
requires review instead of silently becoming a successful strict test.
Six ROM-free verifier tests include missing/reordered writes, matching bytes
with wrong source provenance, tail mutation/truncation, wrong mode and missing
rejection/strict evidence. Run:
`python -m unittest discover -s tests -p test_stack_overlap_evidence.py`.

Replacement metadata is in OBSERVED_RAM_IMAGES.json with static_code_bytes=0.
No executable config, framework source or generated code changed. A future
solution must distinguish live image plus placement when selecting a body
and an interior resume, and must retain guest RAM PC/fetch timing. Adding a
source_addr seed alone does not qualify runtime selection at overlapping PCs.
Evidence: logs/cp010-stack-reset and logs/cp010-stack-reset-repeat.

## Checkpoint 011: qualified later comparison image; prototype rejected

A temporary multi-image hook passed03007D90, then stopped at03007D50.
All24 halfword writes, full48-byte live image and odd Thumb call match
ROM0800529C. Metadata is observation-only: static_code_bytes=0. That prototype
failed the unchanged rooms RGB/PCM gates and was withdrawn. Its full equal32KiB
comparison also overflows the native host stack through recursive forward
branch callbacks. See STACK_DISPATCH_EXPERIMENT.md for limits and reproduction.
The accepted EXE again aborts at03007D90; no static coverage gain is claimed.


## Checkpoint 013

Six observed placements are now qualified in the default host:compare image
ROM0800529C at03007D38/03007D50/03007D18/03007D14;read imageROM080051D4 at
03007D8C/03007D90. New reset probes qualify24 halfword writes/48 live bytes
and actual odd Thumb calls at7D50/7D18/7D14. Image hashes remain27fc0ae4... and
1818db03.... Full epilogue/padding verification and pinned emitter generation
produce46/34 executable bytes respectively, retaining real RAM PCs.

Whole installed image identity and mode authorize every instruction/resume.
Active image identity is retained across tails and IRQ contexts, so another
matching placement cannot authorize a stale active body. Same-image branches
request iterative continuation instead of recursively entering the host body.
No unconditional mutable PC-to-ROM table binding remains for these stack
ranges; unknown placements/images still fall back and strict mismatch aborts.
No helper bytes or generated code were edited/distributed.

Current tool:tools/probe_stack_candidate.py (see STACK_DISPATCH_EXPERIMENT.md).
It separates copy evidence from strict success and intentionally rejects a
changed first miss. Historical fixed-alias negative control probe_stack_overlap.py
is superseded for the new table; its parser tests remain useful.
Default strict600 now first misses ROM08000A98. Runtime03007D08 is still
unqualified ordinary fallback. All4 native routes and explicit SDL window/audio
checks PASS; mGBA timing/fidelity remains DIVERGED. Evidence:logs/cp013-* and
OBSERVED_RAM_IMAGES.json. Broader stack lifetime remains unqualified.


## Checkpoint 017: seventh qualified placement03007D08

The6000-frame strict reset with newly native PATT first exchanges03007D09 into
03007D08. Complete48 bytes equalROM0800529C/27fc0ae4...;24 sequential halfword
writes prove source/destination/value provenance. Source fixture unchanged.
Evidence PASS qualifies copy/mode only; that pre-seventh audit was NOT_PASS.
Final host now selects this comparison image by the same whole live-image/
active-body/IRQ guard and iterative continuation as previous placements.
46 executable bytes,2 padding excluded; never an unconditional RAM-PC mapping.

7 variants x counts1/16/32768 x equal/non =42 differential cases PASS, including
all registers/CPSR/full seeded memory/base cycles; max native depth1. New
03007D08/03007D14 overlap/reinstall tests reject stale active-body identity
although another image matches the PC. Existing every-placement guard tests
cover executable interiors, padding, odd PC, wrong mode and last-byte mutation.
All4 native routes unchanged; strict3000 PASS, longer6000 first03001944 NOT_PASS.
Metadata:OBSERVED_RAM_IMAGES.json. Unknown future images remain fallback.

probe_stack_candidate.py now accepts --frames (positive, default600), records
the requested budget and checks source SRAM unchanged. Before installing the
seventh image, the observed provenance run used
`--runtime 0x03007D08 --source 0x0800529C --size 48 --frames 6000`.
It intentionally fails if that candidate is no longer the first strict miss;
archived actual logs/dump/result retain proof. Do not expect this missing-entry
probe to pass against the final seven-body host.


## Checkpoint 018 — independent BG3 haze image

03001944 is adjacent to, but independent of, the640-byte collision image.
512 live bytes matchROM0805D768, SHA256
`a053e7609eacef83fa2d6bdd10afd79506130df4f60a3c5dba9b5d5ee3b89ad6`.
Reset title-idle observation covers283 samples/4500 steps at16-step intervals:
initial unmatched/cleared image, sampled complete match from1744; DMA watcher
records clears plus three exactsrc0805D768/dad03001944/256halfword installs.
These transition steps are sampled, not exact install timings. The original
strict dump matches and exchange03001945/CPSR3F proves actual Thumb execution.

Ordinary code_copy maps184 executable bytes only, through POP/BX030019FA;
literal pool starts019FC. Runtime independently checks all512 live bytes at
entry/every instruction. Synthetic guard tests cover uninstalled image, final
BX/interior, ARM/odd mode, tail mutation, reinstall, missing/truncated buffers
and excluded neighbors. Only BG3 variant qualified; other images retain live
fallback. No generated code hand edit or decomp C/address import. All4 native
RGB/PCM/SRAM/counter gates PASS. Strict first gap moves fromRAM01944 toROMCCF0.

inspect_gameplay_ram.py accepts --runtime/--source/--size while retaining
collision defaults; records expected-source match and restricts output to
project logs. Reproduce with these separate arguments:
`--runtime 0x03001944 --source 0x0805D768 --size 512 --frames 4500 --interval 16`.
Evidence:logs/cp018-haze-provenance-fixed/evidence.json/native.log. Initial
sampling with the old hardcoded collision read range was rejected/corrected;
it does not qualify haze. Metadata:OBSERVED_RAM_IMAGES.json.
