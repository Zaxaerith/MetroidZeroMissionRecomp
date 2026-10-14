# Third-party notices

The original hand-written integration is licensed under MIT (Copyright (c) 2026 MetroidZeroMissionRecomp contributors); this does not relicense its
framework, patches or game/BIOS content. Full license texts are in `licenses/`.

| Component | Pin / Origin | License / use |
| --- | --- | --- |
| [GBARecomp](https://github.com/mstan/gbarecomp) | e7728148c6829ba526f682876430a0c9022dc6c0 | PolyForm Noncommercial 1.0.0; Copyright 2026 Matthew Stanley; native framework and 3 public source patches; see `licenses/GBARecomp-PolyForm-Noncommercial.txt` |
| [arm-recomp-core](https://github.com/mstan/arm-recomp-core) | c626f4e53fcdca0c72d2a7d34d41663d87c7e175 | MIT; Copyright 2026 Matthew Stanley; decoder/emitter/core; see `licenses/arm-recomp-core.txt` |
| [metroidret/mzm](https://github.com/metroidret/mzm) | 43b7fd52f552e4d38c1521ff9d4df5ee57e61493 | MIT; Copyright 2025 YohannDR; C/ASM/function-boundary/callback/layout and game-state reference; see `licenses/metroidret-mzm-MIT.txt` |
| [SDL2](https://github.com/libsdl-org/SDL) | b7502f1a884c055f8535cf8d2be3f44c41669a43 | zlib; display/input/audio backend; see `licenses/SDL2-zlib.txt` |
| [toml++](https://github.com/marzer/tomlplusplus) | 30172438cee64926dc41fdd9c11fb3ba5b2ba9de | MIT; configuration parser; see `licenses/tomlplusplus.txt` |
| [recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui) | 6374aae201b930c97425718249f5d39b2275ca13 | MIT; Copyright 2026 Matthew Stanley; player GUI launcher and input configuration (formerly mstan/recomp-ui); see `licenses/recomp-ui-MIT.txt` |
| [Dear ImGui](https://github.com/ocornut/imgui) | Bundled in recomp-ui | MIT; Copyright (c) 2014-2024 Omar Cornut; see `licenses/DearImGui-MIT.txt` |
| [tinyfiledialogs](https://sourceforge.net/projects/tinyfiledialogs/) | Bundled in recomp-ui | zlib; Copyright (c) 2014 - 2024 Guillaume Vareille; see `licenses/tinyfiledialogs-zlib.txt` |
| [stb](https://github.com/nothings/stb) | Bundled in recomp-ui | MIT / Public Domain; Sean Barrett; see `licenses/stb-MIT.txt` |
| [mGBA](https://github.com/mgba-emu/mgba) (BIOS HLE) | Embedded in GBARecomp (`src/runtime/bios_hle.{h,cpp}`) | MPL-2.0; Copyright (c) 2013-2024 Jeffrey Pfau; see `licenses/MPL-2.0.txt`. Source available at https://github.com/mstan/gbarecomp |
| [Lato Font](http://www.latofonts.com/) | Bundled in recomp-ui assets | SIL Open Font License 1.1; Copyright (c) 2010-2015 by tyPoland Lukasz Dziedzic; see `licenses/Lato-OFL.txt` |
| [Noto Sans Symbols 2](https://fonts.google.com/noto/specimen/Noto+Sans+Symbols+2) | Bundled in recomp-ui assets | SIL Open Font License 1.1; Copyright 2022 The Noto Project Authors; see `licenses/NotoSymbols-OFL.txt` |
| [OpenMoji](https://openmoji.org/) | Bundled in recomp-ui assets | CC BY-SA 4.0; see `licenses/OpenMoji-LICENSE.txt` |
| [MinGW-w64 / GCC Runtime](https://gcc.gnu.org/) | Runtime DLLs | GPLv3 with GCC Runtime Library Exception; Free Software Foundation; see `licenses/GCC-Runtime-Exception.txt` |
| [JRickey Controller Icons](https://jrickey.itch.io/) | Bundled in recomp-ui assets | MIT; Copyright (c) 2024 JRickey; see `licenses/JRickey-MIT.txt` |

## MPL-2.0 Compliance Notice

The executable statically links `reference/gbarecomp/src/runtime/bios_hle.{h,cpp}`, which are ported from mGBA's `src/gba/bios.c` and are Covered Software governed by the Mozilla Public License, v. 2.0 (MPL-2.0). In accordance with MPL-2.0 Section 3.2, recipients of the executable form are hereby informed that the Source Code Form of this Covered Software is publicly available without charge at:
`https://github.com/mstan/gbarecomp/tree/e7728148c6829ba526f682876430a0c9022dc6c0/src/runtime/`
or via this project's dependency setup script (`tools/setup-deps.ps1`). Under MPL-2.0 Section 3.3, this project constitutes a Larger Work; the MPL-2.0 license continues to apply to `bios_hle.{h,cpp}` without alteration.

`licenses/GBARecomp-THIRD-PARTY.md` reproduces the framework's full attribution,
including MIT/Apache ports and MPL-2.0-covered BIOS-HLE files. Its terms and
file-level notices continue to apply. No framework source is vendored into this
public integration; setup fetches the pinned source and applies ROM-free patches.
The actual framework LICENSE includes its author's noncommercial clarification;
consult that full text rather than treating the whole stack as MIT.

MZM's full MIT text is preserved in `licenses/metroidret-mzm-MIT.txt`. Decomp
semantic names and callback relationships inform ROM-verified metadata. No
copied decomp C/ASM execution implementation is currently linked. Future direct
reuse must retain author/license notices and document the source functions and
version/ABI verification. A decomp without explicit permission is local-reference
only; its copied implementation must not enter the public repository.

Historical architecture reference: MegaManZeroRecomp 218adfddf3572a6eaf7ffbec858d95c71bf53935,
same framework license; no Mega Man addresses/assets/implementation imported.
Historical separate-process oracle: mGBA 0.10.5 commit 26b7884bc25a5933960f3cdcd98bac1ae14d42e2,
MPL-2.0, Jeffrey Pfau and contributors. It is not linked into the game host or
part of the active preview acceptance workflow. Its local checkout retains the
full LICENSE; no oracle source/binary is published here.

These software licenses do not grant rights to redistribute Nintendo ROM/BIOS,
extracted assets, saves or ROM-derived generated source. Such files, compiler
outputs, debug archives and local dependencies are ignored and absent from the
prepared source repository.
