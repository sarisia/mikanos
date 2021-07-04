#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <deque>

#include "frame_buffer_config.hpp"
#include "memory_map.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"
#include "logger.hpp"
#include "mouse.hpp"
#include "interrupt.hpp"
#include "asmfunc.h"
#include "queue.hpp"
#include "segment.hpp"
#include "paging.hpp"
#include "memory_manager.hpp"
#include "window.hpp"
#include "layer.hpp"
#include "timer.hpp"
#include "message.hpp"
#include "acpi.hpp"
#include "keyboard.hpp"
#include "task.hpp"
#include "terminal.hpp"

#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"


int printk(const char* format, ...);

void operator delete(void* obj) noexcept {}


int printk(const char* format, ...) {
    va_list ap;
    int result;
    char s[1024];

    va_start(ap, format);
    result = vsprintf(s, format, ap);
    va_end(ap);

    console->PutString(s);
    return result;
}


// main window (counter)

std::shared_ptr<ToplevelWindow> main_window;
unsigned int main_window_layer_id;

void InitializeMainWindow() {
    // make window
    main_window = std::make_shared<ToplevelWindow>(160, 52, screen_config.pixel_format,
        "Hello window!");

    main_window_layer_id = layer_manager->NewLayer()
        .SetWindow(main_window)
        .SetDraggable(true)
        .Move({ 300, 100 })
        .ID();

    layer_manager->UpDown(main_window_layer_id, std::numeric_limits<int>::max());
}


// text box window

std::shared_ptr<ToplevelWindow> text_window;
unsigned int text_window_layer_id;

void InitializeTextWindow() {
    const int win_w = 160;
    const int win_h = 52;

    text_window = std::make_shared<ToplevelWindow>(win_w, win_h, screen_config.pixel_format,
        "Text Box Test");
    DrawTextbox(*text_window->InnerWriter(), {0, 0}, text_window->InnerSize());

    text_window_layer_id = layer_manager->NewLayer()
        .SetWindow(text_window)
        .SetDraggable(true)
        .Move({ 350, 150 })
        .ID();

    layer_manager->UpDown(text_window_layer_id, std::numeric_limits<int>::max());
}

int text_window_index;

void DrawTextCursor(bool visible) {
    const auto color = visible ? toColor(0) : toColor(0xffffff);
    const int max_chars = (text_window->Width() - 16) / 8;
    if (text_window_index >= max_chars) {
        return;
    }
    const auto pos = Vector2D<int>{ 4 + 8*text_window_index, 5};
    FillRectangle(*text_window->InnerWriter(), pos, {7, 15}, color);
}

void InputTextWindow(char c) {
    if (c == 0) {
        return;
    }

    auto pos = []() {
        return Vector2D<int>{4 + 8*text_window_index, 6};
    };

    const int max_chars = (text_window->InnerSize().x - 8) / 8;
    if (c == '\b' && text_window_index > 0) { // backspace
        DrawTextCursor(false);
        --text_window_index;
        // update 1 char
        FillRectangle(*text_window->InnerWriter(), pos(), {8, 16}, toColor(0xffffff));
        DrawTextCursor(true);
    } else if (c >= ' ' && text_window_index < max_chars) { // ascii 0x20: space
        DrawTextCursor(false);
        WriteAscii(*text_window->InnerWriter(), pos(), c, toColor(0));
        ++text_window_index;
        DrawTextCursor(true);
    }

    layer_manager->Draw(text_window_layer_id);
}


alignas(16) uint8_t kernel_main_stack[1024*1024];

