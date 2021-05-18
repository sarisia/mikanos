#pragma once

#include "frame_buffer_config.hpp"

template <typename T>
struct Vector2D {
    T x, y;

    template <typename U>
    Vector2D<T>& operator +=(const Vector2D<U>& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
};

struct PixelColor {
    uint8_t r, g, b;
};

inline bool operator ==(const PixelColor &lhs, const PixelColor &rhs) {
    return lhs.r == rhs.r
        && lhs.g == rhs.g
        && lhs.b == rhs.b;
}

inline bool operator !=(const PixelColor &lhs, const PixelColor &rhs) {
    return !(lhs == rhs);
}

// PixelWriter Interface
class PixelWriter {
public:
    virtual ~PixelWriter() = default;
    virtual void Write(int x, int y, const PixelColor& c) = 0;
    virtual int Width() const = 0;
    virtual int Height() const = 0;
};

class FrameBufferWriter : public PixelWriter {
private:
    const FrameBufferConfig& config_;

public:
    FrameBufferWriter(const FrameBufferConfig &config)
        : config_{config} {}
    
    ~FrameBufferWriter() override = default;
    int Width() const override {
        return config_.horizontal_resolution;
    }
    int Height() const override {
        return config_.vertical_resolution;
    }    

protected:
    uint8_t* PixelAt(int x, int y) {
        return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * y + x);
    }
};

class RGBResv8BitPerColorPixelWriter : public FrameBufferWriter {
public:
    using FrameBufferWriter::FrameBufferWriter;

    void Write(int x, int y, const PixelColor& c) override;
};

class BGRResv8BitPerColorPixelWriter : public FrameBufferWriter {
public:
    using FrameBufferWriter::FrameBufferWriter;

    void Write(int x, int y, const PixelColor& c) override;
};

void FillRectangle(PixelWriter& writer, const Vector2D<int>& pos,
                   const Vector2D<int>& size, const PixelColor& color);
void DrawRectangle(PixelWriter& writer, const Vector2D<int>& pos,
                   const Vector2D<int>& size, const PixelColor& color);

const PixelColor kDesktopBGColor{ 58, 110, 165 };
const PixelColor kDesktopFGColor{ 255, 255, 255 };

void DrawDesktop(PixelWriter &writer);
