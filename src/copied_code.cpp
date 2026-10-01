#include "copied_code.h"
#include "stack_tail_dispatch.h"
#include "gba_bus.h"
#include "runtime_arm.h"
#include "runtime_bus_bridge.h"
#include <cstdlib>

extern "C" uint32_t g_irq_nest_depth;
namespace mzm {
int copied_code_interpreter_guard(unsigned pc,int thumb);
int copied_code_ram_dispatch(unsigned pc,int thumb);
}
namespace {
mzm::StackTailDispatch tails;
int active_image=-1;
unsigned active_irq_depth=0;
bool stack_pc(uint32_t pc) {
    for(const auto& image:mzm::kStackImages) if(pc-image.runtime<image.size) return true;
    return false;
}
}
int mzm::copied_code_interpreter_guard(unsigned pc,int thumb) {
    if(pc-kCollisionRam>=kCollisionCodeSize && pc-kHazeRam>=kHazeCodeSize && !stack_pc(pc)) return 0;
    auto* bus=gbarecomp::active_bus();
    bool reject=!bus;
    if(bus) {
        const std::span<const uint8_t> rom(bus->rom_ptr(),bus->rom_size()),ram(bus->iwram_ptr(),32768);
        if(pc-kCollisionRam<kCollisionCodeSize)
            reject=collision_code_requires_interpreter(pc,thumb!=0,rom,ram);
        else if(pc-kHazeRam<kHazeCodeSize)
            reject=haze_code_requires_interpreter(pc,thumb!=0,rom,ram);
        else if(active_image>=0 && active_irq_depth==g_irq_nest_depth && pc-kStackImages[active_image].runtime<kStackImages[active_image].size)
            reject=!thumb || (pc&1u) || !stack_image_matches(kStackImages[active_image],rom,ram);
        else reject=select_stack_image(pc,thumb!=0,rom,ram)<0;
    }
    if(!reject) return 0;
    static bool strict=[] { const char* e=std::getenv("GBARECOMP_STRICT_STATIC");return e && *e && *e!='0'; }();
    if(strict) runtime_dispatch_miss(pc);
    return 1;
}
int mzm::copied_code_ram_dispatch(unsigned pc,int thumb) {
    if(!stack_pc(pc)) return 0;
    auto* bus=gbarecomp::active_bus();
    if(!bus) return 0;
    int image=select_stack_image(pc,thumb!=0,{bus->rom_ptr(),bus->rom_size()},{bus->iwram_ptr(),32768});
    if(image<0) return 0;
    if(tails.request(image,g_irq_nest_depth,pc)) return 1;
    struct Scope {
        int previous=active_image;
        unsigned irq=active_irq_depth;
        Scope(int image) { active_image=image;active_irq_depth=g_irq_nest_depth; }
        ~Scope() { active_image=previous;active_irq_depth=irq; }
    } scope(image);
    // Match the previous whole-subtree bridge's headless stopping boundary.
    // This changes scheduler granularity, never guest instruction timing.
    gbarecomp::DeferHeadlessFrameYield frame_boundary;
    constexpr void(*bodies[])()={stack_variant_0,stack_variant_1,stack_variant_2,stack_variant_3,stack_variant_4,stack_variant_5,stack_variant_6};
    static_assert(std::size(bodies)==kStackImages.size());
    tails.run(image,g_irq_nest_depth,pc,[&](uint32_t next) {
        g_runtime_resume_pc=next==kStackImages[image].runtime?0u:next;
        bodies[image]();
    });
    return 1;
}
