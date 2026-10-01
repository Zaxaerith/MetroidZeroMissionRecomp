# Decomp-assisted preview policy

Priority: existing decomp answer → usable native integration → minimal smoke →
cleanup → source publication. Coverage and independent oracle research are
secondary diagnostics, not completion or release gates.

Local source:reference/mzm at43b7fd52f552e4d38c1521ff9d4df5ee57e61493, MIT.
The USA ROM identity matches its documented USA target. The complete decomp ROM
has not been rebuilt/compared, so there is no global symbol/address import.
Per-function ROM verification can qualify a direct C/ASM adapter without making
an unsupported whole-decomp matching claim.

Current assistance includes Samus/sprite/cutscene/color-fading C, callback
arrays, struct/enum knowledge and ASM audio handlers. Example: AudioCommand_Goto
is present in asm/audio_internal.s at its annotated08005030 entry; do not trace
its semantics again. SamusSavingLoadingGame in src/samus.c is the first current
New Game strict miss, but an already playable path need not be rewritten merely
to remove it. Existing generated native/fallback execution remains qualified by
representative gameplay and persistence tests.

For a real defect, read the decomp implementation and call graph first. Direct
reuse must identify region/function source, verify the relevant ROM function and
ABI/layout, use the runtime bus/hardware services, retain MIT attribution and
validate the affected short route. Host pointers cannot simply replace guest
addresses; guest register/return/mode and hardware-visible semantics must remain
correct. Do not hand-edit generated C++ to maintain a copied C implementation.

Scope of acceptance: clean build, boot/title, representative movement/fire/room
change, basic input/audio and save/restart/load. No complete campaign testing,
zero-miss prerequisite, long frame bisection or trajectory research. Physical
controllers/listening are user-deferred; broad compatibility belongs to players.
Old research tools are locally archived under ignored diagnostics/retired-research.
Production ROM/IWRAM identity guards and IRQ-aware stack continuations remain.
