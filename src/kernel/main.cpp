#include <cstdint>
#include <cstddef>
#include <cstdio>

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

#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"


// memory allocator

// void* operator new(size_t size, void* buf) {
//     return buf;
// }

void operator delete(void* obj) noexcept {}


char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;

char console_buf[sizeof(Console)];
Console* console;

char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor* mouse_cursor;

void MouseObserver(int8_t displacement_x, int8_t displacement_y) {
    mouse_cursor->MoveRelative({displacement_x, displacement_y});
}

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

void SwitchEhci2Xhci(const pci::Device& xhc_dev) {
    bool intel_ehc_exist = false;
    for (int i = 0; i < pci::num_device; ++i) {
        if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x20u) // EHCI
            && pci::ReadVendorID(pci::devices[i]) == 0x8086u) { // Intel
            intel_ehc_exist = true;
            break;   
        }
    }

    if (!intel_ehc_exist) {
        return;
    }

    uint32_t superspeed_ports = pci::ReadConfReg(xhc_dev, 0xdcu); // USB3PRM, PCI Device dependent region
    pci::WriteConfigReg(xhc_dev, 0xd8u, superspeed_ports); // USB3_PSSEN
    uint32_t ehci2xhci_ports = pci::ReadConfReg(xhc_dev, 0xd4u); // XUSB2PRM
    pci::WriteConfigReg(xhc_dev, 0xd0u, ehci2xhci_ports); //XUSB2PR

    Log(kDebug, "SwitchEhci2Xhci: SS=%02x, xHCI=%02x\n", superspeed_ports, ehci2xhci_ports);
}

usb::xhci::Controller* xhc;

struct Message {
    enum Type {
        kInterruptXHCI,
    } type;
};

ArrayQueue<Message>* main_queue;

__attribute__((interrupt))
void IntHandlerXHCI(InterruptFrame* frame) {
    main_queue->Push(Message{Message::kInterruptXHCI});
    NotifyEndOfInterrupt();
}

alignas(16) uint8_t kernel_main_stack[1024*1024];

