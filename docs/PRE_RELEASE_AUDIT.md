# Metroid: Zero Mission Recomp — Pre-Release Git, Reproducibility, License & Packaging Audit Report

**Audit Date:** October 2026  
**Audited Branch:** `feature/player-launcher-preview`  
**Target Release:** `v0.1.0-preview`  
**Project:** *Metroid: Zero Mission Recomp*  
**Repository:** `D:\Project\GameRecomp\MetroidZeroMissionRecomp` (`https://github.com/Zaxaerith/MetroidZeroMissionRecomp`)  

---

## 1. Executive Summary

A comprehensive pre-release audit was performed covering Git repository safety, history linear alignment, clean fast-forward merge capability, dependency version locking, framework patch integrity, component license compliance, compiled guest code copyright boundaries, and standalone package verification.

### Audit Result
```
╔══════════════════════════════════════════════════════════════════════════════╗
║                       TECHNICAL RELEASE AUDIT: PASS                          ║
║                                                                              ║
║  Notice: This determination certifies technical integrity, build              ║
║  reproducibility, and license attribution compliance. It explicitly DOES     ║
║  NOT constitute a legal warranty ("LEGAL RELEASE AUTHORIZED") regarding      ║
║  third-party IP rights. GBARecomp imposes strict PolyForm Noncommercial 1.0.0║
║  terms prohibiting commercial use.                                           ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

---

## 2. Phase 1: Git Security & Repository History Audit

| Check Item | Verified Value / Status | Assessment |
|---|---|---|
| **Active Branch** | `feature/player-launcher-preview` | **PASS**: Dedicated isolated feature branch active. |
| **Remote Tracking Base** | `origin/main` (`6c9973ef6c173c2f2fa042b625c6e7c6ed3bad19`) | **PASS**: Upstream main cleanly fetched and pinned. |
| **Merge Base** | `6c9973ef6c173c2f2fa042b625c6e7c6ed3bad19` | **PASS**: Exact match with `origin/main`. Zero divergence. |
| **Fast-Forward Merge** | Linear 1-to-2 commits ahead of remote main | **PASS**: Merge to `main` can be executed via 100% fast-forward without merge commits or `--force`. |
| **Remote Tag Collision** | `git ls-remote --tags origin` returned empty | **PASS**: Zero collisions on tag name `v0.1.0-preview`. |
| **Tracked File Sanitation** | Full `git ls-files` enumeration audited | **PASS**: Zero ROMs (`*.gba`), zero BIOS dumps (`*.bin`), zero saves (`*.sav`), zero compiler artifacts (`build/`, `*.exe`, `*.dll`), zero generated translations (`generated/`). |
| **Leakage & Absolute Paths** | Grepped tracked tree for absolute paths and keys | **PASS**: No local developer paths (`C:\Users\...`, `D:\Project\...`) or private credentials tracked. |
| **Git Submodule Status** | Managed via `FRAMEWORK_PIN.json` & `setup-deps.ps1` | **PASS**: Independent directory checkouts in `.gitignore`; no dangling submodules. |

---

## 3. Phase 2: Dependency Locking & Reproducibility Audit

### Dependency Pins (`docs/FRAMEWORK_PIN.json`)

| Component | Pinned Commit | Upstream Repository | Verification Status |
|---|---|---|---|
| **GBARecomp** | `e7728148c6829ba526f682876430a0c9022dc6c0` | `https://github.com/mstan/gbarecomp.git` | **PASS**: Fixed commit verified. |
| **arm-recomp-core** | `c626f4e53fcdca0c72d2a7d34d41663d87c7e175` | `https://github.com/mstan/arm-recomp-core.git` | **PASS**: Fixed commit verified. |
| **SDL2** | `b7502f1a884c055f8535cf8d2be3f44c41669a43` | `https://github.com/libsdl-org/SDL.git` | **PASS**: Fixed commit verified. |
| **toml++** | `30172438cee64926dc41fdd9c11fb3ba5b2ba9de` | `https://github.com/marzer/tomlplusplus.git` | **PASS**: Fixed commit verified. |
| **recomp-ui** | `6374aae201b930c97425718249f5d39b2275ca13` | `https://github.com/RetroPortingToolKit/recomp-ui.git` | **PASS**: Pinned commit exists in both `mstan/recomp-ui` and new upstream `RetroPortingToolKit/recomp-ui` (identical git objects). |

