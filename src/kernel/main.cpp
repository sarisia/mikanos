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

std::shared_ptr<Window> main_window;
unsigned int main_window_layer_id;

void InitializeMainWindow() {
    // make window
    main_window = std::make_shared<Window>(160, 52, screen_config.pixel_format);
    DrawWindow(*main_window->Writer(), "Hello window!");

    main_window_layer_id = layer_manager->NewLayer()
        .SetWindow(main_window)
        .SetDraggable(true)
        .Move({ 300, 100 })
        .ID();

    layer_manager->UpDown(main_window_layer_id, 2);
}

// text box window

std::shared_ptr<Window> text_window;
unsigned int text_window_layer_id;

void InitializeTextWindow() {
    const int win_w = 160;
    const int win_h = 52;

    text_window = std::make_shared<Window>(win_w, win_h, screen_config.pixel_format);
    DrawWindow(*text_window->Writer(), "Text box test");
    DrawTextbox(*text_window->Writer(), {4, 24}, {win_w-8, win_h-24-4});

    text_window_layer_id = layer_manager->NewLayer()
        .SetWindow(text_window)
        .SetDraggable(true)
        .Move({ 350, 150 })
        .ID();

    layer_manager->UpDown(text_window_layer_id, std::numeric_limits<int>::max());
}

int text_window_index;

void InputTextWindow(char c) {
    if (c == 0) {
        return;
    }

    auto pos = []() {
        return Vector2D<int>{8 + 8*text_window_index, 24+6};
    };

    const int max_chars = (text_window->Width() - 16) / 8;
    if (c == '\b' && text_window_index > 0) { // backspace
        --text_window_index;
        // update 1 char
        FillRectangle(*text_window->Writer(), pos(), {8, 16}, toColor(0xffffff));
    } else if (c >= ' ' && text_window_index < max_chars) { // ascii 0x20: space
        WriteAscii(*text_window->Writer(), pos(), c, toColor(0));
        ++text_window_index;
    }

    layer_manager->Draw(text_window_layer_id);
}


std::deque<Message> *main_queue;


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

    ::main_queue = new std::deque<Message>(32);
    InitializeInterrupt(main_queue);

    InitializePCI();
    usb::xhci::Initialize();

    InitializeLayer();
    InitializeMainWindow();
    InitializeTextWindow();
    InitializeMouse();

    layer_manager->Draw({ {0, 0}, ScreenSize() }); // draw all

    acpi::Initialize(acpi_table);
    InitializeLAPICTimer(*main_queue);
    timer_manager->AddTimer(Timer{200, 2});
    timer_manager->AddTimer(Timer{600, -1});

    InitializeKeyboard(*main_queue);

    // counter
    char str[128];

    while (true) {
        // update counter window
        __asm__("cli");
        const auto tick = timer_manager->CurrentTick();
        __asm__("sti");

        sprintf(str, "%010lu", tick);
        FillRectangle(*main_window->Writer(), {24, 28}, {8*10, 16}, {0xc6, 0xc6, 0xc6});
        WriteString(*main_window->Writer(), {24, 28}, str, {0, 0, 0});
        layer_manager->Draw(main_window_layer_id); // only refresh main window

        __asm__("cli"); // Clear interrupt flag

        if (main_queue->size() == 0) {
            // empty, enable interrupt and halt
            __asm__("sti"); // set interrupt flag
            __asm__("hlt");
            continue;
        }

        auto msg = main_queue->front();
        main_queue->pop_front();
        __asm__("sti");

        switch (msg.type) {
        case Message::kInterruptXHCI:
            usb::xhci::ProcessEvents();
            break;
        case Message::kTimerTimeout:
            printk("Timer: timeout %lu, value %d\n", msg.arg.timer.timeout, msg.arg.timer.value);
            // if (msg.arg.timer.value > 0) {
            //     timer_manager->AddTimer(Timer{
            //         msg.arg.timer.timeout + 100, msg.arg.timer.value + 1
            //     });
            // }
            break;
        case Message::kKeyPush:
            InputTextWindow(msg.arg.keyboard.ascii);
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
