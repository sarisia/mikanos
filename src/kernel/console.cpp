#include <cstring>

#include "console.hpp"
#include "font.hpp"

Console::Console(const PixelColor& fg_color, const PixelColor& bg_color)
    : writer_(nullptr), fg_color_(fg_color), bg_color_(bg_color),
      buffer_(), cur_row_(0), cur_column_(0) {
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
        layer_manager->Draw();
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

void Console::newLine() {
    cur_column_ = 0;
    if (cur_row_ < kRows-1) {
        ++cur_row_;
    } else {
        // console is full
        // clear
        for (int y = 0; y < 16*kRows; ++y) {
            for (int x = 0; x < 8*kColumns; ++x) {
                writer_->Write(x, y, bg_color_);
            }
        }

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
    for (int row = 0; row < kRows; ++row) {
        WriteString(*writer_, 0, 16 * row, buffer_[row], fg_color_);
    }
}
