#include "window.hpp"
#include "logger.hpp"
#include "font.hpp"


namespace {
    const int kCloseButtonWidth = 16;
    const int kCloseButtonHeight = 14;
    const char closeButtonMap[kCloseButtonHeight][kCloseButtonWidth+1] = {
        "...............@",
        ".:::::::::::::$@",
        ".:::::::::::::$@",
        ".:::@@::::@@::$@",
        ".::::@@::@@:::$@",
        ".:::::@@@@::::$@",
        ".::::::@@:::::$@",
        ".:::::@@@@::::$@",
        ".::::@@::@@:::$@",
        ".:::@@::::@@::$@",
        ".:::::::::::::$@",
        ".:::::::::::::$@",
        ".$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@",
    };

    void drawClose(PixelWriter &writer, int window_width) {
        for (int y = 0; y < kCloseButtonHeight; ++y) {
            for (int x = 0; x < kCloseButtonWidth; ++x) {
                auto c = toColor(0xffffff);
                switch (closeButtonMap[y][x]) {
                case '@':
                    c = toColor(0x000000); break;
                case '$':
                    c = toColor(0x848484); break;
                case ':':
                    c = toColor(0xc6c6c6); break;
                }

                writer.Write({ window_width - 5 - kCloseButtonWidth + x, 5 + y }, c);
            }
        }
    }

    void drawTextbox(PixelWriter& writer, Vector2D<int> pos, Vector2D<int> size,
                     const PixelColor &background, const PixelColor &border_light, const PixelColor &border_dark) {
        auto fill_rect =
            [&writer](Vector2D<int> pos, Vector2D<int> size, const PixelColor &color) {
            FillRectangle(writer, pos, size, color);
        };

        // main box
        fill_rect(pos + Vector2D<int>{1, 1}, size - Vector2D<int>{2, 2}, background);
        // border lines
        fill_rect(pos, { size.x, 1 }, border_dark); // top
        fill_rect(pos, { 1, size.y }, border_dark); // left
        fill_rect(pos + Vector2D<int>{0, size.y}, { size.x, 1 }, border_light); // bottom
        fill_rect(pos + Vector2D<int>{size.x, 0}, { 1, size.y }, border_light); // right
    }

}


Window::Window(int width, int height, PixelFormat shadow_format)
    : width_{width}, height_{height} {
    data_.resize(height);
    for (int i = 0; i < height; ++i) {
        data_[i].resize(width);
    }

    FrameBufferConfig config{};
    config.frame_buffer = nullptr;
    config.horizontal_resolution = width;
    config.vertical_resolution = height;
    config.pixel_format = shadow_format;

    if (auto err = shadow_buffer_.Initialize(config)) {
        Log(kError, "Failed to initialize shadow buffer (%s) at %s:%d\n",
            err.Name(), err.File(), err.Line());
    }
}

void Window::DrawTo(FrameBuffer &dst, Vector2D<int> pos, const Rectangle<int> &area) {
    if (!transparent_color_) {
        Rectangle<int> window_area{pos, Size()};
        Rectangle<int> intersection = area & window_area; // 重複領域
        dst.Copy(intersection.pos, shadow_buffer_, {intersection.pos-pos, intersection.size});
        return;
    }

    // transparent
    const auto tc = transparent_color_.value();
    auto& writer = dst.Writer();

    for (int y = std::max(0, 0 - pos.y); y < std::min(Height(), writer.Height() - pos.y); ++y) {
        for (int x = std::max(0, 0 - pos.x); x < std::min(Width(), writer.Width() - pos.x); ++x) {
            const auto c = At(x, y);
            if (c != tc) {
                writer.Write(pos + Vector2D<int>{x, y}, c);
            }
        }
    }
}

void Window::SetTransparentColor(std::optional<PixelColor> c) {
    transparent_color_ = c;
}

Window::WindowWriter *Window::Writer() {
    return &writer_;
}

const PixelColor& Window::At(int x, int y) const {
    return data_[y][x];
}

const PixelColor &Window::At(Vector2D<int> pos) const {
    return data_[pos.y][pos.x];
}

void Window::Write(Vector2D<int> pos, PixelColor color) {
    data_[pos.y][pos.x] = color;
    shadow_buffer_.Writer().Write(pos, color);
}

void Window::Move(Vector2D<int> dst_pos, const Rectangle<int> &src) {
    shadow_buffer_.Move(dst_pos, src);
}

int Window::Width() const {
    return width_;
}

int Window::Height() const {
    return height_;
}

Vector2D<int> Window::Size() const {
    return {width_, height_};
}


ToplevelWindow::ToplevelWindow(int width, int height, PixelFormat shadow_format, const std::string &title)
    : Window{width, height, shadow_format}, title_{title} {
    DrawWindow(*Writer(), title_.c_str());
};

void ToplevelWindow::Activate() {
    Window::Activate();
    DrawWindowTitle(*Writer(), title_.c_str(), true);
}

void ToplevelWindow::Deactivate() {
    Window::Deactivate();
    DrawWindowTitle(*Writer(), title_.c_str(), false);
}

Vector2D<int> ToplevelWindow::InnerSize() const {
    return Size() - kTopLeftMargin - kBottomRightMargin;
}


void DrawWindow(PixelWriter &writer, const char *title) {
    auto fillRect = [&writer](Vector2D<int> pos, Vector2D<int> size, uint32_t color) {
        FillRectangle(writer, pos, size, toColor(color));
    };

    const auto win_w = writer.Width();
    const auto win_h = writer.Height();

    // outline with shadow
    // top
    fillRect({0, 0},        {win_w, 1},         0xc6c6c6);
    fillRect({1, 1},        {win_w-2, 1},       0xffffff);
    // left
    fillRect({0, 0},        {1, win_h},         0xc6c6c6);
    fillRect({1, 1},        {1, win_h-2},       0xffffff);
    // right
    fillRect({win_w-2, 1},  {1, win_h-2},       0x848484);
    fillRect({win_w-1, 0},  {1, win_h},         0x000000);
    // bottom
    fillRect({1, win_h-2},  {win_w-2, 1},       0x848484);
    fillRect({0, win_h-1},  {win_w, 1},         0x000000);
    // window
    fillRect({2, 2},        {win_w-4, win_h-4}, 0xc6c6c6);
    
    DrawWindowTitle(writer, title, false);
}

void DrawTextbox(PixelWriter &writer, Vector2D<int> pos, Vector2D<int> size) {
    return drawTextbox(writer, pos, size,
                       toColor(0xffffff), toColor(0xc6c6c6), toColor(0x848484));
}

void DrawWindowTitle(PixelWriter &writer, const char *title, bool active) {
    const auto win_w = writer.Width();
    uint32_t bgcolor = 0x848484;
    if (active) {
        bgcolor = 0x000084;
    }

    FillRectangle(writer, {3, 3}, {win_w - 6, 18}, toColor(bgcolor));
    WriteString(writer, {24, 4}, title, toColor(0xffffff));

    drawClose(writer, win_w);
}

void DrawTerminal(PixelWriter &writer, Vector2D<int> pos, Vector2D<int> size) {
    return drawTextbox(writer, pos, size, toColor(0), toColor(0xc6c6c6), toColor(0x848484));
}
