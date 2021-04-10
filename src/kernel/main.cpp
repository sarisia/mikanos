#include <cstdint>

#include "frame_buffer_config.hpp"

struct PixelColor {
    uint8_t r, g, b;
};

int WritePixel(
    const FrameBufferConfig &config,
    int x,
    int y,
    const PixelColor &pcolor
) {
    const int pixel_pos = config.pixels_per_scan_line*y + x;
    uint8_t *pixel = &config.frame_buffer[pixel_pos*4];

    if (config.pixel_format == kPixelRGBResv8BitPerColor) {
        pixel[0] = pcolor.r;
        pixel[1] = pcolor.g;
        pixel[2] = pcolor.b;
    } else if (config.pixel_format == kPixelBGRResv8BitPerColor) {
        pixel[0] = pcolor.b;
        pixel[1] = pcolor.g;
        pixel[2] = pcolor.r;
    } else {
        return -1;
    }

    return 0;
}

extern "C"
void KernelMain(
    const struct FrameBufferConfig &frame_buffer_config
) {
    for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x) {
        for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y) {
            WritePixel(frame_buffer_config, x, y, {255, 255, 255});
        }
    }

    for (int x = 0; x < 200; ++x) {
        for (int y = 0; y < 100; ++y) {
            WritePixel(frame_buffer_config, 100+x, 100+y, {0, 255, 0});
        }
    }

    while (1) {
        __asm__("hlt");
    }
}
