#include "terminal.hpp"

#include <cstring>

#include "layer.hpp"
#include "task.hpp"
#include "window.hpp"
#include "logger.hpp"
#include "font.hpp"
#include "pci.hpp"

Terminal::Terminal() {
    window_ = std::make_shared<ToplevelWindow>(
        kColumns * 8 + 8 + ToplevelWindow::kMarginX,
        kRows * 16 + 8 + ToplevelWindow::kMarginY,
        screen_config.pixel_format,
        "MikanTerm"
    );
    DrawTerminal(*window_->InnerWriter(), {0, 0}, window_->InnerSize());

    layer_id_ = layer_manager->NewLayer()
        .SetWindow(window_)
        .SetDraggable(true)
        .ID();

    print("> ");
    cmd_history_.resize(8);
}

unsigned int Terminal::LayerID() const {
    return layer_id_;
}

Rectangle<int> Terminal::BlinkCursor() {
    cursor_visible_ = !cursor_visible_;
    drawCursor(cursor_visible_);

    return {
        calcCursorPos(), // pos
        {7, 15} // size
    };
}

Rectangle<int> Terminal::InputKey(uint8_t modifier, uint8_t keycode, char ascii) {
    drawCursor(false);

    Rectangle<int> draw_area{calcCursorPos(), {8*2, 16}}; // 2 char size (cursor + 1 char)

    if (ascii == '\n') {
        // line break
        linebuf_[linebuf_index_] = 0; // null char terminate
        // save command history
        if (linebuf_index_ > 0) {
            cmd_history_.pop_back();
            cmd_history_.push_front(linebuf_);
        }
        
        linebuf_index_ = 0;
        cmd_history_index_ = -1;
        cursor_.x = 0;

        // Log(kWarn, "line: %s\n", &linebuf_[0]);

        if (cursor_.y < kRows-1) {
            ++cursor_.y;
        } else {
            scroll1();
        }

        executeLine();
        print("> ");
        
        draw_area.pos = ToplevelWindow::kTopLeftMargin;
        draw_area.size = window_->InnerSize();
    } else if (ascii == '\b') {
        // backspace
        if (cursor_.x > 0) {
            --cursor_.x;
            // delete 1 char
            FillRectangle(*window_->Writer(), calcCursorPos(), {8, 16}, toColor(0));
            draw_area.pos = calcCursorPos();

            if (linebuf_index_ > 0) {
                --linebuf_index_;
            }
        }
    } else if (ascii != 0) {
        if (cursor_.x < kColumns - 1 && linebuf_index_ < kLineMax - 1) {
            linebuf_[linebuf_index_] = ascii;
            ++linebuf_index_;
            WriteAscii(*window_->Writer(), calcCursorPos(), ascii, toColor(0xffffff));
            ++cursor_.x;
        }
    } else if (keycode == 0x51) {
        // down arrow
        draw_area = historyUpDown(-1);
    } else if (keycode == 0x52) {
        draw_area = historyUpDown(1);
    }

    drawCursor(true);
    return draw_area;
}

void Terminal::drawCursor(bool visible) {
    const auto color = visible ? toColor(0xffffff) : toColor(0);
    FillRectangle(*window_->Writer(), calcCursorPos(), {7, 15}, color);
}

Vector2D<int> Terminal::calcCursorPos() const {
    return ToplevelWindow::kTopLeftMargin +
        Vector2D<int>{4 + 8*cursor_.x, 4 + 16*cursor_.y};
}

void Terminal::scroll1() {
    Rectangle<int> move_src{
        ToplevelWindow::kTopLeftMargin + Vector2D<int>{4, 4+16},
        {8*kColumns, 16*(kRows-1)}
    };
    window_->Move(ToplevelWindow::kTopLeftMargin + Vector2D<int>{4, 4}, move_src);
    FillRectangle(*window_->InnerWriter(), {4, 4+16*cursor_.y}, {8*kColumns, 16}, toColor(0));
}

