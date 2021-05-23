#pragma once

#include <vector>
#include <cstdint>
#include <memory>

#include "frame_buffer_config.hpp"
#include "error.hpp"
#include "graphics.hpp"

class FrameBuffer {
private:
    FrameBufferConfig config_{};
    std::vector<uint8_t> buffer_{};
    std::unique_ptr<FrameBufferWriter> writer_{};

public:
    Error Initialize(const FrameBufferConfig &config);
    Error Copy(Vector2D<int> pos, const FrameBuffer &src);
    void Move(Vector2D<int> dst_pos, const Rectangle<int> &src);

    FrameBufferWriter &Writer() {
        return *writer_;
    }
};
