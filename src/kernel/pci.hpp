#pragma once

#include <cstdint>
#include <array>

#include "error.hpp"

namespace pci {

// IO port addr of CONFIG_ADDRESS reg
const uint16_t kConfigAddress = 0x0cf8;
// IO port addr of CONFIG__DATA reg
const uint16_t kConfigData = 0x0cfc; 

// Get PCI configuration space
void WriteAddress(uint32_t addr);
void WriteData(uint32_t value);
uint32_t ReadData();

// Get values from PCI configuration space
uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t func);
uint16_t ReadDeviceID(uint8_t bus, uint8_t device, uint8_t func);
uint16_t ReadVendorID(uint8_t bus, uint8_t device, uint8_t func);
uint32_t ReadClassCode(uint8_t bus, uint8_t device, uint8_t func);
uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t func);

// check whether the device has multiple functions or not,
// by checking Header type bit 7
bool IsSingleFunctionDevice(uint8_t header_type);


struct Device {
    uint8_t bus, device, function, header_type;
};

// PCI devices found by ScanAllBus()
inline std::array<Device, 32> devices;
// number of valid device
inline int num_device;

// scan PCI devices recursively, put to devices
Error ScanAllBus();

}
