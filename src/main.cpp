#include <cstdio>
#include <cstring>
#include "runtime.h"
#include "runtime_arm.h"
#include "copied_code.h"

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            std::puts("MetroidZeroMissionRecomp --rom <USA ROM> --bios <GBA BIOS> [game.toml]\n"
                      "Validation: --no-window --frames <count>\n"
                      "Faithful 240x160 presentation; cartridge and BIOS hashes are verified.");
            return 0;
        }
    }
    gbarecomp::RunOptions options;
    options.builtin_game_name = "Metroid: Zero Mission";
    options.builtin_rom_sha1 = "5de8536afe1f0078ee6fe1089f890e8c7aa0a6e8";
    g_runtime_force_interp_hook = mzm::copied_code_interpreter_guard;
    g_runtime_ram_dispatch_hook = mzm::copied_code_ram_dispatch;
    return gbarecomp::run_game(argc, argv, options);
}
