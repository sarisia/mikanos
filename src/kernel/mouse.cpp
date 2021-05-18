#include "mouse.hpp"

namespace {

const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
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

}

void DrawMouseCursor(PixelWriter* writer, Vector2D<int> pos) {
    for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
        for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
            switch (mouse_cursor_shape[dy][dx]) {
            case '@':
                writer->Write(pos.x + dx, pos.y + dy, { 0, 0, 0 });
                break;
            case '.':
                writer->Write(pos.x + dx, pos.y + dy, { 255, 255, 255 });
                break;
            default:
                writer->Write(pos.x + dx, pos.y + dy, kMouseTransparentColor);
            }
        }
    }
}
