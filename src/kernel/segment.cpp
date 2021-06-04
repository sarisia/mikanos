#include "segment.hpp"
#include "asmfunc.h"

namespace {
    // Global Descriptor Table
    std::array<SegmentDescriptor, 3> gdt;

    void setCodeSegment(
        SegmentDescriptor &desc,
        DescriptorType type,
        unsigned int descriptor_privilege_level,
        uint32_t base,
        uint32_t limit
    ) {
        desc.data = 0;

        // base and limit are ignored in 64bit mode (long_mode=1)
        desc.bits.base_low = base & 0xffffu;
        desc.bits.base_middle = (base >> 16) & 0xffu;
        desc.bits.base_high = (base >> 24) & 0xffu;

        desc.bits.limit_low = limit & 0xffffu;
        desc.bits.limit_high = (limit >> 16) & 0xfu;

        desc.bits.type = type;
        desc.bits.system_segment = 1; // code & data segment (1)
        desc.bits.descriptor_privilege_level = descriptor_privilege_level;
        desc.bits.present = 1;
        desc.bits.available = 0;
        desc.bits.long_mode = 1;
        desc.bits.default_operation_size = 0; // 0 when long_mode == 1
        desc.bits.granualarity = 1; // limit unit is 4KiB
    }

    void setDataSegment(
        SegmentDescriptor &desc,
        DescriptorType type,
        unsigned int descriptor_privilege_type,
        uint32_t base,
        uint32_t limit
    ) {
        setCodeSegment(desc, type, descriptor_privilege_type, base, limit);

        desc.bits.long_mode = 0;
        desc.bits.default_operation_size = 1;
    }
}

void SetupSegments() {
    gdt[0].data = 0; // unused
    setCodeSegment(gdt[1], DescriptorType::kExecuteRead, 0, 0, 0xfffff);
    setDataSegment(gdt[2], DescriptorType::kReadWrite, 0, 0, 0xfffff);
    LoadGDT(sizeof(gdt)-1, reinterpret_cast<uintptr_t>(&gdt[0]));
}

void InitializeSegmentation() {
    SetupSegments();
    SetDSAll(kKernelDS);
    SetCSSS(kKernelCS, kKernelSS);
}
