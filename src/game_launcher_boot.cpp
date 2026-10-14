// Generic GBARecomp/recomp-ui seam; no game logic or ROM-derived source.
#include "launcher_seam.h"
#include "sha1.h"

int game_launcher_preboot(std::vector<std::string>& args, const gbarecomp::RunOptions& options) {
    return gbarecomp_launcher_preboot(args, options);
}

// The generic runtime permits warn-and-try BIOS dumps. This game's translated
// retail BIOS requires exact identity verification.
int game_validate_inputs(std::vector<std::string>& args, const gbarecomp::RunOptions& options) {
    std::string path, rom_path;
    for (std::size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "--help" || args[i] == "-h") return 0;
        if (args[i] == "--bios" && i + 1 < args.size()) path = args[++i];
        else if (args[i] == "--rom" && i + 1 < args.size()) rom_path = args[++i];
    }
    if (path.empty()) {
        path = gbarecomp_seam::read_single_line(
            (std::filesystem::path(gbarecomp_seam::exe_dir(args)) / "bios.cfg").string());
    }
    if (path.empty()) {
        std::fprintf(stderr, "[mzm] Retail BIOS required: select it in the launcher or pass --bios.\n");
        return 1;
    }
    gba::GbaBios bios;
    std::string error;
    if (!bios.load_from_file(path, gba::GbaBios::kExpectedSha1, &error)) {
        std::fprintf(stderr, "[mzm] BIOS identity rejected: %s\n", error.c_str());
        return 1;
    }
    if (rom_path.empty()) {
        rom_path = gbarecomp_seam::read_single_line(
            (std::filesystem::path(gbarecomp_seam::exe_dir(args)) / "rom.cfg").string());
    }
    std::ifstream rom(rom_path, std::ios::binary | std::ios::ate);
    if (!rom || rom.tellg() != 8388608) {
        std::fprintf(stderr, "[mzm] Expected the 8 MiB USA BMXE ROM; select it in the launcher or pass --rom.\n");
        return 1;
    }
    std::vector<std::uint8_t> bytes(8388608);
    rom.seekg(0);
    rom.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    if (!rom || gba::sha1(bytes.data(), bytes.size()).hex() != options.builtin_rom_sha1) {
        std::fprintf(stderr, "[mzm] ROM SHA-1 mismatch; expected %s.\n", options.builtin_rom_sha1);
        return 1;
    }
    // Validate before the runtime can persist an explicit invalid path.
    args.emplace_back("--rom");
    args.emplace_back(rom_path);
    args.emplace_back("--bios");
    args.emplace_back(path);
    // An explicit CLI/config hash cannot weaken this cartridge's identity gate.
    args.emplace_back("--rom-sha1");
    args.emplace_back(options.builtin_rom_sha1);
    return 0;
}