extern "C"
void KernelMainNewStack(
    const struct FrameBufferConfig& frame_buffer_config_ref,
    const struct MemoryMap& memory_map_ref
) {
    FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
    MemoryMap memory_map{memory_map_ref};

    switch (frame_buffer_config.pixel_format) {
    case kPixelRGBResv8BitPerColor:
        pixel_writer = new(pixel_writer_buf) RGBResv8BitPerColorPixelWriter(frame_buffer_config);
        break;
    case kPixelBGRResv8BitPerColor:
        pixel_writer = new(pixel_writer_buf) BGRResv8BitPerColorPixelWriter(frame_buffer_config);
        break;
    }

    const int kFrameWidth = frame_buffer_config.horizontal_resolution;
    const int kFrameHeight = frame_buffer_config.vertical_resolution;
    const PixelColor kDesktopBGColor{58, 110, 165};
    const PixelColor kDesktopFGColor{255, 255, 255};

    // background
    FillRectangle(*pixel_writer, {0, 0}, {kFrameWidth, kFrameHeight-50}, kDesktopBGColor);
    // taskbar
    FillRectangle(*pixel_writer, {0, kFrameHeight-50}, { kFrameWidth, 50 }, {212, 208, 200});
    // selected app
    FillRectangle(*pixel_writer, {0, kFrameHeight-50}, {kFrameWidth/5, 50}, {80, 80, 80});
    DrawRectangle(*pixel_writer, {10, kFrameHeight-40}, {30, 30}, {160, 160, 160});


    console = new(console_buf) Console{*pixel_writer, kDesktopFGColor, kDesktopBGColor};
    
    printk("Welcome to MikanOS!\n");
    SetLogLevel(kDebug);

    // setup segments
    SetupSegments();

    const uint16_t kernel_cs = 1 << 3;
    const uint16_t kernel_ss = 2 << 3;
    SetDSAll(0);
    SetCSSS(kernel_cs, kernel_ss);

    SetupIdentityPageTable();

    // dump memory map
    const auto memory_map_base = reinterpret_cast<uintptr_t>(memory_map.buffer);
    for (uintptr_t iter = memory_map_base; iter < memory_map_base + memory_map.map_size; iter += memory_map.descriptor_size) {
        auto desc = reinterpret_cast<MemoryDescriptor *>(iter);
        if (IsAvailable(static_cast<MemoryType>(desc->type))) {
            Log(kDebug, "type=%u, phys=%08lx - %08lx, pages = %lu, attr = %08lx\n",
                desc->type,
                desc->physical_start,
                desc->physical_start+desc->number_of_pages*4096 - 1,
                desc->number_of_pages,
                desc->attribute);
        }
    }

    SetLogLevel(kWarn);

    // initialize mouse
    mouse_cursor = new(mouse_cursor_buf) MouseCursor{
        pixel_writer, kDesktopBGColor, {300, 200}
    };

    // initialize event queue
    std::array <Message,32> main_queue_data;
    ArrayQueue<Message> main_queue{main_queue_data};
    ::main_queue = &main_queue;

    // find all PCI devices
    auto err = pci::ScanAllBus();
    Log(kDebug, "ScanAllBus: %s\n", err.Name());

    // print all devices found
    Log(kDebug, "PCI Devices:\n");
    for (int i = 0; i < pci::num_device; ++i) {
        const auto& dev = pci::devices[i];
        auto vendor_id = pci::ReadVendorID(dev.bus, dev.device, dev.function);
        auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
        Log(kDebug, "%d.%d.%d: vendor %04x, class %08x, head %02x\n",
            dev.bus, dev.device, dev.function,
            vendor_id, class_code, dev.header_type);
    }

    // find xHC device
    pci::Device* xhc_dev = nullptr;
    for (int i = 0; i < pci::num_device; ++i) {
        // base 0x0c: serial bus
        // sub 0x03: USB controller
        // if 0x30: xHCI
        if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u)) {
            xhc_dev = &pci::devices[i];

            // prefer Intel one (vendor 0x8086)
            if (pci::ReadVendorID(*xhc_dev) == 0x8086u) {
                break;
            }
        }

        if (xhc_dev) {
            Log(kInfo, "found xHC: %d.%d.%d\n", xhc_dev->bus, xhc_dev->device, xhc_dev->function);
        }
    }


    // register interrupt handler
    SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
                reinterpret_cast<uint64_t>(IntHandlerXHCI), kernel_cs);
    LoadIDT(sizeof(idt)-1, reinterpret_cast<uintptr_t>(&idt[0]));

    const uint8_t bsp_local_apic_id = *reinterpret_cast<const uint32_t*>(0xfee00020) >> 24;
    pci::ConfigureMSIFixedDestination(
        *xhc_dev,
        bsp_local_apic_id,
        pci::MSITriggerMode::kLevel,
        pci::MSIDeliveryMode::kFixed,
        InterruptVector::kXHCI,
        0
    );


    const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
    Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
    const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
    Log(kDebug, "xHC mmio_base: %08lx\n", xhc_mmio_base);

    // initialize xHC
    usb::xhci::Controller xhc{xhc_mmio_base};
    
    // workaround for Intel Panther Point
    if (pci::ReadVendorID(*xhc_dev) == 0x8086u) {
        SwitchEhci2Xhci(*xhc_dev);
    }

    if (auto err = xhc.Initialize()) {
        Log(kDebug, "xhc.Initialize: %s\n", err.Name());
    }

    Log(kInfo, "Starting xHC\n");
    xhc.Run();

    ::xhc = &xhc;

    // register USB event handler
    usb::HIDMouseDriver::default_observer = MouseObserver;
    
    for (int i = 1; i <= xhc.MaxPorts(); ++i) {
        auto port = xhc.PortAt(i);
        Log(kDebug, "Port %d: IsConnected=%d\n", i, port.IsConnected());

        if (port.IsConnected()) {
            if (auto err = ConfigurePort(xhc, port)) {
                Log(kError, "Failed to configure port (%s) at %s:%d\n", err.Name(), err.File(), err.Line());
                continue;
            }
        }
    }


    while (true) {
        __asm__("cli"); // Clear interrupt flag

        if (main_queue.Count() == 0) {
            // empty, enable interrupt and halt
            __asm__("sti");
            __asm__("hlt");
            continue;
        }

        auto msg = main_queue.Front();
        main_queue.Pop();
        __asm__("sti");

        switch (msg.type) {
        case Message::kInterruptXHCI:
            while (xhc.PrimaryEventRing()->HasFront()) {
                if (auto err = ProcessEvent(xhc)) {
                    Log(kError, "Error while ProcessEvent (%s) at %s:%d\n",
                        err.Name(), err.File(), err.Line());
                }
            }
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
