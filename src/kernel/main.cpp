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


struct TaskContext {
    uint64_t cr3, rip, rflags, reserved1; // offset 0x00
    uint64_t cs, ss, fs, gs; // offset 0x20
    uint64_t rax, rbx, rcx, rdx, rdi, rsi, rsp, rbp; // offset 0x40
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15; // offset 0x80
    std::array<uint8_t, 512> fxsave_area; // offset 0xc0
} __attribute__((packed));

alignas(16) TaskContext task_a_ctx, task_b_ctx;


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


// task b window

std::shared_ptr<Window> task_b_window;
unsigned int task_b_window_layer_id;

void InitializeTaskBWindow() {
    task_b_window = std::make_shared<Window>(160, 52, screen_config.pixel_format);
    DrawWindow(*task_b_window->Writer(), "Task B");

    task_b_window_layer_id = layer_manager->NewLayer()
        .SetWindow(task_b_window)
        .SetDraggable(true)
        .Move({400, 400})
        .ID();

    layer_manager->UpDown(task_b_window_layer_id, std::numeric_limits<int>::max());
}

void TaskB(int task_id, int data) {
    printk("TaskB: task_id=%d, data=%d\n", task_id, data);
    char str[128];
    int count = 0;

    while (true) {
        ++count;
        sprintf(str, "%010d", count);
        FillRectangle(*task_b_window->Writer(), {24, 28}, {8*10, 16}, toColor(0xc6c6c6u));
        WriteString(*task_b_window->Writer(), {24, 28}, str, toColor(0));
        layer_manager->Draw(task_b_window_layer_id);

        SwitchContext(&task_a_ctx, &task_b_ctx);
    }
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

void DrawTextCursor(bool visible) {
    const auto color = visible ? toColor(0) : toColor(0xffffff);
    const int max_chars = (text_window->Width() - 16) / 8;
    if (text_window_index >= max_chars) {
        return;
    }
    const auto pos = Vector2D<int>{ 8 + 8*text_window_index, 24+5};
    FillRectangle(*text_window->Writer(), pos, {7, 15}, color);
}

void InputTextWindow(char c) {
    if (c == 0) {
        return;
    }

    auto pos = []() {
        return Vector2D<int>{8 + 8*text_window_index, 24+6};
    };

    const int max_chars = (text_window->Width() - 16) / 8;
    if (c == '\b' && text_window_index > 0) { // backspace
        DrawTextCursor(false);
        --text_window_index;
        // update 1 char
        FillRectangle(*text_window->Writer(), pos(), {8, 16}, toColor(0xffffff));
        DrawTextCursor(true);
    } else if (c >= ' ' && text_window_index < max_chars) { // ascii 0x20: space
        DrawTextCursor(false);
        WriteAscii(*text_window->Writer(), pos(), c, toColor(0));
        ++text_window_index;
        DrawTextCursor(true);
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
    InitializeTaskBWindow();
    InitializeMouse();

    layer_manager->Draw({ {0, 0}, ScreenSize() }); // draw all

    acpi::Initialize(acpi_table);
    InitializeLAPICTimer(*main_queue);

    InitializeKeyboard(*main_queue);

    // blink textbox cursor
    const int kTextboxCursorTimer = 1;
    const int kTimer05Sec = static_cast<int>(kTimerFreq*0.5);
    __asm__("cli");
    timer_manager->AddTimer(Timer{kTimer05Sec, kTextboxCursorTimer});
    __asm__("sti");
    bool textbox_cursor_visible = false;

    // task B
    std::vector<uint64_t> task_b_stack(1024);
    uint64_t task_b_stack_end = reinterpret_cast<uint64_t>(&task_b_stack[1024]);

    memset(&task_b_ctx, 0, sizeof(task_b_ctx));
    task_b_ctx.rip = reinterpret_cast<uint64_t>(TaskB);
    task_b_ctx.rdi = 1; // arg0 task_id
    task_b_ctx.rsi = 42; // arg1 data

    task_b_ctx.cr3 = GetCR3();
    task_b_ctx.rflags = 0x202; // bit 9: interrupt flag
    task_b_ctx.cs = kKernelCS;
    task_b_ctx.ss = kKernelSS;
    task_b_ctx.rsp = (task_b_stack_end & ~0xflu) - 8; // stack pointer initial value

    // MXCSR 例外マスク (???)
    // 浮動小数点計算
    *reinterpret_cast<uint32_t *>(&task_b_ctx.fxsave_area[24]) = 0x1f80;

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
            // __asm__("hlt");
            SwitchContext(&task_b_ctx, &task_a_ctx);
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
            if (msg.arg.timer.value == kTextboxCursorTimer) {
                __asm__("cli");
                timer_manager->AddTimer(Timer{msg.arg.timer.timeout + kTimer05Sec, kTextboxCursorTimer});
                __asm__("sti");
                textbox_cursor_visible = !textbox_cursor_visible;
                DrawTextCursor(textbox_cursor_visible);
                layer_manager->Draw(text_window_layer_id);
            }
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
