#include "copied_code.h"
#include <algorithm>
#include <cstdio>
#include <vector>

int main() {
    std::vector<std::uint8_t> rom(mzm::kCollisionRomOffset + mzm::kCollisionImageSize);
    std::vector<std::uint8_t> ram(32 * 1024, 0);
    constexpr auto dest = mzm::kCollisionRam - 0x03000000;
    for (std::uint32_t i = 0; i < mzm::kCollisionImageSize; ++i)
        rom[mzm::kCollisionRomOffset + i] = static_cast<std::uint8_t>(i * 37 + 11);
    int failures = 0;
    auto check = [&](bool condition, const char* name) {
        if (!condition) { std::fprintf(stderr, "FAIL: %s\n", name); ++failures; }
    };
    auto reject = [&](std::uint32_t pc, bool thumb = true) {
        return mzm::collision_code_requires_interpreter(pc, thumb, rom, ram);
    };
    check(reject(mzm::kCollisionRam), "uninstalled image rejected");
    std::copy_n(rom.begin() + mzm::kCollisionRomOffset, mzm::kCollisionImageSize, ram.begin() + dest);
    check(!reject(mzm::kCollisionRam), "installed Thumb entry accepted");
    check(!reject(mzm::kCollisionRam + 12), "installed interior resume accepted");
    check(reject(mzm::kCollisionRam, false), "same bytes in ARM mode rejected");
    // Same entry prefix, but a changed tail/table must still invalidate AOT.
    ram[dest + mzm::kCollisionImageSize - 1] ^= 1;
    check(reject(mzm::kCollisionRam), "last-byte overwrite invalidates entry");
    check(reject(mzm::kCollisionRam + 12), "overwrite invalidates interior resume");
    ram[dest + mzm::kCollisionImageSize - 1] ^= 1;
    check(!reject(mzm::kCollisionRam + 12), "reinstalled image accepted without stale cache");
    check(!reject(mzm::kCollisionRam - 2), "address before image unaffected");
    check(!reject(mzm::kCollisionRam + mzm::kCollisionCodeSize), "neighbor code/data unaffected");
    check(mzm::collision_code_requires_interpreter(mzm::kCollisionRam, true, {}, ram), "missing ROM rejected");
    check(mzm::collision_code_requires_interpreter(mzm::kCollisionRam, true, rom, {}), "missing RAM rejected");
    rom.pop_back();
    check(reject(mzm::kCollisionRam), "truncated ROM rejected");
    constexpr auto stack_dest = mzm::kStackRam - 0x03000000;
    for (std::uint32_t i = 0; i < mzm::kStackImageSize; ++i)
        rom[mzm::kStackRomOffset + i] = static_cast<std::uint8_t>(i * 13 + 7);
    auto stack_reject = [&](std::uint32_t pc, bool thumb = true) {
        return mzm::stack_code_requires_interpreter(pc, thumb, rom, ram);
    };
    check(stack_reject(mzm::kStackRam), "uninstalled stack image rejected");
    std::copy_n(rom.begin() + mzm::kStackRomOffset, mzm::kStackImageSize, ram.begin() + stack_dest);
    check(!stack_reject(mzm::kStackRam), "installed stack entry accepted");
    check(!stack_reject(mzm::kStackRam + 24), "original stack interior accepted");
    // Install the same helper 24 bytes later, as observed at 03007D50.
    // Its entry overwrites an instruction of the first image at the same PC.
    std::copy_n(rom.begin() + mzm::kStackRomOffset, mzm::kStackImageSize, ram.begin() + stack_dest + 24);
    check(stack_reject(mzm::kStackRam), "overlapping copy invalidates original root");
    check(stack_reject(mzm::kStackRam + 24), "new root cannot use original interior alias");
    std::copy_n(rom.begin() + mzm::kStackRomOffset, mzm::kStackImageSize, ram.begin() + stack_dest);
    check(!stack_reject(mzm::kStackRam + 24), "reinstall restores original interior");
    check(stack_reject(mzm::kStackRam, false), "stack ARM mode rejected");
    check(!stack_reject(mzm::kStackRam + 48), "unmapped later stack address unaffected");
    constexpr auto read_dest = mzm::kStackReadRam - 0x03000000;
    for (std::uint32_t i = 0; i < mzm::kStackReadImageSize; ++i)
        rom[mzm::kStackReadRomOffset + i] = static_cast<std::uint8_t>(i * 17 + 3);
    auto read_reject = [&](std::uint32_t pc) {
        return mzm::stack_read_code_requires_interpreter(pc, true, rom, ram);
    };
    std::copy_n(rom.begin() + mzm::kStackReadRomOffset, mzm::kStackReadImageSize, ram.begin() + read_dest);
    check(!read_reject(mzm::kStackReadRam), "installed SRAM read image accepted");
    std::copy_n(rom.begin() + mzm::kStackReadRomOffset, mzm::kStackReadImageSize, ram.begin() + read_dest + 4);
    check(read_reject(mzm::kStackReadRam + 4), "read helper relocated over old interior is rejected");
    check(!read_reject(mzm::kStackReadRam + 36), "read span end excluded");
    // The dispatcher selects the complete installed placement, not the first
    // same-PC alias. These tests also exercise the shared overlapping tail.
    check(mzm::select_stack_image(0x03007D90, true, rom, ram) == 2, "replacement root selects new body");
    check(mzm::select_stack_image(0x03007DA6, true, rom, ram) == 2, "replacement interior selects new body");
    check(!mzm::stack_image_matches(mzm::kStackImages[1], rom, ram), "active old body must yield even when another image matches");
    check(mzm::select_stack_image(0x03007D8C, true, rom, ram) == -1, "invalidated first root excluded");
    check(mzm::select_stack_image(0x03007D90, false, rom, ram) == -1, "replacement ARM mode rejected");
    check(mzm::select_stack_image(0x03007D91, true, rom, ram) == -1, "odd resume PC rejected");
    check(mzm::select_stack_image(0x03007DB2, true, rom, ram) == -1, "replacement padding is not executable");
    check(mzm::select_stack_image(0x03007DB4, true, rom, ram) == -1, "replacement end excluded");
    check(mzm::select_stack_image(0x03007D90, true, {}, ram) == -1, "selector missing ROM rejected");
    check(mzm::select_stack_image(0x03007D90, true, rom, {}) == -1, "selector missing RAM rejected");
    ram[read_dest + 4 + mzm::kStackReadImageSize - 1] ^= 1;
    check(mzm::select_stack_image(0x03007DA6, true, rom, ram) == -1, "new image tail mutation invalidates interior");
    ram[read_dest + 4 + mzm::kStackReadImageSize - 1] ^= 1;
    std::copy_n(rom.begin() + mzm::kStackReadRomOffset, mzm::kStackReadImageSize, ram.begin() + read_dest);
    check(mzm::select_stack_image(0x03007D90, true, rom, ram) == 1, "reinstalled first image chooses old interior correctly");
    // The later 03007D08 install overlaps the old 03007D14 body. Its live
    // interior is allowed, but must never authorize an already-active old body.
    std::fill(ram.begin(),ram.end(),0);
    const auto old_image=mzm::kStackImages[5], new_image=mzm::kStackImages[6];
    std::copy_n(rom.begin()+old_image.source_offset,old_image.size,ram.begin()+old_image.runtime-0x03000000);
    check(mzm::select_stack_image(old_image.runtime,true,rom,ram)==5,"old comparison root selected");
    std::copy_n(rom.begin()+new_image.source_offset,new_image.size,ram.begin()+new_image.runtime-0x03000000);
    check(mzm::select_stack_image(old_image.runtime,true,rom,ram)==6,"new lower image interior selected after replacement");
    check(!mzm::stack_image_matches(old_image,rom,ram),"new matching image cannot authorize stale active comparison body");
    std::copy_n(rom.begin()+old_image.source_offset,old_image.size,ram.begin()+old_image.runtime-0x03000000);
    check(mzm::select_stack_image(old_image.runtime,true,rom,ram)==5,"old comparison reinstall selected");
    check(!mzm::stack_image_matches(new_image,rom,ram),"old reinstall invalidates lower comparison image");
    // Every observed placement must authorize only its complete live image,
    // including all executable interior resumes, never its padding or odd PCs.
    for (int i=0;i<static_cast<int>(mzm::kStackImages.size());++i) {
        const auto image=mzm::kStackImages[i];
        std::fill(ram.begin(),ram.end(),0);
        const auto offset=image.runtime-0x03000000;
        std::copy_n(rom.begin()+image.source_offset,image.size,ram.begin()+offset);
        for (unsigned pc=image.runtime;pc<image.runtime+image.size-2;pc+=2)
            check(mzm::select_stack_image(pc,true,rom,ram)==i,"installed image selects every interior resume");
        check(mzm::select_stack_image(image.runtime+image.size-2,true,rom,ram)==-1,"placement padding excluded");
        check(mzm::select_stack_image(image.runtime+1,true,rom,ram)==-1,"placement odd resume rejected");
        check(mzm::select_stack_image(image.runtime,false,rom,ram)==-1,"placement wrong mode rejected");
        ram[offset+image.size-1]^=1;
        check(mzm::select_stack_image(image.runtime,true,rom,ram)==-1,"placement mutated tail rejects root");
        check(mzm::select_stack_image(image.runtime+2,true,rom,ram)==-1,"placement mutated tail rejects interior");
    }
    rom.resize(mzm::kHazeRomOffset+mzm::kHazeImageSize);
    for (unsigned i=0;i<mzm::kHazeImageSize;++i) rom[mzm::kHazeRomOffset+i]=static_cast<uint8_t>(i*19+5);
    std::fill(ram.begin(),ram.end(),0);
    const auto haze_offset=mzm::kHazeRam-0x03000000;
    auto haze_reject=[&](uint32_t pc,bool thumb=true) {return mzm::haze_code_requires_interpreter(pc,thumb,rom,ram);};
    check(haze_reject(mzm::kHazeRam),"uninstalled haze rejected");
    std::copy_n(rom.begin()+mzm::kHazeRomOffset,mzm::kHazeImageSize,ram.begin()+haze_offset);
    check(!haze_reject(mzm::kHazeRam),"complete haze root accepted");
    check(!haze_reject(mzm::kHazeRam+0xB6),"haze final BX accepted");
    check(haze_reject(mzm::kHazeRam,false),"haze ARM mode rejected");
    check(haze_reject(mzm::kHazeRam+1),"haze odd PC rejected");
    ram[haze_offset+mzm::kHazeImageSize-1]^=1;
    check(haze_reject(mzm::kHazeRam),"haze tail mutation invalidates root");
    check(haze_reject(mzm::kHazeRam+0x40),"haze tail mutation invalidates interior");
    ram[haze_offset+mzm::kHazeImageSize-1]^=1;
    check(!haze_reject(mzm::kHazeRam+0x40),"haze reinstall accepted without cached identity");
    check(!haze_reject(mzm::kHazeRam+mzm::kHazeCodeSize),"haze literals and neighbors excluded");
    check(mzm::haze_code_requires_interpreter(mzm::kHazeRam,true,{},ram),"missing haze ROM rejected");
    check(mzm::haze_code_requires_interpreter(mzm::kHazeRam,true,rom,{}),"missing haze RAM rejected");
    rom.pop_back();check(haze_reject(mzm::kHazeRam),"truncated haze ROM rejected");
    if (!failures) std::puts("copied code image guard PASS (synthetic bytes; no ROM/BIOS required)");
    return failures ? 1 : 0;
}