### Framework Patches Integrity (`patches/framework/`)

All 3 framework patches are preserved byte-for-byte and verified against `docs/FRAMEWORK_PIN.json`:

| Patch File | Recorded SHA-256 | Actual File SHA-256 | Status |
|---|---|---|---|
| `0001-obj-color-first-target.patch` | `1f11d32b74d93b7a0c1382b54a8f52ff05b9f249e3b30bdc284df8edd3c7a782` | `1f11d32b74d93b7a0c1382b54a8f52ff05b9f249e3b30bdc284df8edd3c7a782` | **MATCH** |
| `0002-scoped-headless-frame-yield.patch` | `6cb4276db6cd0ba13a6eb75cb17ca3be254bcce1c9690da32a8105d04d7db0c5` | `6cb4276db6cd0ba13a6eb75cb17ca3be254bcce1c9690da32a8105d04d7db0c5` | **MATCH** |
| `0003-scoped-callback-codegen.patch` | `35314ed54c12cb0dc37a14ab5094a650e5c03d77700445981051839b023a5a4f` | `35314ed54c12cb0dc37a14ab5094a650e5c03d77700445981051839b023a5a4f` | **MATCH** |

### MZM Runtime Invariants Preservation
- `g_runtime_force_interp_hook = mzm::copied_code_interpreter_guard`: Fully preserved and active.
- `g_runtime_ram_dispatch_hook = mzm::copied_code_ram_dispatch`: Fully preserved and active.
- 7 moving-stack helper variants (`stack_variant_0` through `stack_variant_6`): Preserved.
- `StackTailDispatch` reentrant continuation engine: Preserved.
- Collision RAM & BG3 Haze RAM byte-checking guards: Preserved.

---

## 4. Phase 3: Comprehensive Component License Audit

| Component | Applicable License | Copyright Holder / Attribution | Commercial Restriction? | Local License File |
|---|---|---|---|---|
| **MZM Integration & Tooling** | MIT | Copyright (c) 2026 Antigravity & Codex Contributors | No | `LICENSE` |
| **GBARecomp** | PolyForm Noncommercial 1.0.0 | Copyright (c) 2026 Matthew Stanley | **YES (Strict Noncommercial)** | `licenses/GBARecomp-PolyForm-Noncommercial.txt` |
| **arm-recomp-core** | MIT | Copyright (c) 2026 Matthew Stanley | No | `licenses/arm-recomp-core.txt` |
| **recomp-ui** | MIT | Copyright (c) 2026 Matthew Stanley | No | `licenses/recomp-ui-MIT.txt` |
| **Dear ImGui** | MIT | Copyright (c) 2014-2024 Omar Cornut | No | `licenses/DearImGui-MIT.txt` |
| **tinyfiledialogs** | zlib | Copyright (c) 2014-2024 Guillaume Vareille | No | `licenses/tinyfiledialogs-zlib.txt` |
| **stb (stb_image, etc.)** | MIT / Public Domain | Sean Barrett | No | `licenses/stb-MIT.txt` |
| **mGBA (BIOS HLE)** | MPL-2.0 | Copyright (c) 2013-2024 Jeffrey Pfau | Weak copyleft (file-level) | `licenses/MPL-2.0.txt` |
| **SDL2** | zlib | Copyright (c) 1997-2024 Sam Lantinga | No | `licenses/SDL2-zlib.txt` |
| **toml++** | MIT | Copyright (c) 2019-2024 Mark Gillard | No | `licenses/tomlplusplus.txt` |
| **Lato Font** | SIL Open Font License 1.1 | Copyright (c) 2010-2015 by tyPoland Lukasz Dziedzic | No | `licenses/Lato-OFL.txt` |
| **Noto Sans Symbols 2** | SIL Open Font License 1.1 | Copyright 2022 The Noto Project Authors | No | `licenses/NotoSymbols-OFL.txt` |
| **OpenMoji** | CC BY-SA 4.0 | OpenMoji Authors | ShareAlike attribute | `licenses/OpenMoji-LICENSE.txt` |
| **MinGW-w64 / GCC Runtime** | GPLv3 with GCC Runtime Library Exception | Free Software Foundation | Permitted linking exception | `licenses/GCC-Runtime-Exception.txt` |
| **JRickey Gamepad Icons** | MIT | Copyright (c) 2024 JRickey | No | `licenses/JRickey-MIT.txt` |
| **metroidret/mzm (Decomp)** | MIT | Copyright (c) 2025 YohannDR | Reference only (no code copied) | `licenses/metroidret-mzm-MIT.txt` |

