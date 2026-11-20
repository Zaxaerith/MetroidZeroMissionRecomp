#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "runtime.h"
#include "runtime_arm.h"
#include "copied_code.h"
#include "native_thunk.h"
#include "native_pipeline.h"
#include "mod_function_hooks.h"

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
                      "  --native-math      Enable Native C++ FixedMultiplication (0x08000A4C)\n"
                      "  --native-soft-reset Enable Native C++ SoftResetCheck (0x080007D0)\n"
                      "  --native-samus-anim Enable Native C++ SamusUpdateAnimation (0x0800847C)\n"
                      "  --native-samus-turn Enable Native C++ SamusTurningAroundGfx (0x080088F8)\n"
                      "  --native-samus-standing-gfx Enable Native C++ SamusStandingGfx (0x0800881C)\n"
                      "  --native-samus-shooting-gfx Enable Native C++ SamusShootingGfx (0x08008940)\n"
                      "  --native-samus-shooting-crouching-gfx Enable Native C++ SamusShootingAndCrouchingGfx (0x08008C50)\n"
                      "  --native-samus-turning-crouching-gfx Enable Native C++ SamusTurningAroundAndCrouchingGfx (0x08008C24)\n"
                      "  --native-samus-starting-spin-jump-gfx Enable Native C++ SamusStartingSpinJumpGfx (0x08008ED8)\n"
                      "  --native-samus-turning-midair-gfx Enable Native C++ SamusTurningAroundMidAirGfx (0x08008EB8)\n"
                      "  --native-all       Enable all verified Native C++ replacements\n"
                      "  --no-native-math   Disable Native C++ FixedMultiplication\n"
                      "  --no-native-soft-reset Disable Native C++ SoftResetCheck\n"
                      "  --no-native-samus-anim Disable Native C++ SamusUpdateAnimation\n"
                      "  --no-native-samus-turn Disable Native C++ SamusTurningAroundGfx\n"
                      "  --no-native-samus-standing-gfx Disable Native C++ SamusStandingGfx\n"
                      "  --no-native-samus-shooting-gfx Disable Native C++ SamusShootingGfx\n"
                      "  --no-native-samus-shooting-crouching-gfx Disable Native C++ SamusShootingAndCrouchingGfx\n"
                      "  --no-native-samus-turning-crouching-gfx Disable Native C++ SamusTurningAroundAndCrouchingGfx\n"
                      "  --no-native-samus-starting-spin-jump-gfx Disable Native C++ SamusStartingSpinJumpGfx\n"
                      "  --no-native-samus-turning-midair-gfx Disable Native C++ SamusTurningAroundMidAirGfx\n"
                      "  --no-native-all    Disable all Native C++ replacements (default: original AOT)\n"
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

    // Process native pipeline experimental toggles
    bool enable_math = false;
    bool enable_soft_reset = false;
    bool enable_samus_anim = false;
    bool enable_samus_turn = false;
    bool enable_samus_standing = false;
    bool enable_samus_shooting = false;
    bool enable_samus_shooting_crouching = false;
    bool enable_samus_turning_crouching = false;
    bool enable_samus_starting_spin_jump = false;
    bool enable_samus_turning_midair = false;

    // Check environment variables
    const char* env_all = std::getenv("MZM_NATIVE_ALL");
    if (env_all && (std::strcmp(env_all, "1") == 0 || std::strcmp(env_all, "true") == 0)) {
        enable_math = true;
        enable_soft_reset = true;
        enable_samus_anim = true;
        enable_samus_turn = true;
        enable_samus_standing = true;
        enable_samus_shooting = true;
        enable_samus_shooting_crouching = true;
        enable_samus_turning_crouching = true;
        enable_samus_starting_spin_jump = true;
        enable_samus_turning_midair = true;
    }
    const char* env_math = std::getenv("MZM_NATIVE_FIXED_MULT");
    if (env_math && (std::strcmp(env_math, "1") == 0 || std::strcmp(env_math, "true") == 0)) {
        enable_math = true;
    }
    const char* env_sr = std::getenv("MZM_NATIVE_SOFT_RESET");
    if (env_sr && (std::strcmp(env_sr, "1") == 0 || std::strcmp(env_sr, "true") == 0)) {
        enable_soft_reset = true;
    }
    const char* env_anim = std::getenv("MZM_NATIVE_SAMUS_ANIM");
    if (env_anim && (std::strcmp(env_anim, "1") == 0 || std::strcmp(env_anim, "true") == 0)) {
        enable_samus_anim = true;
    }
    const char* env_turn = std::getenv("MZM_NATIVE_SAMUS_TURN");
    if (env_turn && (std::strcmp(env_turn, "1") == 0 || std::strcmp(env_turn, "true") == 0)) {
        enable_samus_turn = true;
    }
    const char* env_standing = std::getenv("MZM_NATIVE_SAMUS_STANDING_GFX");
    if (!env_standing) {
        env_standing = std::getenv("MZM_NATIVE_SAMUS_STANDING");
    }
    if (env_standing && (std::strcmp(env_standing, "1") == 0 || std::strcmp(env_standing, "true") == 0)) {
        enable_samus_standing = true;
    }
    const char* env_shooting = std::getenv("MZM_NATIVE_SAMUS_SHOOTING_GFX");
    if (!env_shooting) {
        env_shooting = std::getenv("MZM_NATIVE_SAMUS_SHOOTING");
    }
    if (env_shooting && (std::strcmp(env_shooting, "1") == 0 || std::strcmp(env_shooting, "true") == 0)) {
        enable_samus_shooting = true;
    }
    const char* env_crouching = std::getenv("MZM_NATIVE_SAMUS_SHOOTING_CROUCHING_GFX");
    if (!env_crouching) {
        env_crouching = std::getenv("MZM_NATIVE_SAMUS_SHOOTING_CROUCHING");
    }
    if (env_crouching && (std::strcmp(env_crouching, "1") == 0 || std::strcmp(env_crouching, "true") == 0)) {
        enable_samus_shooting_crouching = true;
    }
    const char* env_turn_crouch = std::getenv("MZM_NATIVE_SAMUS_TURNING_CROUCHING_GFX");
    if (!env_turn_crouch) {
        env_turn_crouch = std::getenv("MZM_NATIVE_SAMUS_TURNING_CROUCHING");
    }
    if (env_turn_crouch && (std::strcmp(env_turn_crouch, "1") == 0 || std::strcmp(env_turn_crouch, "true") == 0)) {
        enable_samus_turning_crouching = true;
    }
    const char* env_starting_spin_jump = std::getenv("MZM_NATIVE_SAMUS_STARTING_SPIN_JUMP_GFX");
    if (!env_starting_spin_jump) {
        env_starting_spin_jump = std::getenv("MZM_NATIVE_SAMUS_STARTING_SPIN_JUMP");
    }
    if (env_starting_spin_jump && (std::strcmp(env_starting_spin_jump, "1") == 0 || std::strcmp(env_starting_spin_jump, "true") == 0)) {
        enable_samus_starting_spin_jump = true;
    }
    const char* env_turning_midair = std::getenv("MZM_NATIVE_SAMUS_TURNING_MIDAIR_GFX");
    if (!env_turning_midair) {
        env_turning_midair = std::getenv("MZM_NATIVE_SAMUS_TURNING_MIDAIR");
    }
    if (env_turning_midair && (std::strcmp(env_turning_midair, "1") == 0 || std::strcmp(env_turning_midair, "true") == 0)) {
        enable_samus_turning_midair = true;
    }

    // CLI flags override environment variables
    for (auto it = args.begin() + 1; it != args.end(); ) {
        if (*it == "--native-all") {
            enable_math = true;
            enable_soft_reset = true;
            enable_samus_anim = true;
            enable_samus_turn = true;
            enable_samus_standing = true;
            enable_samus_shooting = true;
            enable_samus_shooting_crouching = true;
            enable_samus_turning_crouching = true;
            enable_samus_starting_spin_jump = true;
            enable_samus_turning_midair = true;
            it = args.erase(it);
        } else if (*it == "--no-native-all") {
            enable_math = false;
            enable_soft_reset = false;
            enable_samus_anim = false;
            enable_samus_turn = false;
            enable_samus_standing = false;
            enable_samus_shooting = false;
            enable_samus_shooting_crouching = false;
            enable_samus_turning_crouching = false;
            enable_samus_starting_spin_jump = false;
            enable_samus_turning_midair = false;
            it = args.erase(it);
        } else if (*it == "--native-math" || *it == "--native-fixed-mult") {
            enable_math = true;
            it = args.erase(it);
        } else if (*it == "--no-native-math" || *it == "--disable-native-fixed-mult") {
            enable_math = false;
            it = args.erase(it);
        } else if (*it == "--native-soft-reset") {
            enable_soft_reset = true;
            it = args.erase(it);
        } else if (*it == "--no-native-soft-reset") {
            enable_soft_reset = false;
            it = args.erase(it);
        } else if (*it == "--native-samus-anim") {
            enable_samus_anim = true;
            it = args.erase(it);
        } else if (*it == "--no-native-samus-anim") {
            enable_samus_anim = false;
            it = args.erase(it);
        } else if (*it == "--native-samus-turn") {
            enable_samus_turn = true;
            it = args.erase(it);
        } else if (*it == "--no-native-samus-turn") {
            enable_samus_turn = false;
            it = args.erase(it);
        } else if (*it == "--native-samus-standing-gfx" || *it == "--native-samus-standing") {
            enable_samus_standing = true;
            it = args.erase(it);
        } else if (*it == "--no-native-samus-standing-gfx" || *it == "--no-native-samus-standing") {
            enable_samus_standing = false;
            it = args.erase(it);
        } else if (*it == "--native-samus-shooting-gfx" || *it == "--native-samus-shooting") {
            enable_samus_shooting = true;
            it = args.erase(it);
        } else if (*it == "--no-native-samus-shooting-gfx" || *it == "--no-native-samus-shooting") {
            enable_samus_shooting = false;
            it = args.erase(it);
        } else if (*it == "--native-samus-shooting-crouching-gfx" || *it == "--native-samus-shooting-crouching") {
            enable_samus_shooting_crouching = true;
            it = args.erase(it);
        } else if (*it == "--no-native-samus-shooting-crouching-gfx" || *it == "--no-native-samus-shooting-crouching") {
            enable_samus_shooting_crouching = false;
            it = args.erase(it);
        } else if (*it == "--native-samus-turning-crouching-gfx" || *it == "--native-samus-turning-crouching") {
            enable_samus_turning_crouching = true;
            it = args.erase(it);
        } else if (*it == "--no-native-samus-turning-crouching-gfx" || *it == "--no-native-samus-turning-crouching") {
            enable_samus_turning_crouching = false;
            it = args.erase(it);
        } else if (*it == "--native-samus-starting-spin-jump-gfx" || *it == "--native-samus-starting-spin-jump") {
            enable_samus_starting_spin_jump = true;
            it = args.erase(it);
        } else if (*it == "--no-native-samus-starting-spin-jump-gfx" || *it == "--no-native-samus-starting-spin-jump") {
            enable_samus_starting_spin_jump = false;
            it = args.erase(it);
        } else if (*it == "--native-samus-turning-midair-gfx" || *it == "--native-samus-turning-midair") {
            enable_samus_turning_midair = true;
            it = args.erase(it);
        } else if (*it == "--no-native-samus-turning-midair-gfx" || *it == "--no-native-samus-turning-midair") {
            enable_samus_turning_midair = false;
            it = args.erase(it);
        } else {
            ++it;
        }
    }

    mzm::native::set_function_enabled(mzm::native::NativeFunctionId::FixedMultiplication, enable_math);
    mzm::native::set_function_enabled(mzm::native::NativeFunctionId::SoftResetCheck, enable_soft_reset);
    mzm::native::set_function_enabled(mzm::native::NativeFunctionId::SamusUpdateAnimation, enable_samus_anim);
    mzm::native::set_function_enabled(mzm::native::NativeFunctionId::SamusTurningAroundGfx, enable_samus_turn);
    mzm::native::set_function_enabled(mzm::native::NativeFunctionId::SamusStandingGfx, enable_samus_standing);
    mzm::native::set_function_enabled(mzm::native::NativeFunctionId::SamusShootingGfx, enable_samus_shooting);
    mzm::native::set_function_enabled(mzm::native::NativeFunctionId::SamusShootingAndCrouchingGfx, enable_samus_shooting_crouching);
    mzm::native::set_function_enabled(mzm::native::NativeFunctionId::SamusTurningAroundAndCrouchingGfx, enable_samus_turning_crouching);
    mzm::native::set_function_enabled(mzm::native::NativeFunctionId::SamusStartingSpinJumpGfx, enable_samus_starting_spin_jump);
    mzm::native::set_function_enabled(mzm::native::NativeFunctionId::SamusTurningAroundMidAirGfx, enable_samus_turning_midair);
    mzm::native::sync_all_hooks();

    std::printf("[mzm:native] Native Pipeline Configuration: FixedMultiplication=%s, SoftResetCheck=%s, SamusUpdateAnimation=%s, SamusTurningAroundGfx=%s, SamusStandingGfx=%s, SamusShootingGfx=%s, SamusShootingAndCrouchingGfx=%s, SamusTurningAroundAndCrouchingGfx=%s, SamusStartingSpinJumpGfx=%s, SamusTurningAroundMidAirGfx=%s\n",
                enable_math ? "ON" : "OFF",
                enable_soft_reset ? "ON" : "OFF",
                enable_samus_anim ? "ON" : "OFF",
                enable_samus_turn ? "ON" : "OFF",
                enable_samus_standing ? "ON" : "OFF",
                enable_samus_shooting ? "ON" : "OFF",
                enable_samus_shooting_crouching ? "ON" : "OFF",
                enable_samus_turning_crouching ? "ON" : "OFF",
                enable_samus_starting_spin_jump ? "ON" : "OFF",
                enable_samus_turning_midair ? "ON" : "OFF");

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

    int ret = gbarecomp::run_game(static_cast<int>(native_args.size()), native_args.data(), options);

    mzm::native::print_session_summary();

    return ret;
}
