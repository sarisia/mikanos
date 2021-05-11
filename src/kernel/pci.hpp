#pragma once

#include <cstdint>
#include <array>

#include "error.hpp"

namespace pci {

// IO port addr of CONFIG_ADDRESS reg
const uint16_t kConfigAddress = 0x0cf8;
// IO port addr of CONFIG__DATA reg
const uint16_t kConfigData = 0x0cfc;

// PCI device class code
struct Classcode {
    uint8_t base, sub, interface;

    bool Match(uint8_t b) {
        return base == b;
    }

    bool Match(uint8_t b, uint8_t s) {
        return Match(b) && sub == s;
    }

    bool Match(uint8_t b, uint8_t s, uint8_t i) {
        return Match(b, s) && interface == i;
    }
};

struct Device {
    uint8_t bus, device, function, header_type;
    Classcode class_code;
};

// Get PCI configuration space
void WriteAddress(uint32_t addr);
void WriteData(uint32_t value);
uint32_t ReadData();

// Get values from PCI configuration space
uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t func);
uint16_t ReadDeviceID(uint8_t bus, uint8_t device, uint8_t func);
uint16_t ReadVendorID(uint8_t bus, uint8_t device, uint8_t func);
inline uint16_t ReadVendorID(const Device& dev) {
    return ReadVendorID(dev.bus, dev.device, dev.function);
}
Classcode ReadClassCode(uint8_t bus, uint8_t device, uint8_t func);
uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t func);
// Read Base Address Register
WithError<uint64_t> ReadBar(Device& device, unsigned int bar_index);

uint32_t ReadConfReg(const Device& device, uint8_t reg_addr);
void WriteConfigReg(const Device& device, uint8_t reg_addr, uint32_t value);

// check whether the device has multiple functions or not,
// by checking Header type bit 7
bool IsSingleFunctionDevice(uint8_t header_type);

constexpr uint8_t CalcBarAddress(unsigned int bar_index) {
    return 0x10 + 4*bar_index;
}

// PCI devices found by ScanAllBus()
inline std::array<Device, 32> devices;
// number of valid device
inline int num_device;

// scan PCI devices recursively, put to devices
Error ScanAllBus();

}
