# Supported cartridge

Place one legally obtained `.gba` in the project root. The current scripts
use `Metroid - Zero Mission (USA).gba`; cartridge bytes are never distributed.

| Field | Verified value |
| --- | --- |
| Region / revision | USA / 0 |
| Header title | ZEROMISSIONE |
| Game code / maker | BMXE / 01 |
| Size | 8,388,608 bytes |
| SHA-1 | `5de8536afe1f0078ee6fe1089f890e8c7aa0a6e8` |
| SHA-256 | `fc94f65380b65b870a30b9b04b39cca1dc63d6e46a4a373d3904adc0912ebc37` |
| Save | SRAM_V113, observed 32 KiB |

A user-supplied real GBA BIOS is required. Pass its path with `--bios` or
`-Bios`; expected size is 16,384 bytes and SHA-1 is
`300c20df6731a33952ded8c436f7f186d25d3492`.

These are the recorded, already measured identity values, not new region
support. Europe/Japan are not qualified. The pinned USA decomp has not been
rebuilt to the same hash; no decomp addresses/symbols are imported. See
[identity record](docs/ROM_IDENTITY.json) and [progress](docs/PROGRESS.md).
