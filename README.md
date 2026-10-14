# MetroidZeroMissionRecomp

[English](README.md) | [简体中文](README.zh-CN.md)

A Windows x64 native recompilation integration for **Metroid: Zero Mission (USA, Game Code: BMXE, Revision 0)**, built using [GBARecomp](https://github.com/mstan/gbarecomp) and [recomp-ui](https://github.com/mstan/recomp-ui).

> **v0.1.0-preview — Experimental Preview**  
> This unofficial, non-commercial project is not affiliated with, endorsed by, or associated with Nintendo. Players must supply their own legally obtained GBA ROM and retail GBA BIOS. Full-game completion is not claimed.

*Metroid: Zero Mission* is Nintendo's critically acclaimed action-adventure masterpiece for the Game Boy Advance. This native Windows integration translates original ARM/Thumb code into native x64 instructions, preserving original game timing, collision mechanics, audio, and cartridge SRAM persistence while providing a modern desktop player launcher.

---

## Verified Playable Scope & Current Status

* [x] **BIOS / Cold Boot**: LLE BIOS reset sequence and official Nintendo intro verified (`gba_bios.bin`).
* [x] **Title Screen & New Game**: Title sequence animations, file selection / New Game creation verified.
* [x] **Core Samus Actions**: Movement, aiming, firing beam/missiles, jumping, crouching, and Morph Ball transformation verified.
* [x] **Room Transitions**: Room transition gates, map tile loading, BG3 haze, and collision RAM execution verified.
* [x] **Cartridge SRAM Persistence**: Original 32 KiB SRAM save format verified (`saves/metroid_zero_mission_usa.sav`), including in-game save station save, clean quit, and fresh process reload.
* [x] **Native Player GUI Launcher**: Integrated interactive GUI (`recomp-ui`) with strict SHA-1 and file size validation, keyboard & gamepad rebinding, display scaling, volume control, and persistent launch.

### Scope Open for Community Testing

This preview focuses on a solid, verified native foundation. The following chapters and features remain open for community testing and feedback:
* Full campaign progression across Brinstar, Norfair, Kraid, Ridley, Tourian, and Chozodia.
* Late-game boss battles, Mecha Ridley, and escape sequence.
* All 100% item completion routes and gallery endings.
* Extended listening across all audio channels during complex boss encounters.

---

## Quick Start (Players)

No developer tools (CMake, Ninja, GCC, Python, or command-line shells) are required.

1. **Download**: Grab `MetroidZeroMissionRecomp-windows-x64-v0.1.0-preview.zip`.
2. **Extract**: Extract the entire archive into a writable folder on your computer. Ensure `MetroidZeroMissionRecomp.exe`, runtime DLLs (`SDL2.dll`, `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`), `assets/`, and `licenses/` stay together.
3. **Launch**: Double-click `MetroidZeroMissionRecomp.exe`.
4. **Setup in Launcher**:
   * Click **Browse** to select your supported USA ROM (`.gba`) and retail GBA BIOS (`.bin`).
   * The launcher automatically computes SHA-1 checksums and verifies file sizes. Once valid, green checkmarks appear.
   * Customize your controller or keyboard layout, window scaling, and audio volume.
5. **Play**: Click **Play** to enter the game!

> [!TIP]
> Your selected ROM, BIOS, and settings are saved automatically (`rom.cfg`, `bios.cfg`, `config.ini`, `keybinds.ini`). If you ever check "Skip Launcher" and later want to reopen the setup GUI, run `MetroidZeroMissionRecomp.exe --launcher` from a shortcut or command prompt.

---

## Required Private Files

You must provide your own legally dumped game ROM and retail GBA BIOS. Neither file is included in this repository or any release download.

| File | Supported Identity | Expected Size | SHA-1 Hash | SHA-256 Checksum |
|---|---|---:|---|---|
| **Game ROM** | *Metroid: Zero Mission* (USA, Code `BMXE`, Rev 0) | 8,388,608 bytes | `5de8536afe1f0078ee6fe1089f890e8c7aa0a6e8` | `fc94f65380b65b870a30b9b04b39cca1dc63d6e46a4a373d3904adc0912ebc37` |
| **GBA BIOS** | Retail Game Boy Advance BIOS | 16,384 bytes | `300c20df6731a33952ded8c436f7f186d25d3492` | `94c229e5e3428d003b689c1ad4d6199a5e427fb2e260bfb2d41a7c73b9845d4f` |

> [!WARNING]
> Other regional releases (European, Japanese), modified ROMs, and high-level emulation (HLE) BIOS replacements are strictly rejected by the launcher to ensure execution integrity. Renaming an unsupported file will not bypass hash verification.

---

## Controls, Display, and Audio

### Default Keyboard Mapping

| GBA Button | PC Default Key | Function |
|---|---|---|
| **D-Pad** | `W / A / S / D` or `↑ / ↓ / ← / →` | Movement / Aiming / Menu Navigation |
| **A** | `Z` / `J` | Jump / Confirm |
| **B** | `X` / `K` | Fire Beam / Fire Missile / Cancel |
| **L** | `Q` / `U` | Diagonal Aim Down / Missile Select |
| **R** | `E` / `I` | Diagonal Aim Up / Missile Arm |
| **START** | `Enter` | Pause / Status Screen |
| **SELECT** | `Backspace` / `Tab` | Minimap Screen |

### Game Controller Support

Standard modern game controllers (Xbox / XInput, PlayStation DualShock 4 / DualSense, Nintendo Switch Pro Controller, and generic DirectInput gamepads) are natively supported via SDL2. Bindings can be freely customized within the launcher's **Input** tab.

### Display and Audio Settings

* **Window Scaling**: 1x through 8x integer scaling, or borderless / exclusive fullscreen.
* **Aspect Ratio**: Faithful 3:2 GBA aspect ratio (240 × 160 native resolution).
* **Audio**: Volume slider with 16-bit 32.768 kHz direct stereo mixing.

---

## Native Saves and Configuration

* **Cartridge Saves**: When launched via the GUI or double-click, game saves are stored beside the executable in **`saves/metroid_zero_mission_usa.sav`**. This is an authentic 32 KiB SRAM image directly compatible with GBA hardware and standard emulators.
* **Settings Persistence**: Launcher options, file paths, and keybindings are stored in `rom.cfg`, `bios.cfg`, `config.ini`, and `keybinds.ini` in the executable folder.
* **Upgrades**: When upgrading to a newer release, copy your `saves/` folder and `.cfg`/`.ini` files into the new directory to preserve your progress and configurations.

---

## Technical Architecture

* **AOT Static Recompilation**: Uses pinned [GBARecomp](https://github.com/mstan/gbarecomp) (`e7728148c6829ba526f682876430a0c9022dc6c0`) and [arm-recomp-core](https://github.com/mstan/arm-recomp-core) (`c626f4e53fcdca0c72d2a7d34d41663d87c7e175`).
* **Framework Patches**:
  1. `0001-obj-color-first-target.patch`: Honor BLDCNT opaque OBJ first target while keeping semitransparent alpha distinct.
  2. `0002-scoped-headless-frame-yield.patch`: Scoped RAII frame yield preserving interpreter-subtree stopping boundaries.
  3. `0003-scoped-callback-codegen.patch`: Opt-in boolean callback-table codegen scope preserving measured headless boundaries.
* **MZM RAM Code Execution & Protection**:
  * `g_runtime_force_interp_hook = mzm::copied_code_interpreter_guard`: Byte-verified integrity checks for collision and haze RAM images before interpreting or dispatching.
  * `g_runtime_ram_dispatch_hook = mzm::copied_code_ram_dispatch`: Tail-dispatch execution for 7 qualified moving-stack helper variants (`stack_variant_0` through `stack_variant_6`).
  * `StackTailDispatch`: Reentrant-safe continuation dispatcher preventing stack overflow on interrupt nest changes.
  * `DeferHeadlessFrameYield`: Headless stopping boundary synchronization.
* **GUI Launcher**: Pinned [recomp-ui](https://github.com/mstan/recomp-ui) (`6374aae201b930c97425718249f5d39b2275ca13`) powered by Dear ImGui.
* **Host Platform**: SDL2 drives window creation, hardware OpenGL rendering, low-latency audio streaming, and controller hot-plugging.
* **Decompilation Assistance**: Appreciation to [metroidret/mzm](https://github.com/metroidret/mzm) contributors for documenting symbol boundaries, callback relationships, and memory layout. No copyrighted game code or assets from decompilation projects are redistributed in this repository.

---

## Building from Source (Developers)

### Prerequisites

* Windows 10 / 11 64-bit
* MinGW-w64 (GCC 11+ with C++20 support)
* CMake 3.20+ and Ninja
* Python 3.11+
* Git

### Step-by-Step Build

1. Clone the repository:
   ```powershell
   git clone https://github.com/Zaxaerith/MetroidZeroMissionRecomp.git
   cd MetroidZeroMissionRecomp
   ```

2. Fetch pinned source dependencies:
   ```powershell
   .\tools\setup-deps.ps1
   ```

3. Build host, framework tools, regenerate cartridge/BIOS source, and compile:
   ```powershell
   .\tools\build-host.ps1 -Toolchain "C:\path\to\mingw64" -Bios "C:\path\to\gba_bios.bin" -Regenerate -Jobs 8
   ```

4. Run the compiled executable:
   ```powershell
   .\build\host\MetroidZeroMissionRecomp.exe
   ```

### Developer Command-Line Options

* `--rom <path>`: Specify path to GBA ROM directly.
* `--bios <path>`: Specify path to GBA BIOS directly.
* `--save <path>`: Specify custom save file path.
* `--launcher`: Force open the GUI launcher even if previously skipped.
* `--frames <count>`: Run for a fixed number of frames and exit (headless validation).
* `--no-window`: Run in headless mode without creating an SDL window.

---

## Credits and Licenses

* **GBARecomp**: Developed by Matthew Stanley ([mstan/gbarecomp](https://github.com/mstan/gbarecomp)). Licensed under [PolyForm Noncommercial 1.0.0](https://polyformproject.org/licenses/noncommercial/1.0.0/).
* **recomp-ui**: Developed by Matthew Stanley ([mstan/recomp-ui](https://github.com/mstan/recomp-ui)). Licensed under MIT.
* **arm-recomp-core**: Developed by Matthew Stanley ([mstan/arm-recomp-core](https://github.com/mstan/arm-recomp-core)). Licensed under MIT.
* **MetroidZeroMissionRecomp Integration**: Integration code, stack dispatch, and tooling. Licensed under [MIT](LICENSE).
* **Third-Party Components**:
  * [Dear ImGui](https://github.com/ocornut/imgui) — MIT License
  * [toml++](https://github.com/marzer/tomlplusplus) — MIT License
  * [Simple DirectMedia Layer (SDL2)](https://www.libsdl.org/) — zlib License
  * Fonts (*LatoLatin*, *OpenMoji*, *NotoSansSymbols2*) — OFL-1.1 / CC BY-SA 4.0
  * MinGW-w64 / GCC Runtime (`libgcc`, `libstdc++`, `libwinpthread`) — GPLv3 with GCC Runtime Library Exception
* See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and `licenses/` for full licensing information.

---

## Legal Notice

*Metroid*, *Metroid: Zero Mission*, *Samus Aran*, and all related trademarks and copyrights belong to **Nintendo Co., Ltd.**

This project is an independent, non-commercial software engineering and digital preservation project. It is **not** affiliated with, sponsored by, or endorsed by Nintendo. No proprietary ROM images, BIOS dumps, or copyrighted assets are hosted or distributed in this repository.
