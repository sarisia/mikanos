#include "mouse.hpp"

namespace {

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
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

void drawMouseCursor(PixelWriter* writer, Vector2D<int> pos) {
    for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
        for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
            switch (mouse_cursor_shape[dy][dx]) {
            case '@':
                writer->Write(pos.x+dx, pos.y+dy, { 0, 0, 0 });
                break;
            case '.':
                writer->Write(pos.x+dx, pos.y+dy, {255, 255, 255});
                break;
            }
        }
    }
}

void eraseMouseCursor(PixelWriter* writer, Vector2D<int> pos, PixelColor erase_color) {
    for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
        for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
            if (mouse_cursor_shape[dy][dx] != ' ') {
                writer->Write(pos.x+dx, pos.y+dy, erase_color);
            }
        }
    }
}

}

MouseCursor::MouseCursor(PixelWriter* writer, PixelColor erase_color,
                         Vector2D<int> initial_position)
    : pixel_writer_(writer), erase_color_(erase_color), position_(initial_position) {
    drawMouseCursor(pixel_writer_, position_);
}

void MouseCursor::MoveRelative(Vector2D<int> displacement) {
    eraseMouseCursor(pixel_writer_, position_, erase_color_);
    position_ += displacement;
    drawMouseCursor(pixel_writer_, position_);
}
