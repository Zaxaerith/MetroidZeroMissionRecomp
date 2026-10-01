// Emit qualified moving-stack placements using the pinned framework emitter.
// Output contains ROM-derived code and belongs only in ignored generated/.
#include "copied_code.h"
#include "emit_function.h"
#include "thumb_decode.h"
#include "sha1.h"
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("Expected ROM and generated output paths");
        std::ifstream input(argv[1], std::ios::binary);
        std::vector<uint8_t> rom((std::istreambuf_iterator<char>(input)), {});
        if (gba::sha1(rom.data(), rom.size()).hex() != "5de8536afe1f0078ee6fe1089f890e8c7aa0a6e8")
            throw std::runtime_error("ROM identity mismatch");
        std::string output = "// Generated from verified local ROM; DO NOT EDIT OR DISTRIBUTE.\n#include \"runtime_arm.h\"\n#include \"copied_code.h\"\nnamespace mzm {\n";
        for (std::size_t i = 0; i < mzm::kStackImages.size(); ++i) {
            auto image = mzm::kStackImages[i];
            // Qualify the terminal BX and excluded alignment padding.
            auto hw = [&](uint32_t offset) { return uint16_t(rom.at(offset) | (rom.at(offset+1)<<8)); };
            if ((hw(image.source_offset+image.size-4) & 0xFF87) != 0x4700 ||
                hw(image.source_offset+image.size-2) != 0)
                throw std::runtime_error("Verified epilogue/padding mismatch");
            gbarecomp::Function function{};
            function.addr = image.runtime;
            function.source_addr = 0x08000000 + image.source_offset;
            function.mode = gbarecomp::CpuMode::Thumb;
            function.end_addr = image.runtime + image.size - 2;
            function.walk_end_addr = function.end_addr;
            function.name = "stack_variant_" + std::to_string(i);
            for (uint32_t pc=image.runtime; pc<function.end_addr; pc+=2) {
                auto ins = armv4t::ThumbDecoder::decode(hw(image.source_offset+pc-image.runtime),pc);
                if (ins.is_undefined) throw std::runtime_error("Undefined instruction in bounded image");
                if (pc != image.runtime) function.alias_entries.push_back(pc);
            }
            // No cross-image direct C calls; local branches remain local in
            // the complete bounded body, and every instruction is resumable.
            output += "void " + function.name + "() {\n";
            output += gbarecomp::emit_function_body_str(function, rom.data(), rom.size(), 0x08000000, {});
            output += "}\n";
        }
        output += "}\n";
        std::ofstream file(argv[2],std::ios::binary);
        file << output;
        if (!file) throw std::runtime_error("Failed writing generated variants");
        std::cout << "Generated " << mzm::kStackImages.size() << " bounded stack variants\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
