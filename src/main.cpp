#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "runtime.h"
#include "runtime_arm.h"
#include "copied_code.h"

int game_launcher_preboot(std::vector<std::string>&, const gbarecomp::RunOptions&);
int game_validate_inputs(std::vector<std::string>&, const gbarecomp::RunOptions&);

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            std::puts("MetroidZeroMissionRecomp [--rom <USA ROM>] [--bios <GBA BIOS>] [game.toml]\n"
                      "Options:\n"
                      "  --launcher         Force GUI launcher\n"
                      "  --window           Run in windowed mode\n"
                      "  --fullscreen       Run in fullscreen mode\n"
                      "  --scale <1..8>     Window scale factor\n"
                      "  --volume <0..100>  Audio volume\n"
                      "  --save <path>      Save file path (default: saves/metroid_zero_mission_usa.sav)\n"
                      "  --no-window        Headless validation mode\n"
                      "  --frames <count>   Stop after specified frame count\n"
                      "\n"
                      "Faithful 240x160 presentation; cartridge and BIOS hashes are verified.");
            return 0;
        }
    }

    gbarecomp::RunOptions options;
    options.builtin_game_name = "Metroid: Zero Mission";
    options.builtin_rom_sha1 = "5de8536afe1f0078ee6fe1089f890e8c7aa0a6e8";
    options.builtin_rom_crc32 = 0x5c61a844;
    options.freely_resizable_window = true;
    options.expose_assist_tools = false;
    options.launcher_region = "USA";
    options.launcher_game_config = "game.toml";
    options.launcher_expose_widescreen = false;
    options.launcher_expose_adaptive_view = false;

    std::vector<std::string> args(argv, argv + argc);
    const bool player_launch = argc == 1 || (argc == 2 && args[1] == "--launcher");

    std::error_code ec;
    auto exe_path = std::filesystem::absolute(argv[0], ec);
    auto exe_dir = ec ? std::filesystem::current_path() : exe_path.parent_path();

    if (player_launch) {
        // The portable launcher, settings, and assets live beside the EXE.
        if (!ec) std::filesystem::current_path(exe_dir, ec);
    }

    std::string save_path;
    bool has_cli_save = false;
    for (std::size_t i = 1; i < args.size(); ++i) {
        if ((args[i] == "--save" || args[i] == "--save-path") && i + 1 < args.size()) {
            has_cli_save = true;
            save_path = args[i + 1];
            break;
        }
    }

    if (!has_cli_save) {
        if (std::filesystem::exists(exe_dir / "saves" / "metroid_zero_mission_usa.sav")) {
            save_path = (exe_dir / "saves" / "metroid_zero_mission_usa.sav").string();
        } else if (std::filesystem::exists(exe_dir / "metroid_zero_mission_usa.sav")) {
            save_path = (exe_dir / "metroid_zero_mission_usa.sav").string();
        } else if (std::filesystem::exists("saves/metroid_zero_mission_usa.sav")) {
            save_path = "saves/metroid_zero_mission_usa.sav";
        } else {
            std::filesystem::create_directories(exe_dir / "saves", ec);
            save_path = (exe_dir / "saves" / "metroid_zero_mission_usa.sav").string();
        }
    }
    options.launcher_save_path = save_path.c_str();

    // Pre-boot GUI launcher
    if (game_launcher_preboot(args, options)) return 0;
    // Strict ROM & BIOS input gate
    if (game_validate_inputs(args, options)) return 1;

    if (player_launch) {
        if (!has_cli_save) {
            args.emplace_back("--save");
            args.emplace_back(options.launcher_save_path);
        }
        args.emplace_back("--window");
    }

    // Preserve MZM special RAM execution mechanisms (interpreter guard + dynamic RAM dispatch)
    g_runtime_force_interp_hook = mzm::copied_code_interpreter_guard;
    g_runtime_ram_dispatch_hook = mzm::copied_code_ram_dispatch;

    std::vector<char*> native_args;
    native_args.reserve(args.size());
    for (auto& arg : args) native_args.push_back(arg.data());

    return gbarecomp::run_game(static_cast<int>(native_args.size()), native_args.data(), options);
}
