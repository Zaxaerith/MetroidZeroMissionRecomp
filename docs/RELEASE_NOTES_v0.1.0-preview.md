# Metroid: Zero Mission Recomp — v0.1.0-preview Release Notes

**Release Version:** v0.1.0-preview  
**Release Date:** October 2026  
**Target Platform:** Windows x64 (Native Standalone)  
**Supported Game:** *Metroid: Zero Mission* (USA, BMXE, Revision 0)  

---

## 1. Release Overview & Experimental Preview Notice

This is the initial **v0.1.0-preview** release of **MetroidZeroMissionRecomp**, an unofficial native Windows x64 static recompilation of *Metroid: Zero Mission* built on the [GBARecomp](https://github.com/mstan/gbarecomp) framework and [recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui) launcher.

> [!WARNING]
> **Experimental Preview Notice**  
> This release represents an early, non-commercial research and digital preservation preview. While foundational systems, physics, and gameplay have been strictly verified through manual testing, this is **not** a claimed 100% bug-free or full-game completion certified release. Unforeseen issues may occur in unverified late-game chapters.

---

## 2. No Original ROM or BIOS Files Included

**This package contains ZERO proprietary Nintendo game assets, ROM dumps, or BIOS images.**

### Notice on Compiled Guest Code and Distribution Boundary
The standalone executable contains statically recompiled native x64 translations of original Game Boy Advance machine instructions. It does not contain or distribute the original proprietary ROM or BIOS binary images, graphics tiles, audio samples, or cartridge assets.

To satisfy runtime integrity verification and play the game, players **must provide their own legally obtained original ROM and retail GBA BIOS files**. The launcher enforces strict cryptographic checksums before allowing the game to boot.

### Required Private Files Specification

| File | Supported Identity | Expected Size | SHA-1 Hash | SHA-256 Checksum |
|---|---|---:|---|---|
| **Game ROM** | *Metroid: Zero Mission* (USA, Code `BMXE`, Rev 0) | 8,388,608 bytes | `5de8536afe1f0078ee6fe1089f890e8c7aa0a6e8` | `fc94f65380b65b870a30b9b04b39cca1dc63d6e46a4a373d3904adc0912ebc37` |
| **GBA BIOS** | Retail Game Boy Advance BIOS | 16,384 bytes | `300c20df6731a33952ded8c436f7f186d25d3492` | `94c229e5e3428d003b689c1ad4d6199a5e427fb2e260bfb2d41a7c73b9845d4f` |

*Note: Non-USA regional releases (European, Japanese), modified ROMs, and high-level emulation (HLE) BIOS clones will fail hash validation and will not launch.*

---

## 3. Verified Playable Scope

The following systems and milestones have been strictly validated on native Windows x64:
- [x] **Native Launcher GUI (`recomp-ui`)**: Integrated launcher powered by Dear ImGui with file browsing, real-time SHA-1 verification, keyboard and controller rebinding, display resolution/integer scaling, and audio volume adjustment.
- [x] **LLE Retail BIOS Cold Boot**: Accurate boot reset sequence and authentic Nintendo GBA boot screen.
- [x] **Title Screen & New Game**: Title sequence animations, save slot creation, and seamless transition to intro cutscene.
- [x] **Samus Aran Core Controls**: Native 60 FPS physics, running, aiming (straight & diagonal), beam firing, missile firing, jumping, wall jumping, crouching, and Morph Ball transformation.
- [x] **Room Transitions & Graphics**: Real-time room gate transitions, tilemap decompression, BG3 atmospheric haze effects, and collision RAM code dispatch.
- [x] **Authentic SRAM Persistence**: Standard 32 KiB GBA SRAM save format (`saves/metroid_zero_mission_usa.sav`). Save stations, clean exits, and cold process reloads verified without data corruption.
- [x] **Configuration Persistence**: Automatic memory of valid ROM/BIOS paths, window geometry, and input mappings across process restarts (`rom.cfg`, `bios.cfg`, `config.ini`, `keybinds.ini`).

---

## 4. Open for Community Testing

Community feedback and route reports are warmly invited for:
- Full campaign progression from Brinstar through Norfair, Kraid, Ridley, Tourian, and Chozodia.
- Late-game boss battles (Mother Brain, Ridley, Mecha Ridley) and space pirate stealth escape.
- 100% item completion routes, speedrunning tricks, and post-credits ending galleries.
- Extended audio fidelity across prolonged multi-channel sound playback.

---

## 5. Quick Start for Players

1. Extract `MetroidZeroMissionRecomp-windows-x64-v0.1.0-preview.zip` to a writable local directory.
2. Ensure `MetroidZeroMissionRecomp.exe`, runtime DLLs (`SDL2.dll`, `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`), `assets/`, and `licenses/` remain together.
3. Launch `MetroidZeroMissionRecomp.exe`.
4. Click **Browse** to select your valid USA ROM and retail GBA BIOS.
5. Configure your preferred display scaling, audio volume, and keyboard/controller bindings.
6. Click **Play** to start the game!

*(Tip: To reopen the launcher after selecting "Skip Launcher", pass `--launcher` via a shortcut or command line.)*

---

## 6. Technical Architecture & Invariant Protections

- **AOT Recompilation Framework**: Pinned GBARecomp (`e7728148...`) and arm-recomp-core (`c626f4e5...`).
- **MZM Special RAM Execution**: Protected collision and haze RAM code via byte-checked runtime hooks (`mzm::copied_code_interpreter_guard` and `mzm::copied_code_ram_dispatch`).
- **Moving Stack Execution**: 7 qualified moving-stack variants with reentrant-safe `StackTailDispatch` and headless stopping boundary synchronization.
- **Three Framework Patches**:
  1. `0001-obj-color-first-target.patch`: Proper BLDCNT opaque OBJ first target blending.
  2. `0002-scoped-headless-frame-yield.patch`: Scoped RAII headless frame yield stopping boundary.
  3. `0003-scoped-callback-codegen.patch`: Scoped callback codegen for measured headless interpreter subtrees.

---

## 7. Credits & Third-Party Licenses

- **GBARecomp**: Developed by Matthew Stanley ([mstan/gbarecomp](https://github.com/mstan/gbarecomp)) — PolyForm Noncommercial 1.0.0.
- **recomp-ui**: Developed by Matthew Stanley ([RetroPortingToolKit/recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui), formerly `mstan/recomp-ui`) — MIT License.
- **arm-recomp-core**: Developed by Matthew Stanley ([mstan/arm-recomp-core](https://github.com/mstan/arm-recomp-core)) — MIT License.
- **MetroidZeroMissionRecomp Integration**: Original hand-written integration and tooling — MIT License.
- **Third-Party Libraries & Assets**:
  - Dear ImGui (MIT) — Omar Cornut
  - SDL2 (zlib) — Sam Lantinga & SDL contributors
  - toml++ (MIT) — Mark Gillard
  - tinyfiledialogs (zlib) — Guillaume Vareille
  - stb (MIT / Public Domain) — Sean Barrett
  - mGBA BIOS HLE (MPL-2.0) — Jeffrey Pfau & mGBA contributors
  - Fonts (Lato, Noto Sans Symbols 2, OpenMoji) — SIL OFL 1.1 / CC BY-SA 4.0
  - Controller Icons (JRickey) — MIT License
  - MinGW-w64 / GCC Runtime — GPLv3 with GCC Runtime Library Exception
- Detailed notices and license texts are located in `THIRD_PARTY_NOTICES.md` and the `licenses/` directory.

---

## 8. Legal Disclaimer

*Metroid*, *Metroid: Zero Mission*, *Samus Aran*, and all associated trademarks and copyrights are the property of **Nintendo Co., Ltd.**

This project is an independent, non-commercial software research and preservation project. It is **not** endorsed by, sponsored by, or affiliated with Nintendo.
