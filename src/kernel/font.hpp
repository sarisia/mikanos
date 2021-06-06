#pragma once

#include <cstdint>
#include "graphics.hpp"

void WriteAscii(PixelWriter& writer, int x, int y, char c, const PixelColor& color);
void WriteAscii(PixelWriter& writer, Vector2D<int> pos, char c, const PixelColor& color);
void WriteString(PixelWriter& writer, int x, int y, const char* s, const PixelColor& color);
inline void WriteString(PixelWriter& writer, Vector2D<int> pos, const char* c, const PixelColor& color) {
    WriteString(writer, pos.x, pos.y, c, color);
}
