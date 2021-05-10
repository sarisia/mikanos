#include "graphics.hpp"

void RGBResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor& c) {
    auto p = PixelAt(x, y);
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
}

void BGRResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor& c) {
    auto p = PixelAt(x, y);
    p[0] = c.b;
    p[1] = c.g;
    p[2] = c.r;
}

void FillRectangle(PixelWriter& writer, const Vector2D<int>& pos,
    const Vector2D<int>& size, const PixelColor& color) {
    for (int dy = 0; dy < size.y; ++dy) {
        for (int dx = 0; dx < size.x; ++dx) {
            writer.Write(pos.x + dx, pos.y + dy, color);
        }
    }
}

void DrawRectangle(PixelWriter& writer, const Vector2D<int>& pos,
    const Vector2D<int>& size, const PixelColor& color) {
    for (int dx = 0; dx < size.x; ++dx) {
        writer.Write(pos.x + dx, pos.y, color);
        writer.Write(pos.x + dx, pos.y + size.y - 1, color);
    }

    for (int dy = 0; dy < size.y; ++dy) {
        writer.Write(pos.x, pos.y + dy, color);
        writer.Write(pos.x + size.x - 1, pos.y + dy, color);
    }
}