void Terminal::print(const char *s) {
    drawCursor(false);

    auto newline = [this]() {
        cursor_.x = 0;
        if (cursor_.y < kRows-1) {
            ++cursor_.y;
        } else {
            scroll1();
        }
    };

    while (*s) {
        if (*s == '\n') {
            newline();
        } else {
            WriteAscii(*window_->Writer(), calcCursorPos(), *s, toColor(0xffffff));
            if (cursor_.x == kColumns-1) {
                newline();
            } else {
                ++cursor_.x;
            }
        }

        ++s;
    }

    drawCursor(true);
}

void Terminal::executeLine() {
    char *command = &linebuf_[0];
    char *first_arg = strchr(&linebuf_[0], ' ');
    if (first_arg) {
        *first_arg = 0;
        ++first_arg;
    }

    if (strcmp(command, "echo") == 0) {
        if (first_arg) {
            print(first_arg);
        }
        print("\n");
    } else if (strcmp(command, "clear") == 0) {
        FillRectangle(*window_->InnerWriter(), {4, 4}, {8*kColumns, 16*kRows}, toColor(0));
        cursor_.y = 0;
    } else if (strcmp(command, "lspci") == 0) {
        char s[64];
        for (int i = 0 ; i < pci::num_device; ++i) {
            const auto &dev = pci::devices[i];
            auto vendor_id = pci::ReadVendorID(dev);
            sprintf(s, "%02x:%02x.%d vendor %04x head %02x class %02x.%02x.%02x\n", 
                dev.bus, dev.device, dev.function, vendor_id, dev.header_type,
                dev.class_code.base, dev.class_code.sub, dev.class_code.interface);
            print(s);
        }
    } else if (command[0] != 0) {
        print("no such command: ");
        print(command);
        print("\n");
    }
}

Rectangle<int> Terminal::historyUpDown(int direction) {
    if (direction == -1 && cmd_history_index_ >= 0) {
        // down arrow
        --cmd_history_index_;
    } else if (direction == 1 && cmd_history_index_ + 1 < cmd_history_.size()) {
        // up arrow
        ++cmd_history_index_;
    }

    cursor_.x = 2;
    const auto first_pos = calcCursorPos();

    // erase current
    Rectangle<int> draw_area{first_pos, {8*(kColumns-2), 16}};
    FillRectangle(*window_->Writer(), draw_area.pos, draw_area.size, toColor(0));

    const char *history = "";
    if (cmd_history_index_ >= 0) {
        history = &cmd_history_[cmd_history_index_][0];
    }

    strcpy(&linebuf_[0], history);
    linebuf_index_ = strlen(history);

    WriteString(*window_->Writer(), first_pos, history, toColor(0xffffff));
    cursor_.x = linebuf_index_ + 2;

    return draw_area;
}


void TerminalTask(uint64_t task_id, int64_t data) {
    __asm__("cli");
    Task &task = task_manager->CurrentTask();
    Terminal *terminal = new Terminal;
    layer_manager->Move(terminal->LayerID(), {100, 200});
    active_layer->Activate(terminal->LayerID());
    layer_task_map->insert(std::make_pair(terminal->LayerID(), task_id));
    __asm__("sti");

    // mainloop
    while (true) {
        __asm__("cli");
        auto msg = task.ReceiveMessage();
        if (!msg) {
            task.Sleep();
            __asm__("sti");
            continue;
        }

        switch (msg->type) {
        case Message::kTimerTimeout:
            {
                const auto area = terminal->BlinkCursor();

                // send draw command
                Message msg = MakeLayerMessage(
                    task_id, terminal->LayerID(), LayerOperation::DrawArea, area
                );
                __asm__("cli");
                task_manager->SendMessage(1, msg);
                __asm__("sti");
            }

            break;
        case Message::kKeyPush:
            {
                const auto area = terminal->InputKey(msg->arg.keyboard.modifier,
                                                     msg->arg.keyboard.keycode,
                                                     msg->arg.keyboard.ascii);
                Message msg = MakeLayerMessage(task_id, terminal->LayerID(), LayerOperation::DrawArea, area);
                __asm__("cli");
                task_manager->SendMessage(1, msg);
                __asm__("sti");
            }
            break;
        default:
            break;
        }
    }
}
