#pragma once
#include <cstdint>

namespace mzm {
// Whole-body local B edges become continuation requests, not nested host calls.
// The caller still verifies image identity and publishes the real guest PC.
// IRQ depth prevents a reentrant interrupt call from hijacking its mainline.
class StackTailDispatch {
    struct Frame {
        int image;
        unsigned irq_depth;
        std::uint32_t next_pc;
        bool pending = false;
    };
    Frame* active_ = nullptr;
    unsigned depth_ = 0;
public:
    unsigned max_depth = 0;
    unsigned long long tails = 0;

    bool request(int image, unsigned irq_depth, std::uint32_t pc) {
        if (!active_ || active_->image != image || active_->irq_depth != irq_depth)
            return false;
        active_->next_pc = pc;
        active_->pending = true;
        ++tails;
        return true;
    }
    template<class Body>
    void run(int image, unsigned irq_depth, std::uint32_t pc, Body&& body) {
        Frame frame{image, irq_depth, pc};
        struct Scope {
            StackTailDispatch& owner;
            Frame* previous;
            Scope(StackTailDispatch& dispatcher, Frame& frame)
                : owner(dispatcher), previous(dispatcher.active_) {
                owner.active_ = &frame;
                if (++owner.depth_ > owner.max_depth) owner.max_depth = owner.depth_;
            }
            ~Scope() { owner.active_ = previous; --owner.depth_; }
        } scope(*this, frame);
        do {
            pc = frame.next_pc;
            frame.pending = false;
            body(pc);
        } while (frame.pending);
    }
};
} // namespace mzm
