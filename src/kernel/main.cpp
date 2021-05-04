#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth+1] = {
    "@              ",
    "@@             ",
    "@.@            ",
    "@..@           ",
    "@...@          ",
    "@....@         ",
    "@.....@        ",
    "@......@       ",
    "@.......@      ",
    "@........@     ",
    "@.........@    ",
    "@..........@   ",
    "@...........@  ",
    "@............@ ",
    "@......@@@@@@@@",
    "@......@       ",
    "@....@@.@      ",
    "@...@ @.@      ",
    "@..@   @.@     ",
    "@.@    @.@     ",
    "@@      @.@    ",
    "@       @.@    ",
    "         @.@   ",
    "         @@@   "
};

// memory allocator

void* operator new(size_t size, void* buf) {
    return buf;
}

void operator delete(void* obj) noexcept {}


char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;

char console_buf[sizeof(Console)];
Console* console;

int printk(const char* format, ...) {
    va_list ap;
    int result;
    char s[1024];

    va_start(ap, format);
    result = vsprintf(s, format, ap);
    va_end(ap);

    console->PutString(s);
    return result;
}

extern "C"
void KernelMain(
    const struct FrameBufferConfig& frame_buffer_config
) {
    switch (frame_buffer_config.pixel_format) {
    case kPixelRGBResv8BitPerColor:
        pixel_writer = new(pixel_writer_buf) RGBResv8BitPerColorPixelWriter(frame_buffer_config);
        break;
    case kPixelBGRResv8BitPerColor:
        pixel_writer = new(pixel_writer_buf) BGRResv8BitPerColorPixelWriter(frame_buffer_config);
        break;
    }

    const int kFrameWidth = frame_buffer_config.horizontal_resolution;
    const int kFrameHeight = frame_buffer_config.vertical_resolution;
    const PixelColor kDesktopBGColor{58, 110, 165};
    const PixelColor kDesktopFGColor{255, 255, 255};

    // background
    FillRectangle(*pixel_writer, {0, 0}, {kFrameWidth, kFrameHeight-50}, kDesktopBGColor);
    // taskbar
    FillRectangle(*pixel_writer, {0, kFrameHeight-50}, { kFrameWidth, 50 }, {212, 208, 200});
    // selected app
    FillRectangle(*pixel_writer, {0, kFrameHeight-50}, {kFrameWidth/5, 50}, {80, 80, 80});
    DrawRectangle(*pixel_writer, {10, kFrameHeight-40}, {30, 30}, {160, 160, 160});


    console = new(console_buf) Console{*pixel_writer, {0, 0, 0}, {255, 255, 255}};
    
    printk("Welcome to MikanOS!\n");

    // draw mouse cursor
    for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
        for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
            if (mouse_cursor_shape[dy][dx] == '@') {
                pixel_writer->Write(200+dx, 100+dy, {0, 0, 0});
            } else if (mouse_cursor_shape[dy][dx] == '.') {
                pixel_writer->Write(200+dx, 100+dy, {255, 255, 255});
            }
        }
    }

    while (1) {
        __asm__("hlt");
    }
}
