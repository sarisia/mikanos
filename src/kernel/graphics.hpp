#pragma once

#include <algorithm>

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

template <typename T, typename U>
auto operator +(const Vector2D<T> &lhs, const Vector2D<U> &rhs) -> Vector2D<decltype(lhs.x+rhs.x)> {
    return {lhs.x+rhs.x, lhs.y+rhs.y};
}

template <typename T, typename U>
auto operator -(const Vector2D<T> &lhs, const Vector2D<U> &rhs) -> Vector2D<decltype(lhs.x+rhs.x)> {
    return {lhs.x-rhs.x, lhs.y-rhs.y};
}

template <typename T>
Vector2D<T> ElementMax(const Vector2D<T> &lhs, const Vector2D<T> &rhs) {
    return {std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y)};
}

template <typename T>
Vector2D<T> ElementMin(const Vector2D<T> &lhs, const Vector2D<T> &rhs) {
    return {std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y)};
}


template <typename T>
struct Rectangle {
    Vector2D<T> pos, size;
};

template <typename T, typename U>
Rectangle<T> operator &(const Rectangle<T> &lhs, const Rectangle<U> &rhs) {
    const auto lhs_end = lhs.pos + lhs.size;
    const auto rhs_end = rhs.pos + rhs.size;
    if (lhs_end.x < rhs.pos.x || lhs_end.y < rhs.pos.y ||
        rhs_end.x < lhs.pos.x || rhs_end.y < lhs.pos.y) {
        return {{0, 0}, {0, 0}};
    }

    auto new_pos = ElementMax(lhs.pos, rhs.pos);
    auto new_size = ElementMin(lhs_end, rhs_end) - new_pos;
    return {new_pos, new_size};
}


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
    virtual void Write(Vector2D<int> pos, const PixelColor &color) = 0;
    virtual void Write(int x, int y, const PixelColor& c) {
        return Write({x, y}, c);
    }
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
    uint8_t* PixelAt(Vector2D<int> pos) {
        return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * pos.y + pos.x);
    }
};

class RGBResv8BitPerColorPixelWriter : public FrameBufferWriter {
public:
    using FrameBufferWriter::FrameBufferWriter;

    void Write(int x, int y, const PixelColor& c) override;
    void Write(Vector2D<int> pos, const PixelColor& c) override;
};

class BGRResv8BitPerColorPixelWriter : public FrameBufferWriter {
public:
    using FrameBufferWriter::FrameBufferWriter;

    void Write(int x, int y, const PixelColor& c) override;
    void Write(Vector2D<int> pos, const PixelColor& c) override;
};


void FillRectangle(PixelWriter& writer, const Vector2D<int>& pos,
                   const Vector2D<int>& size, const PixelColor& color);
void DrawRectangle(PixelWriter& writer, const Vector2D<int>& pos,
                   const Vector2D<int>& size, const PixelColor& color);

const PixelColor kDesktopBGColor{ 58, 110, 165 };
const PixelColor kDesktopFGColor{ 255, 255, 255 };

void DrawDesktop(PixelWriter &writer);

extern FrameBufferConfig screen_config;
extern PixelWriter* screen_writer;

Vector2D<int> ScreenSize();
void InitializeGraphics(const FrameBufferConfig &screen_config);

constexpr PixelColor toColor(uint32_t color) {
    return {
        static_cast<uint8_t>((color >> 16) & 0xff),
        static_cast<uint8_t>((color >> 8) & 0xff),
        static_cast<uint8_t>(color & 0xff)
    };
}
