# Frozen miss audit — Checkpoint020

Superseded development strategy: this classification is retained for bug triage,
not a plan to clear coverage. See DECOMP_INTEGRATION.md and PROGRESS.md.

Root `recomp_master_misses_BMXE.toml.frag` is a proposal, never auto-merged.
Its audited input has15 entries from CP019 original8-point gameplay/oracle route;
rooms/demo/save/reload union has61 distinct Thumb ROM PCs. Counts are bridge
invocations, not guest instruction counts or AOT percentages.

Decomp was consulted first: local `reference/mzm/src`. Names, C/ASM bodies,
function boundary comments and callback relationships inform classification.
Exact decomp rebuild is unavailable, so its addresses are not imported; actual
ROM aligned odd pointers and runtime miss modes corroborate callback entries.
No raw ROM bytes, generated guest C++ or full trace is published.

## Master fragment classification

| PC | Calls on fragment route | Class | Decomp answer | Aligned ROM slot |
| --- | ---: | --- | --- | --- |
| 0x080084DC | 45 | indirect callback / function entry | SamusRunning (reference/mzm/src/samus.c:3847) | 0x0875E6A8 |
| 0x080086D4 | 161 | indirect callback / function entry | SamusStanding (reference/mzm/src/samus.c:4019) | 0x0875E6AC, 0x0875E6B4, 0x0875E6D0, 0x0875E794 |
| 0x08008CF8 | 11 | indirect callback / function entry | SamusMidAir (reference/mzm/src/samus.c:4498) | 0x0875E6C8 |
| 0x08009C04 | 2042 | indirect callback / function entry | SamusFacingTheForeground (reference/mzm/src/samus.c:5814) | 0x0875E720 |
| 0x08009FF0 | 219 | indirect callback / function entry | SamusSavingLoadingGame (reference/mzm/src/samus.c:6230) | 0x0875E758, 0x0875E75C |
| 0x080132A8 | 221 | indirect callback / function entry | MorphBallOutside (reference/mzm/src/sprites_ai/morph_ball.c:157) | 0x0875F234 |
| 0x08026B50 | 221 | indirect callback / function entry | MorphBallLauncherPart (reference/mzm/src/sprites_ai/morph_ball_launcher.c:218) | 0x0875F278 |
| 0x08046098 | 660 | indirect callback / function entry | GunshipPart (reference/mzm/src/sprites_ai/gunship.c:1401) | 0x0875F2B0 |
| 0x08050B64 | 13 | indirect callback / function entry | ProjectileProcessNormalBeam (reference/mzm/src/projectile.c:24) | 0x0875F974 |
| 0x0805448C | 4 | indirect callback / function entry | ParticleShootingBeamRight (reference/mzm/src/particle.c:655) | 0x0875F9EC |
| 0x0805BDC8 | 2 | indirect callback / function entry | unk_5bdc8 (reference/mzm/src/color_fading.c:127) | 0x0875FD80 |
| 0x0805CB78 | 5 | indirect callback / function entry | ColorFadingProcess_BeforeIntroText (reference/mzm/src/color_fading.c:985) | 0x08345EEC |
| 0x0805CC94 | 71 | indirect callback / function entry | ColorFadingProcess_BeforeLandingShip (reference/mzm/src/color_fading.c:1079) | 0x08345EFC |
| 0x0805F8F4 | 360 | indirect callback / function entry | InGameCutsceneSamusCloseUp (reference/mzm/src/in_game_cutscene.c:32) | 0x0836030C |
| 0x08062FD8 | 1262 | indirect callback / function entry | StoryTextCutsceneHandler (reference/mzm/src/cutscenes/story_text_cutscene.c:376) | 0x0836BDC0, 0x0836BDD0, 0x0836BDE0 |

## Categories and limits

- Function entry / indirect callback: all current master proposals are Thumb
  ROM callback entries with immutable aligned odd-pointer slots and matching
  decomp semantic functions. Pointer tables select functions; they are not
  automatically switch-case jump tables or interior resumes.
- Interior resume / jump table: no confirmed current master case. Existing
  static_resume_all aliases do not make a missing callback an interior resume.
- ROM-to-IWRAM code copy: none in current15-entry fragment or61-PC route union.
  Existing collision/haze/moving-stack mappings have separate byte/lifetime
  guards; unknown images remain fallback. RAM copies are not declared resolved
  globally merely because this route does not expose them.
- ARM-Thumb: all observed current modes Thumb; no mismatch established.
- Emitter gap: no confirmed example in current fragment. Missing metadata
  roots are not evidence of incorrect instruction emission.
- Genuinely dynamic code: no established example here; mutable RAM alone does
  not prove dynamic generation (observed images originate from immutable ROM).

## Priority

Blocking an earlier strict gate outranks total route frequency. Before this
batch, reset rooms route passes title1000/file-select1300, first aborts during
New Game launch at completed-step2323:08046098 GunshipPart. Later gates must be
NOT_RUN until this prefix is static.2200 is difficulty selection, not launch.
Only3–5 actual first blocking entries will be seeded this batch; each subsequent
entry requires a fresh prefix gate. No wholesale fragment merge.

High-frequency later candidates: SamusFacingForeground08009C04 (2042 on
original route), StoryTextCutsceneHandler08062FD8 (1262), GunshipPart08046098
(660). Frequency is recorded per route, not compared as interchangeable runs.
The idle reset probe's first0805BDC8 is a different route and does not outrank
an earlier rooms-route blocker.

Full61-PC table/frequencies/source locations are local evidence in
logs/cp020-miss-audit-before.json. This document will record this batch's actual
first-miss chain after generation/build/smoke.

Final cleanup: historical log paths in this frozen audit identify retired evidence;
raw trace files are not retained or required for a reproducible build.
