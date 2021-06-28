#pragma once

#include <vector>
#include <optional>
#include <string>

#include "graphics.hpp"
#include "frame_buffer.hpp"

class Window {    
public:
    // WindowWriter provides PixelWriter combined with Window
    class WindowWriter: public PixelWriter {
    private:
        Window &window_;

    public:
        WindowWriter(Window &window): window_{window} {}

        // Write draws pixel to the given position
        void Write(int x, int y, const PixelColor& color) override {
            Write(Vector2D<int>{x, y}, color);
        }
        void Write(Vector2D<int> pos, const PixelColor &color) override {
            window_.Write(pos, color);
        }

        // Width returns window's width, in pixel manner
        int Width() const override {
            return window_.Width();
        }

        // Height returns window's height, in pixel manner
        int Height() const override {
            return window_.Height();
        }
    };

    Window(int width, int height, PixelFormat shadow_format);
    virtual ~Window() = default;

    // prohibit copy
    Window(const Window& rhs) = delete;
    Window &operator =(const Window& rhs) = delete;

    // DrawTo draws this window to the given PixelWriter
    void DrawTo(FrameBuffer &dst, Vector2D<int> pos, const Rectangle<int> &area);
    // SetTransparentColor sets transparent color
    void SetTransparentColor(std::optional<PixelColor> c);

    WindowWriter *Writer();
    const PixelColor &At(int x, int y) const;
    const PixelColor &At(Vector2D<int> pos) const;
    void Write(Vector2D<int> pos, PixelColor color);

    // move rectangle in this window buffer
    void Move(Vector2D<int> dst_pos, const Rectangle<int> &src);

    int Width() const;
    int Height() const;
    Vector2D<int> Size() const;

    virtual void Activate() {};
    virtual void Deactivate() {};

private:
    int width_, height_;
    std::vector<std::vector<PixelColor>> data_{};
    WindowWriter writer_{ *this };
    std::optional<PixelColor> transparent_color_{std::nullopt};
    FrameBuffer shadow_buffer_{};
};


class ToplevelWindow: public Window {
public:
    static constexpr Vector2D<int> kTopLeftMargin{4, 24};
    static constexpr Vector2D<int> kBottomRightMargin{4, 4};
    static constexpr int kMarginX = kTopLeftMargin.x +kBottomRightMargin.x;
    static constexpr int kMarginY = kTopLeftMargin.y + kBottomRightMargin.y;

    class InnerAreaWriter: public PixelWriter {
    public:
        InnerAreaWriter(ToplevelWindow &window): window_{window} {}

        virtual void Write(Vector2D<int> pos, const PixelColor &color) override {
            window_.Write(pos+kTopLeftMargin, color);
        }

        virtual int Width() const override {
            return window_.Width() - kTopLeftMargin.x - kBottomRightMargin.x;
        }

        virtual int Height() const override {
            return window_.Height() - kTopLeftMargin.y - kBottomRightMargin.y;
        }

    private:
        ToplevelWindow& window_;
    };

    ToplevelWindow(int width, int height, PixelFormat shadow_format, const std::string &title);

    virtual void Activate() override;
    virtual void Deactivate() override;

    InnerAreaWriter *InnerWriter() {
        return &inner_writer_;
    }
    Vector2D<int> InnerSize() const;

private:
    std::string title_;
    InnerAreaWriter inner_writer_{*this};
};


void DrawWindow(PixelWriter &writer, const char *title);
void DrawTextbox(PixelWriter &writer, Vector2D<int> pos, Vector2D<int> size);
void DrawWindowTitle(PixelWriter &writer, const char *title, bool active);
void DrawTerminal(PixelWriter &writer, Vector2D<int> pos, Vector2D<int> size);
