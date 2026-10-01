# Third-party notices

The original hand-written integration is MIT; this does not relicense its
framework, patches or game/BIOS content. Full license texts are in `licenses/`.

| Component | Pin | License / use |
| --- | --- | --- |
| [GBARecomp](https://github.com/mstan/gbarecomp) | e7728148c6829ba526f682876430a0c9022dc6c0 | PolyForm Noncommercial1.0.0; Copyright2026 Matthew Stanley; native framework and3 public source patches |
| [arm-recomp-core](https://github.com/mstan/arm-recomp-core) | c626f4e53fcdca0c72d2a7d34d41663d87c7e175 | MIT; Copyright2026 Matthew Stanley; decoder/emitter/core |
| [metroidret/mzm](https://github.com/metroidret/mzm) | 43b7fd52f552e4d38c1521ff9d4df5ee57e61493 | MIT; Copyright2025 YohannDR; C/ASM/function-boundary/callback/layout and game-state reference |
| [SDL2](https://github.com/libsdl-org/SDL) | b7502f1a884c055f8535cf8d2be3f44c41669a43 | zlib; display/input/audio backend |
| [toml++](https://github.com/marzer/tomlplusplus) | 30172438cee64926dc41fdd9c11fb3ba5b2ba9de | MIT; configuration parser |

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

Historical architecture reference: MegaManZeroRecomp218adfddf3572a6eaf7ffbec858d95c71bf53935,
same framework license; no Mega Man addresses/assets/implementation imported.
Historical separate-process oracle: mGBA0.10.5 commit26b7884bc25a5933960f3cdcd98bac1ae14d42e2,
MPL-2.0, Jeffrey Pfau and contributors. It is not linked into the game host or
part of the active preview acceptance workflow. Its local checkout retains the
full LICENSE; no oracle source/binary is published here.

These software licenses do not grant rights to redistribute Nintendo ROM/BIOS,
extracted assets, saves or ROM-derived generated source. Such files, compiler
outputs, debug archives and local dependencies are ignored and absent from the
prepared source repository.
