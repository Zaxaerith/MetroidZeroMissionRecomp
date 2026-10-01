#include "stack_tail_dispatch.h"
#include <stdexcept>
#include <cstdio>

int main() {
    mzm::StackTailDispatch dispatch;
    int failures=0,main_visits=0,irq_visits=0;
    auto check=[&](bool valid,const char* message) {
        if(!valid) { std::fprintf(stderr,"FAIL: %s\n",message);++failures; }
    };
    check(!dispatch.request(0,0,2),"no active continuation accepts a tail");
    dispatch.run(0,0,100,[&](unsigned pc) {
        ++main_visits;
        if(pc==100) {
            check(!dispatch.request(0,1,999),"IRQ cannot replace mainline continuation");
            dispatch.run(0,1,200,[&](unsigned irq_pc) {
                ++irq_visits;
                if(irq_pc==200) check(dispatch.request(0,1,202),"IRQ owns a separate tail");
                else check(irq_pc==202,"IRQ resumes its requested PC");
            });
            check(!dispatch.request(1,0,999),"another image cannot replace active body");
            try { dispatch.run(1,1,300,[](unsigned) { throw std::runtime_error("unwind"); }); }
            catch(const std::runtime_error&) {}
            check(dispatch.request(0,0,102),"nested exception restores parent continuation");
        } else check(pc==102,"parent resumes its own PC after IRQ returns");
    });
    check(main_visits==2 && irq_visits==2,"both continuations complete exactly once");
    check(dispatch.max_depth==2,"IRQ nesting is bounded independently of tail count");
    check(!dispatch.request(0,0,2),"completed contexts leave no dangling continuation");
    try { dispatch.run(0,0,400,[](unsigned) { throw std::runtime_error("outer unwind"); }); }
    catch(const std::runtime_error&) {}
    check(!dispatch.request(0,0,2),"outer exception clears active context");
    if(!failures) std::puts("stack tail continuation IRQ/image isolation and unwind PASS");
    return failures?1:0;
}
