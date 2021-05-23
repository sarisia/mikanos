#include "graphics.hpp"

void RGBResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor& c) {
    auto p = PixelAt(x, y);
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
}
void RGBResv8BitPerColorPixelWriter::Write(Vector2D<int> pos, const PixelColor& c) {
    auto p = PixelAt(pos.x, pos.y);
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
void BGRResv8BitPerColorPixelWriter::Write(Vector2D<int> pos, const PixelColor& c) {
    auto p = PixelAt(pos.x, pos.y);
    p[0] = c.b;
    p[1] = c.g;
    p[2] = c.r;
}


void FillRectangle(PixelWriter& writer, const Vector2D<int>& pos,
    const Vector2D<int>& size, const PixelColor& color) {
    for (int dy = 0; dy < size.y; ++dy) {
        for (int dx = 0; dx < size.x; ++dx) {
            writer.Write(pos + Vector2D<int>{dx, dy}, color);
        }
    }
}

void DrawRectangle(PixelWriter& writer, const Vector2D<int>& pos,
    const Vector2D<int>& size, const PixelColor& color) {
    for (int dx = 0; dx < size.x; ++dx) {
        writer.Write(pos + Vector2D<int>{dx, 0}, color);
        writer.Write(pos + Vector2D<int>{dx, size.y-1}, color);
    }

    for (int dy = 0; dy < size.y; ++dy) {
        writer.Write(pos + Vector2D<int>{0, dy}, color);
        writer.Write(pos + Vector2D<int>{size.x-1, dy}, color);
    }
}

void DrawDesktop(PixelWriter &writer) {
    const auto width = writer.Width();
    const auto height = writer.Height();

    // background
    FillRectangle(writer, {0, 0}, {width, height-50}, kDesktopBGColor);
    // taskbar
    FillRectangle(writer, {0, height-50}, { width, 50 }, {212, 208, 200});
    // selected app
    FillRectangle(writer, {0, height-50}, {width/5, 50}, {80, 80, 80});
    DrawRectangle(writer, {10, height-40}, {30, 30}, {160, 160, 160});
}
