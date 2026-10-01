// Differential qualification of complete bounded bodies, not full-game fidelity.
#include "copied_code.h"
#include "runtime_arm.h"
#include "interpreter.h"
#include "thumb_decode.h"
#include "stubs.h"
#include <fstream>
#include <iterator>
#include <vector>
#include <cstring>
#include <iostream>
#include <algorithm>
#include "sha1.h"
#include "stack_tail_dispatch.h"

struct Memory : armv4t::Bus {
    std::vector<uint8_t> bytes;
    static constexpr uint32_t base=0x03000000;
    Memory():bytes(0x30000){}
    uint8_t read8(uint32_t a) override { return bytes.at(a-base); }
    uint16_t read16(uint32_t a) override { return read8(a)|(read8(a+1)<<8); }
    uint32_t read32(uint32_t a) override { return read16(a)|(uint32_t(read16(a+2))<<16); }
    void write8(uint32_t a,uint8_t v) override { bytes.at(a-base)=v; }
    void write16(uint32_t a,uint16_t v) override { write8(a,v);write8(a+1,v>>8); }
    void write32(uint32_t a,uint32_t v) override { write16(a,v);write16(a+2,v>>16); }
};
uint32_t pack(const armv4t::CPSR& c) {
    return c.mode|(uint32_t(c.t)<<5)|(uint32_t(c.f)<<6)|(uint32_t(c.i)<<7)|
        (uint32_t(c.v)<<28)|(uint32_t(c.c)<<29)|(uint32_t(c.z)<<30)|(uint32_t(c.n)<<31);
}
int tested_variant=0;
mzm::StackTailDispatch tail_dispatch;
void run_variant(uint32_t pc) {
    constexpr void(*bodies[])()={mzm::stack_variant_0,mzm::stack_variant_1,mzm::stack_variant_2,mzm::stack_variant_3,mzm::stack_variant_4,mzm::stack_variant_5,mzm::stack_variant_6};
    static_assert(std::size(bodies)==mzm::kStackImages.size());
    tail_dispatch.run(tested_variant,0,pc,[&](uint32_t next) {
        g_runtime_resume_pc=next==mzm::kStackImages[tested_variant].runtime?0u:next;
        bodies[tested_variant]();
    });
}
int resume_variant(uint32_t pc,int thumb) {
    const auto image=mzm::kStackImages[tested_variant];
    if(!thumb || pc-image.runtime>=image.size-2) return 0;
    if(!tail_dispatch.request(tested_variant,0,pc)) run_variant(pc);
    return 1;
}
int main(int argc,char** argv) {
    if(argc!=2 && argc!=3) return 2;
    const bool stress=argc==3 && std::string(argv[2])=="--stress-compare";
    std::cout.setf(std::ios::unitbuf);
    std::ifstream input(argv[1],std::ios::binary);
    std::vector<uint8_t> rom((std::istreambuf_iterator<char>(input)),{});
    if(gba::sha1(rom.data(),rom.size()).hex()!="5de8536afe1f0078ee6fe1089f890e8c7aa0a6e8") return 2;
    int failures=0;
    constexpr void(*bodies[])()={mzm::stack_variant_0,mzm::stack_variant_1,mzm::stack_variant_2,mzm::stack_variant_3,mzm::stack_variant_4,mzm::stack_variant_5,mzm::stack_variant_6};
    static_assert(std::size(bodies)==mzm::kStackImages.size());
    g_runtime_ram_dispatch_hook=resume_variant;
    for(int variant=0;variant<static_cast<int>(mzm::kStackImages.size());++variant) for(uint32_t count:{1u,16u,32768u}) for(bool equal:{false,true}) {
        const bool stress_case=variant==0 && count==32768 && equal;
        if(stress && !stress_case) continue;
        tested_variant=variant;
        auto image=mzm::kStackImages[variant];
        Memory bus;
        std::copy_n(rom.begin()+image.source_offset,image.size,bus.bytes.begin()+image.runtime-Memory::base);
        for(uint32_t i=0;i<count;++i) bus.write8(0x03020000+i,uint8_t(i*31+7));
        if(equal) for(uint32_t i=0;i<count;++i) bus.write8(0x03010000+i,bus.read8(0x03020000+i));
        armv4t::CPUState cpu{};
        cpu.cpsr.mode=0x1F;cpu.cpsr.t=true;cpu.thumb=true;
        cpu.R[0]=0x03020000;cpu.R[1]=0x03010000;cpu.R[2]=count;
        cpu.R[13]=0x0300F000;cpu.R[14]=0x08001001;cpu.R[15]=image.runtime;
        codegen_test::bus_reset(Memory::base,bus.bytes.size());
        for(uint32_t i=0;i<bus.bytes.size();i+=4) codegen_test::bus_write_u32_direct(Memory::base+i,bus.read32(Memory::base+i));
        std::memset(&g_cpu,0,sizeof(g_cpu));
        std::copy_n(cpu.R,16,g_cpu.R);g_cpu.cpsr=pack(cpu.cpsr);
        uint64_t cycles=0;
        while(cpu.R[15]!=0x08001000 && cycles<2000000) {
            auto ins=armv4t::ThumbDecoder::decode(bus.read16(cpu.R[15]),cpu.R[15]);
            uint32_t step_cycles=0;
            auto result=armv4t::Interpreter::step(cpu,bus,ins,&step_cycles);
            if(result==armv4t::Interpreter::Result::Undefined || result==armv4t::Interpreter::Result::NotImplemented) return 3;
            cycles+=step_cycles;
        }
        runtime_call_stack_restore(nullptr,0);runtime_call_push_return(0x08001000);g_runtime_resume_pc=0;
        tail_dispatch=mzm::StackTailDispatch{};
        run_variant(image.runtime);
        bool match=std::equal(cpu.R,cpu.R+16,g_cpu.R) && pack(cpu.cpsr)==g_cpu.cpsr &&
            std::equal(bus.bytes.begin(),bus.bytes.end(),codegen_test::bus_data()) &&
            cycles==codegen_test::g_ticked_cycles && !codegen_test::bus_oob_seen() && tail_dispatch.max_depth==1;
        std::cout<<"variant="<<variant<<" count="<<count<<" equal="<<equal<<" cycles="<<cycles<<"/"<<codegen_test::g_ticked_cycles<<" depth="<<tail_dispatch.max_depth<<" tails="<<tail_dispatch.tails<<" "<<(match?"PASS":"FAIL")<<"\n";
        if(!match) { ++failures;for(int r=0;r<16;++r) if(cpu.R[r]!=g_cpu.R[r]) std::cout<<"R"<<r<<" "<<std::hex<<cpu.R[r]<<"/"<<g_cpu.R[r]<<std::dec<<"\n"; }
    }
    return failures?1:0;
}