---

## 5. Phase 4: Compiled Guest Code Copyright Boundary

### Legal Demarcation
1. **No Proprietary Assets Distributed:** Neither the source repository nor the release package contains any Nintendo ROM image (`*.gba`), retail BIOS dump (`*.bin`), audio samples, graphical tilemaps, or extracted script archives.
2. **Compiled Guest Code Nature:** The compiled binary contains native x64 translations of original ARM/Thumb instructions generated through static recompilation. While distinct from distributing the original binary ROM file, recompiled machine code represents an analytical translation of proprietary software.
3. **Mandatory User-Supplied Verification:** The application strictly enforces runtime cryptographic SHA-1 verification. The game will not boot unless the user provides their own legally acquired USA Rev 0 ROM (`5de8536afe1f...`) and retail GBA BIOS (`300c20df6731...`).
4. **Boundary Statement:** The project is positioned strictly as a non-commercial digital preservation and educational research effort. Neither Antigravity nor previous contributors represent that compiled guest code is exempt from copyright law in all jurisdictions.

---

## 6. Phase 5: Standalone Release Package Verification

### Binary Immutability Check
The executable was **NOT recompiled** during this audit. The tested binary from user acceptance was preserved byte-for-byte.

| Artifact | File Size | SHA-256 Checksum | Identity Verification |
|---|---:|---|---|
| **User-Tested Host EXE** (`build/host/MetroidZeroMissionRecomp.exe`) | 34,011,777 bytes | `eef95b2788a84264c55a4c9587c89f55a985a97cdeb06a770924fa917f7844a4` | **ORIGINAL TESTED** |
| **Packaged Release EXE** (`release-stage/.../MetroidZeroMissionRecomp.exe`) | 34,011,777 bytes | `eef95b2788a84264c55a4c9587c89f55a985a97cdeb06a770924fa917f7844a4` | **100% BYTE MATCH** |

### Release Archive Check

- **Archive File:** `release-stage\MetroidZeroMissionRecomp-windows-x64-v0.1.0-preview.zip`
- **Archive Size:** `16,613,754` bytes (`15.84 MB`)
- **Archive SHA-256:** `67afd32cffc613132784fcc31dd10621c72dd7c7ff29eab3cd362ee885b69690`
- **Included Content:**
  - `MetroidZeroMissionRecomp.exe` (Verified identical)
  - `SDL2.dll`, `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`
  - `assets/` (Complete recomp-ui fonts and gamepad icons)
  - `game.toml` (Vanilla game configuration)
  - `README.md`, `README.zh-CN.md`, `LICENSE`, `THIRD_PARTY_NOTICES.md`
  - `licenses/` (All 17 license files included)
  - `SHA256SUMS.txt` & `manifest.json`
- **Prohibited File Scan:** Zero `*.gba`, `*.bin`, `*.sav`, `*.log`, `*.pdb`, `*.cpp`, `*.h` in release stage.
- **Isolated Smoke Test:** Automated isolated extraction to `build/release-test/` and execution of `--help` verified exit code `0` and proper output.

---

## 7. Residual Risks & Blockers

| Risk Item | Severity | Current Status | Remediation / Recommendation |
|---|---|---|---|
| **GBARecomp Commercial Restriction** | Moderate | Mitigated | PolyForm Noncommercial 1.0.0 is explicitly highlighted in README, release notes, and notices. Strictly non-commercial distribution only. |
| **Late-Game / Boss Stability** | Low | Noted in Release Notes | Handled by "Experimental Preview" designation. Community route testing invited. |
| **Upstream Git Operations** | None (Audit Only) | Enforced | Zero git tags created, zero branches pushed, zero releases drafted. Awaiting user's explicit authorization. |

---

## 8. Final Audit Sign-Off

- **Technical Audit Status:** `TECHNICAL RELEASE AUDIT: PASS`
- **Fast-Forward Merge Ready:** Yes (`feature/player-launcher-preview` -> `main`)
- **Release Assets Staged:** Yes (`release-stage/MetroidZeroMissionRecomp-windows-x64-v0.1.0-preview.zip`)
- **Next Action:** Await user's explicit authorization for merge or publishing.