extern "C"
void KernelMainNewStack(
    const struct FrameBufferConfig& frame_buffer_config_ref,
    const struct MemoryMap& memory_map_ref,
    const acpi::RSDP &acpi_table // root system descriptor pointer
) {
    // FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
    MemoryMap memory_map{memory_map_ref};

    InitializeGraphics(frame_buffer_config_ref);
    InitializeConsole();

    printk("Welcome to MikanOS!\n");
    SetLogLevel(kDebug);

    InitializeSegmentation();
    InitializePaging();
    InitializeMemoryManager(memory_map);

    SetLogLevel(kWarn);

    InitializeInterrupt();

    InitializePCI();

    InitializeLayer();
    InitializeMainWindow();
    InitializeTextWindow();

    layer_manager->Draw({ {0, 0}, ScreenSize() }); // draw all

    acpi::Initialize(acpi_table);
    InitializeLAPICTimer();

    // blink textbox cursor
    const int kTextboxCursorTimer = 1;
    const int kTimer05Sec = static_cast<int>(kTimerFreq*0.5);
    // __asm__("cli");
    timer_manager->AddTimer(Timer{kTimer05Sec, kTextboxCursorTimer});
    // __asm__("sti");
    bool textbox_cursor_visible = false;

    InitializeTask();
    Task& main_task = task_manager->CurrentTask();

    const auto task_terminal_id = task_manager->NewTask()
        .InitContext(TerminalTask, 0)
        .Wakeup()
        .ID();

    // pci devices
    usb::xhci::Initialize();
    InitializeKeyboard();
    InitializeMouse();

    // counter
    char str[128];

    while (true) {
        // update counter window
        __asm__("cli");
        const auto tick = timer_manager->CurrentTick();
        __asm__("sti");

        sprintf(str, "%010lu", tick);
        FillRectangle(*main_window->InnerWriter(), {20, 4}, {8*10, 16}, {0xc6, 0xc6, 0xc6});
        WriteString(*main_window->InnerWriter(), {20, 4}, str, {0, 0, 0});
        layer_manager->Draw(main_window_layer_id); // only refresh main window

        __asm__("cli"); // Clear interrupt flag

        auto rmsg = main_task.ReceiveMessage();
        if (!rmsg) {
            main_task.Sleep();
            __asm__("sti"); // set interrupt flag
            continue;
        }

        __asm__("sti");

        auto& msg = *rmsg;
        switch (msg.type) {
        case Message::kInterruptXHCI:
            usb::xhci::ProcessEvents();
            break;
        case Message::kTimerTimeout:
            if (msg.arg.timer.value == kTextboxCursorTimer) {
                __asm__("cli");
                timer_manager->AddTimer(Timer{msg.arg.timer.timeout + kTimer05Sec, kTextboxCursorTimer});
                __asm__("sti");

                textbox_cursor_visible = !textbox_cursor_visible;
                DrawTextCursor(textbox_cursor_visible);
                layer_manager->Draw(text_window_layer_id);

                __asm__("cli");
                task_manager->SendMessage(task_terminal_id, msg);
                __asm__("sti");
            }
            break;
        case Message::kKeyPush:
            if (auto act = active_layer->GetActive(); act == text_window_layer_id) {
                InputTextWindow(msg.arg.keyboard.ascii);
            } else {
                // send key event to the task of active layer (window)
                __asm__("cli");
                auto task_it = layer_task_map->find(act);
                __asm__("sti");

                if (task_it != layer_task_map->end()) {
                    __asm__("cli");
                    task_manager->SendMessage(task_it->second, msg);
                    __asm__("sti");
                } else {
                    printk("Keypush not handled (keycode: %02x, ascii: %02x)\n", msg.arg.keyboard.keycode, msg.arg.keyboard.ascii);
                }
            }

            break;
        case Message::kLayer:
            ProcessLayerMessage(msg);
            __asm__("cli");
            task_manager->SendMessage(msg.src_task, Message{Message::kLayerFinish});
            __asm__("sti");
            break;
        default:
            Log(kError, "Unknown interrupt message (%d)\n", msg.type);
        }
    }
}

extern "C" void __cxa_pure_virtual() {
    while (1) {
        __asm__("hlt");
    }
}
