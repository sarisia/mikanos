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

    const auto bits_per_pixel = bitsPerPixel(config_.pixel_format);
    if (bits_per_pixel <= 0) {
        return MAKE_ERROR(Error::kUnknownPixelFormat);
    }

    const auto dst_width = config_.horizontal_resolution;
    const auto dst_height = config_.vertical_resolution;
    const auto src_width = src.config_.horizontal_resolution;
    const auto src_height = src.config_.vertical_resolution;

    const int copy_start_dst_x = std::max(pos.x, 0);
    const int copy_start_dst_y = std::max(pos.y, 0);
    const int copy_end_dst_x = std::min(pos.x+src_width, dst_width);
    const int copy_end_dst_y = std::min(pos.y+src_height, dst_height);

    const auto bytes_per_pixel = (bits_per_pixel+7) / 8; //align 8
    const auto bytes_per_copy_line = bytes_per_pixel * (copy_end_dst_x-copy_start_dst_x);

    uint8_t *dst_buf = config_.frame_buffer +
        bytes_per_pixel * (copy_start_dst_x + config_.pixels_per_scan_line*copy_start_dst_y);
    const uint8_t *src_buf = src.config_.frame_buffer;

    for (int dy = 0; dy < copy_end_dst_y; ++dy) {
        memcpy(dst_buf, src_buf, bytes_per_copy_line);
        dst_buf += bytes_per_pixel * config_.pixels_per_scan_line;
        src_buf += bytes_per_pixel * src.config_.pixels_per_scan_line;
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
