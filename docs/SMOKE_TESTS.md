# Minimal preview acceptance

Build clean, boot/title, one representative real gameplay route, basic input/
audio, then save/exit/restart/load. Stop there; do not expand into full-game
automated testing. Repeat save-station traversal only for changes that affect
persistence; an already qualified saved fixture can check fresh-process load.

The optional existing manifests record native regression RGB/full PCM/SRAM
hashes for a local empty-slot fixture, SHA256
38e4ce507ef6db5a73a647443fb4b410873c94246b88f28231144f3af4d48a4a.
That32KiB fixture is not distributed. They are native regression gates, not
independent emulator/full-game acceptance. Never normalize a player's save to
fit a manifest. All tests use isolated output saves and verify the input save.

With your local fixture and BIOS:

```powershell
.\tools\test-smoke.ps1 -Bios $biosPath -Route rooms
.\tools\test-save-load.ps1 -Bios $biosPath
```

The rooms route covers title/file select/New Game, movement/fire/jump and two
room changes with enemy contact. Enemy defeat/boss behavior is not qualified.
PCM hashes/signal gates do not substitute for listening. Physical device tests
remain deferred to the user. Scripts retain compact results and no duplicate WAV.
Completed native TCP VBlank steps are not CLI scanline-wrap frame numbers.
Debug savestate acceptance and full instruction traces are prohibited.
