#pragma once

#include <vector>
#include <optional>

#include "graphics.hpp"

class Window {    
public:
    // WindowWriter provides PixelWriter combined with Window
    class WindowWriter: public PixelWriter {
    private:
        Window &window_;

    public:
        WindowWriter(Window &window): window_{window} {}
        // Write draws pixel to the given position
        void Write(int x, int y, const PixelColor &color) override {
            window_.At(x, y) = color;
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

    Window(int width, int height);
    ~Window() = default;

    // prohibit copy
    Window(const Window& rhs) = delete;
    Window &operator =(const Window& rhs) = delete;

    // DrawTo draws this window to the given PixelWriter
    void DrawTo(PixelWriter &writer, Vector2D<int> position);
    // SetTransparentColor sets transparent color
    void SetTransparentColor(std::optional<PixelColor> c);

    WindowWriter *Writer();
    PixelColor &At(int x, int y);
    const PixelColor &At(int x, int y) const;

    int Width() const;
    int Height() const;


private:
    int width_, height_;
    std::vector<std::vector<PixelColor>> data_{};
    WindowWriter writer_{ *this };
    std::optional<PixelColor> transparent_color_{std::nullopt};
};
