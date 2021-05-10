#include "pci.hpp"
#include "asmfunc.h"

namespace {
using namespace pci;

// makeAddress generates 32bit integer for CONFIG_ADDRESS
uint32_t makeAddress(uint8_t bus, uint8_t device,
                     uint8_t func, uint8_t reg_addr) {
    auto shl = [](uint32_t x, unsigned int bits) {
        return x << bits;
    };

    return shl(1, 31) // enable bit: 31
         | shl(bus, 16) // bus number: 23:16
         | shl(device, 11) // device number: 15:11
         | shl(func, 8) // function number 10:8
         | (reg_addr & 0b11111100u); // register address 7:0, 4 byte off
}

Error scanBus(uint8_t bus);

Error addDevice(uint8_t bus, uint8_t device, uint8_t func, uint8_t header_type) {
    if (devices.size() == num_device) {
        return Error::kFull;
    }

    devices[num_device++] = Device{bus, device, func, header_type};
    return Error::kSuccess;
}

Error scanFunction(uint8_t bus, uint8_t device, uint8_t func) {
    // add device (function) to pci::devices
    auto header_type = ReadHeaderType(bus, device, func);
    if (auto err = addDevice(bus, device, func, header_type)) {
        return err;
    }

    // check PCI-to-PCI bridge
    auto class_code = ReadClassCode(bus, device, func);
    uint8_t base = (class_code >> 24) & 0xffu;
    uint8_t sub = (class_code >> 16) & 0xffu;

    // base 0x06, sub 0x04 -> PCI-to-PCI bridge
    if (base == 0x06u && sub == 0x04u) {
        auto bus_number = ReadBusNumbers(bus, device, func);
        uint8_t secondary_bus = (bus_number >> 8) & 0xffu;
        return scanBus(secondary_bus);
    }

    return Error::kSuccess;
}

Error scanDevice(uint8_t bus, uint8_t device) {
    // every device must have func 0
    if (auto err = scanFunction(bus, device, 0)) {
        return err;
    }

    if (IsSingleFunctionDevice(ReadHeaderType(bus, device, 0))) {
        return Error::kSuccess;
    }

    // every device can have up to 8 functions
    for (uint8_t func = 1; func < 8; ++func) {
        if (ReadVendorID(bus, device, func) == 0xffffu) {
            continue;
        }

        if (auto err = scanFunction(bus, device, func)) {
            return err;
        }
    }

    return Error::kSuccess;
}

Error scanBus(uint8_t bus) {
    // every bus can have up to 32 devices
    for (uint8_t dev = 0; dev < 32; ++dev) {
        // check valid device by checking all devices' func 0
        if (ReadVendorID(bus, dev, 0) == 0xffffu) {
            continue;
        }

        if (auto err = scanDevice(bus, dev)) {
            return err;
        }
    }

    return Error::kSuccess;
}

}


namespace pci {

void WriteAddress(uint32_t addr) {
    IoOut32(kConfigAddress, addr);
}

void WriteData(uint32_t value) {
    IoOut32(kConfigData, value);
}

uint32_t ReadData() {
    return IoIn32(kConfigData);
}

uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t func) {
    WriteAddress(makeAddress(bus, device, func, 0x0cu));
    return (ReadData() >> 16) & 0xffu;
}

uint16_t ReadDeviceID(uint8_t bus, uint8_t device, uint8_t func) {
    WriteAddress(makeAddress(bus, device, func, 0x00u));
    return ReadData() >> 16;
}

uint16_t ReadVendorID(uint8_t bus, uint8_t device, uint8_t func) {
    WriteAddress(makeAddress(bus, device, func, 0x00u));
    return ReadData() & 0xffffu;
}

uint32_t ReadClassCode(uint8_t bus, uint8_t device, uint8_t func) {
    WriteAddress(makeAddress(bus, device, func, 0x08u));
    return ReadData();
}

uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t func) {
    // Base Address Register 2?
    WriteAddress(makeAddress(bus, device, func, 0x18u));
    return ReadData();
}

bool IsSingleFunctionDevice(uint8_t header_type) {
    // bit 7
    return (header_type & 0b10000000u) == 0;
}

Error ScanAllBus() {
    num_device = 0;

    // bus 0 dev 0 func 0 -> Host bridge
    // device must have func 0
    auto header_type = ReadHeaderType(0, 0, 0);
    if (IsSingleFunctionDevice(header_type)) {
        // the machine only have one host bridge
        return scanBus(0);
    }

    // if not a SingleFuncDevice, search all functions
    for (uint8_t func = 1; func < 8; ++func) {
        if (ReadVendorID(0, 0, func) == 0xffffu) {
            // invalid vendor ID, not valid function
            continue;
        }
        
        if (auto err = scanBus(func)) {
            return err;
        }
    }

    return Error::kSuccess;
}

}
