#include "runtime_arm.h"
#include "runtime_bus_bridge.h"
#include <cstdio>
#include <functional>

void runtime_set_frame_present_hook(std::function<bool()> hook);

int main() {
    int failures=0;
    auto check=[&](bool ok,const char* message) {
        if(!ok) { std::fprintf(stderr,"FAIL: %s\n",message);++failures; }
    };
    g_cpu.cpsr=0x1F;
    g_cpu.R[15]=0x08000100;
    g_runtime_vblank_starts=1;
    check(runtime_should_yield(),"ordinary frame boundary yields");
    g_runtime_vblank_starts=2;
    {
        gbarecomp::DeferHeadlessFrameYield outer;
        check(!runtime_should_yield(),"native subtree defers headless frame stop");
        { gbarecomp::DeferHeadlessFrameYield inner;
          check(!runtime_should_yield(),"nested subtree still defers"); }
        check(!runtime_should_yield(),"inner return preserves outer scope");
    }
    check(runtime_should_yield(),"pending frame yields after subtree returns");
    check(!runtime_should_yield(),"pending frame notification consumed once");
    try { gbarecomp::DeferHeadlessFrameYield unwind;throw 1; } catch(int) {}
    g_runtime_vblank_starts=3;
    check(runtime_should_yield(),"exception restores ordinary frame stopping");
    const auto cycles=g_runtime_cycles;
    const auto cpu=g_cpu;
    int presents=0;
    runtime_set_frame_present_hook([&] { ++presents;return false; });
    {
        gbarecomp::DeferHeadlessFrameYield scope;
        g_runtime_vblank_starts=4;
        check(!runtime_should_yield() && presents==1,"present hook still runs inside scope");
        g_runtime_break_pc=g_cpu.R[15];
        check(runtime_should_yield(),"debug breakpoint still unwinds inside scope");
        g_runtime_break_pc=0;
    }
    runtime_set_frame_present_hook([] { return true; });
    {
        gbarecomp::DeferHeadlessFrameYield scope;
        g_runtime_vblank_starts=5;
        check(runtime_should_yield(),"present hook quit still unwinds inside scope");
        check(runtime_should_yield(),"present hook quit remains sticky");
    }
    runtime_set_frame_present_hook({});
    check(g_runtime_cycles==cycles && g_cpu.cpsr==cpu.cpsr &&
          g_cpu.R[15]==cpu.R[15],"scheduler scope preserves guest timing and PC");
    if(!failures) std::puts("scoped headless frame boundary PASS");
    return failures?1:0;
}
