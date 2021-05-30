#include <cstring>

#include "frame_buffer.hpp"


namespace {
    int bitsPerPixel(PixelFormat format) {
        switch (format) {
        case kPixelRGBResv8BitPerColor:
            return 32;
        case kPixelBGRResv8BitPerColor:
            return 32;
        }

        return -1;
    }

    int bytesPerPixel(PixelFormat format) {
        switch (format) {
        case kPixelRGBResv8BitPerColor:
            return 4;
        case kPixelBGRResv8BitPerColor:
            return 4;
        }

        return -1;
    }

    int bytesPerScanLine(const FrameBufferConfig &config) {
        return bytesPerPixel(config.pixel_format) * config.pixels_per_scan_line;
    }

    Vector2D<int> frameBufferSize(const FrameBufferConfig &config) {
        return {
            static_cast<int>(config.horizontal_resolution),
            static_cast<int>(config.vertical_resolution)
        };
    }

    uint8_t *frameAddrAt(Vector2D<int> pos, const FrameBufferConfig &config) {
        return config.frame_buffer + bytesPerPixel(config.pixel_format) *
            (pos.x + config.pixels_per_scan_line * pos.y);
    }
}


Error FrameBuffer::Initialize(const FrameBufferConfig &config) {
    config_ = config;
    const auto bits_per_pixel = bitsPerPixel(config_.pixel_format);
    if (bits_per_pixel <= 0) {
        return MAKE_ERROR(Error::kUnknownPixelFormat);
    }

    if (config_.frame_buffer) {
        buffer_.resize(0);
    } else {
        buffer_.resize(
            ((bits_per_pixel+7)/8) // 不足しないように
            * config_.horizontal_resolution * config_.vertical_resolution
        );
        config_.frame_buffer = buffer_.data();
        config_.pixels_per_scan_line = config_.horizontal_resolution;
    }

    switch (config_.pixel_format) {
    case kPixelRGBResv8BitPerColor:
        writer_ = std::make_unique<RGBResv8BitPerColorPixelWriter>(config_);
        break;
    case kPixelBGRResv8BitPerColor:
        writer_ = std::make_unique<BGRResv8BitPerColorPixelWriter>(config_);
        break;
    default:
        return MAKE_ERROR(Error::kUnknownPixelFormat);
    }

    return MAKE_ERROR(Error::kSuccess);
}

Error FrameBuffer::Copy(Vector2D<int> pos, const FrameBuffer &src) {
    if (config_.pixel_format != src.config_.pixel_format) {
        return MAKE_ERROR(Error::kUnknownPixelFormat);
    }

    const auto bytes_per_pixel = bytesPerPixel(config_.pixel_format);
    if (bytes_per_pixel <= 0) {
        return MAKE_ERROR(Error::kUnknownPixelFormat);
    }

    const auto dst_size = frameBufferSize(config_);
    const auto src_size = frameBufferSize(src.config_);

    const Vector2D<int> dst_start = ElementMax(pos, {0, 0});
    const Vector2D<int> dst_end = ElementMin(pos+src_size, dst_size);

    uint8_t *dst_buf = frameAddrAt(dst_start, config_);
    const uint8_t *src_buf = frameAddrAt({0, 0}, src.config_);

    for (int dy = dst_start.y; dy < dst_end.y; ++dy) {
        memcpy(dst_buf, src_buf, bytes_per_pixel * (dst_end.x - dst_start.x));
        dst_buf += bytesPerScanLine(config_);
        src_buf += bytesPerScanLine(src.config_);
    }

    return MAKE_ERROR(Error::kSuccess);
}

// pos は交差領域の絶対位置
// src_area.pos は pos からの相対位置
Error FrameBuffer::Copy(Vector2D<int> pos, const FrameBuffer &src, const Rectangle<int> &src_area) {
    if (config_.pixel_format != src.config_.pixel_format) {
        return MAKE_ERROR(Error::kUnknownPixelFormat);
    }

    const auto bytes_per_pixel = bytesPerPixel(config_.pixel_format);
    if (bytes_per_pixel <= 0) {
        return MAKE_ERROR(Error::kUnknownPixelFormat);
    }

    const Rectangle<int> src_area_shifted{pos, src_area.size};
    const Rectangle<int> src_outline{pos-src_area.pos, frameBufferSize(src.config_)}; // src_area.pos = pos - target_abs_pos
    const Rectangle<int> dst_outline{{0, 0}, frameBufferSize(config_)};
    const auto copy_area = dst_outline & src_outline & src_area_shifted;
    const auto src_start_pos = copy_area.pos - (pos - src_area.pos);

    uint8_t *dst_buf = frameAddrAt(copy_area.pos, config_);
    const uint8_t *src_buf = frameAddrAt(src_start_pos, src.config_);

    for (int dy = 0; dy < copy_area.size.y; ++dy) {
        memcpy(dst_buf, src_buf, bytes_per_pixel * copy_area.size.x);
        dst_buf += bytesPerScanLine(config_);
        src_buf += bytesPerScanLine(src.config_);
    }

    return MAKE_ERROR(Error::kSuccess);
}

void FrameBuffer::Move(Vector2D<int> dst_pos, const Rectangle<int> &src) {
    const auto bytes_per_pixel = bytesPerPixel(config_.pixel_format);
    const auto bytes_per_scan_line = bytesPerScanLine(config_);

    if (dst_pos.y < src.pos.y) { // move up
        uint8_t *dst_buf = frameAddrAt(dst_pos, config_);
        const uint8_t *src_buf = frameAddrAt(src.pos, config_);

        for (int y = 0; y < src.size.y; ++y) {
            memcpy(dst_buf, src_buf, bytes_per_pixel * src.size.x);
            dst_buf += bytes_per_scan_line;
            src_buf += bytes_per_scan_line;
        }
    } else { // move down
        // do from tail
        uint8_t *dst_buf = frameAddrAt(dst_pos + Vector2D<int>{0, src.size.y-1}, config_);
        const uint8_t *src_buf = frameAddrAt(src.pos + Vector2D<int>{0, src.size.y-1}, config_);

        for (int y = 0; y < src.size.y; ++y) {
            memcpy(dst_buf, src_buf, bytes_per_pixel * src.size.x);
            dst_buf -= bytes_per_scan_line;
            src_buf -= bytes_per_scan_line;
        }
    }
}
