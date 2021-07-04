#pragma once

#include <memory>

#include "window.hpp"
#include "graphics.hpp"

class Terminal {
public:
    static const int kRows = 15;
    static const int kColumns = 60;
    static const int kLineMax = 128;

    Terminal();
    unsigned int LayerID() const;
    Rectangle<int> BlinkCursor();
    Rectangle<int> InputKey(uint8_t modifier, uint8_t keycode, char ascii);

private:
    std::shared_ptr<ToplevelWindow> window_;
    unsigned int layer_id_;
    Vector2D<int> cursor_{0, 0};
    bool cursor_visible_{false};

    int linebuf_index_{0};
    std::array<char, kLineMax> linebuf_{};
    
    void drawCursor(bool visible);
    Vector2D<int> calcCursorPos() const;

    void scroll1();
    void print(const char *s);
    void executeLine();
};

void TerminalTask(uint64_t task_id, int64_t data);
