#include "interrupt.hpp"
#include "segment.hpp"
#include "asmfunc.h"
#include "timer.hpp"
#include "task.hpp"

std::array<InterruptDescriptor, 256> idt;

void SetIDTEntry(InterruptDescriptor& desc,
                 InterruptDescriptorAttribute attr,
                 uint64_t offset,
                 uint16_t segment_selector) {
    desc.attr = attr;
    desc.offset_low = offset & 0xffffu;
    desc.offset_middle = (offset >> 16) & 0xffffu;
    desc.offset_high = offset >> 32;
    desc.segment_selector = segment_selector;
}

void NotifyEndOfInterrupt() {
    volatile auto end_of_interrupt = reinterpret_cast<uint32_t*>(0xfee000b0);
    *end_of_interrupt = 0;
}


namespace {
    __attribute__((interrupt))
    void intHandlerXHCI(InterruptFrame* frame) {
        task_manager->SendMessage(1, Message{Message::kInterruptXHCI});
        NotifyEndOfInterrupt();
    }

    __attribute__((interrupt))
    void intHandlerLAPICTimer(InterruptFrame *frame) {
        LAPICTimerOnInterrupt();
    }
}

void InitializeInterrupt() {
    // USB
    SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
        reinterpret_cast<uint64_t>(intHandlerXHCI), kKernelCS);
    // Timer
    SetIDTEntry(idt[InterruptVector::kLAPICTimer], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
        reinterpret_cast<uint64_t>(intHandlerLAPICTimer), kKernelCS);
    LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));
}
