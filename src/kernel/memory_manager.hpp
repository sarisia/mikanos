#pragma once

#include <cstdint>
#include <limits>

#include "error.hpp"
#include "memory_map.hpp"

namespace {
    constexpr unsigned long long operator ""_KiB(unsigned long long kib) {
        return kib * 1024;
    }

    constexpr unsigned long long operator ""_MiB(unsigned long long mib) {
        return mib * 1024_KiB;
    }

    constexpr unsigned long long operator ""_GiB(unsigned long long gib) {
        return gib * 1024_MiB;
    }
}

// physical memory frame size
static const auto kBytesPerFrame = 4_KiB;

class FrameID {
private:
    size_t id_;

public:
    explicit FrameID(size_t id): id_{id} {}
    size_t ID() const {
        return id_;
    }
    void *Frame() const {
        return reinterpret_cast<void *>(id_ * kBytesPerFrame);
    }
};

static const FrameID kNullFrame{std::numeric_limits<size_t>::max()};

class BitmapMemoryManager {
public:
    // max memory size this class can manage
    static const auto kMaxPhysicalMemoryBytes = 128_GiB;
    // max frames
    static const auto kFrameCount = kMaxPhysicalMemoryBytes / kBytesPerFrame;

    // bitmap type
    using MapLineType = unsigned long;
    // bits the MapLineType has
    static const size_t kBitsPerMapLine = 8 * sizeof(MapLineType);

    BitmapMemoryManager();

    WithError<FrameID> Allocate(size_t num_frames);
    Error Free(FrameID start_frame, size_t num_frames);
    void MarkAllocated(FrameID start_frame, size_t num_frames);
    void SetMemoryRange(FrameID range_begin, FrameID range_end);

private:
    std::array<MapLineType, kFrameCount/kBitsPerMapLine> alloc_map_;
    FrameID range_begin_;
    FrameID range_end_;

    bool getBit(FrameID frame) const;
    void setBit(FrameID frame, bool allocalted);
};

// allocates new heap, set up newlib_support:sbrk()
void InitializeMemoryManager(const MemoryMap &memory_map);
