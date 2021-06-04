#include <cstring>

#include "console.hpp"
#include "font.hpp"

Console::Console(const PixelColor& fg_color, const PixelColor& bg_color)
    : writer_(nullptr), window_{}, fg_color_(fg_color), bg_color_(bg_color),
      buffer_(), cur_row_(0), cur_column_(0), layer_id_{0} {
}

void Console::PutString(const char* s) {
    while (*s) {
        if (*s == '\n') {
            newLine();
        } else if (cur_column_ < kColumns-1) {
            WriteAscii(*writer_, 8*cur_column_, 16*cur_row_, *s, fg_color_);
            buffer_[cur_row_][cur_column_] = *s;
            ++cur_column_;
        }

        ++s;
    }

    // if global layer_manager is available, refresh desktop
    if (layer_manager) {
        layer_manager->Draw(layer_id_);
    }
}

void Console::SetWriter(PixelWriter *writer) {
    // avoid redraw
    if (writer == writer_) {
        return;
    }

    writer_ = writer;
    refresh();
}

void Console::SetWindow(const std::shared_ptr<Window> &window) {
    if (window == window_) {
        return;
    }

    window_ = window;
    writer_ = window->Writer();
    refresh();
}

void Console::SetLayerID(unsigned int layer_id) {
    layer_id_ = layer_id;
}

unsigned int Console::LayerID() const {
    return layer_id_;
}

void Console::newLine() {
    cur_column_ = 0;
    if (cur_row_ < kRows-1) {
        ++cur_row_;
        return;
    } 
    
    if (window_) {
        // move all pixels to 1 lines above
        Rectangle<int> move_src{{0, 16}, {8*kColumns, 16*(kRows-1)}};
        window_->Move({0, 0}, move_src);
        // fill final line
        FillRectangle(*writer_, {0, 16*(kRows-1)}, {8*kColumns, 16}, bg_color_);
    } else {
        // clear all
        FillRectangle(*writer_, {0, 0}, {8*kColumns, 16*kRows}, bg_color_);

        // move buffers
        for (int row = 1; row < kRows; ++row) {
            memcpy(buffer_[row-1], buffer_[row], kColumns+1);
            WriteString(*writer_, 0, 16*(row-1), buffer_[row-1], fg_color_);
        }

        // clear last 1 line
        memset(buffer_[kRows-1], 0, kColumns+1);
    }
}

void Console::refresh() {
    FillRectangle(*writer_, {0, 0}, {8*kColumns, 16*kRows}, bg_color_);
    for (int row = 0; row < kRows; ++row) {
        WriteString(*writer_, 0, 16 * row, buffer_[row], fg_color_);
    }
}


namespace {
    char console_buf[sizeof(Console)];
}

Console *console;

void InitializeConsole() {
    ::console = new(console_buf) Console{
        kDesktopFGColor, kDesktopBGColor
    };
    console->SetWriter(screen_writer);
}
