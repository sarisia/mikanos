#include "pci.hpp"
#include "asmfunc.h"
#include "logger.hpp"

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

Error addDevice(const Device& device) {
    if (devices.size() == num_device) {
        return MAKE_ERROR(Error::kFull);
    }

    devices[num_device++] = device;
    return MAKE_ERROR(Error::kSuccess);
}

Error scanFunction(uint8_t bus, uint8_t device, uint8_t func) {
    // add device (function) to pci::devices
    auto header_type = ReadHeaderType(bus, device, func);
    auto class_code = ReadClassCode(bus, device, func);
    Device dev{bus, device, func, header_type, class_code};
    if (auto err = addDevice(dev)) {
        return err;
    }

    // check PCI-to-PCI bridge
    // base 0x06, sub 0x04 -> PCI-to-PCI bridge
    if (class_code.Match(0x06u, 0x04u)) {
        auto bus_number = ReadBusNumbers(bus, device, func);
        uint8_t secondary_bus = (bus_number >> 8) & 0xffu;
        return scanBus(secondary_bus);
    }

    return MAKE_ERROR(Error::kSuccess);
}

Error scanDevice(uint8_t bus, uint8_t device) {
    // every device must have func 0
    if (auto err = scanFunction(bus, device, 0)) {
        return err;
    }

    if (IsSingleFunctionDevice(ReadHeaderType(bus, device, 0))) {
        return MAKE_ERROR(Error::kSuccess);
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

    return MAKE_ERROR(Error::kSuccess);
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

    return MAKE_ERROR(Error::kSuccess);
}

// read PCI device's MSI capability
MSICapability readMSICapability(const Device& dev, uint8_t msi_cap_addr) {
    MSICapability msi_cap{};
    msi_cap.header.data = ReadConfReg(dev, msi_cap_addr);
    msi_cap.msg_addr = ReadConfReg(dev, msi_cap_addr+4);

    uint8_t msg_data_addr = msi_cap_addr + 8;
    if (msi_cap.header.bits.addr_64_capable) {
        msi_cap.msg_upper_addr = ReadConfReg(dev, msi_cap_addr+8);
        msg_data_addr = msi_cap_addr + 12;
    }

    msi_cap.msg_data = ReadConfReg(dev, msg_data_addr);

    if (msi_cap.header.bits.per_vector_mask_capable) {
        msi_cap.mask_bits = ReadConfReg(dev, msg_data_addr+4);
        msi_cap.pending_bits = ReadConfReg(dev, msg_data_addr+8);
    }

    return msi_cap;
}

void writeMSICapability(const Device& dev, uint8_t msi_cap_addr, const MSICapability& msi_cap) {
    WriteConfigReg(dev, msi_cap_addr, msi_cap.header.data);
    WriteConfigReg(dev, msi_cap_addr+4, msi_cap.msg_addr);

    uint8_t msg_data_addr = msi_cap_addr + 8;
    if (msi_cap.header.bits.addr_64_capable) {
        WriteConfigReg(dev, msi_cap_addr+8, msi_cap.msg_upper_addr);
        msg_data_addr = msi_cap_addr + 12;
    }

    WriteConfigReg(dev, msg_data_addr, msi_cap.msg_data);

    if (msi_cap.header.bits.per_vector_mask_capable) {
        WriteConfigReg(dev, msg_data_addr+4, msi_cap.mask_bits);
        WriteConfigReg(dev, msg_data_addr+8, msi_cap.pending_bits);
    }
}

// Get MSI capability, then modify and write back to PCI configuration space.
Error configureMSIRegister(
    const Device& dev,
    uint8_t msi_cap_addr,
    uint32_t msg_addr,
    uint32_t msg_data,
    unsigned int num_vector_exponent
) {
    auto msi_cap = readMSICapability(dev, msi_cap_addr);
    if (msi_cap.header.bits.multi_msg_capable <= num_vector_exponent) {
        msi_cap.header.bits.multi_msg_enable = msi_cap.header.bits.multi_msg_capable;
    } else {
        msi_cap.header.bits.multi_msg_enable = num_vector_exponent;
    }

    msi_cap.header.bits.msi_enable = 1;
    msi_cap.msg_addr = msg_addr;
    msi_cap.msg_data = msg_data;

    writeMSICapability(dev, msi_cap_addr, msi_cap);
    return MAKE_ERROR(Error::kSuccess);
}

Error configureMSIXRegister(
    const Device& dev,
    uint8_t msi_cap_addr,
    uint32_t msg_addr,
    uint32_t msg_data,
    unsigned int num_vector_exponent
) {
    return MAKE_ERROR(Error::kNotImplemented);
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

Classcode ReadClassCode(uint8_t bus, uint8_t device, uint8_t func) {
    WriteAddress(makeAddress(bus, device, func, 0x08u));
    auto reg = ReadData();
    Classcode cc;
    cc.base = (reg >> 24) & 0xffu;
    cc.sub =  (reg >> 16) & 0xffu;
    cc.interface = (reg >> 8) & 0xffu;

    return cc;
}

uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t func) {
    // Base Address Register 2?
    WriteAddress(makeAddress(bus, device, func, 0x18u));
    return ReadData();
}

WithError<uint64_t> ReadBar(Device& device, unsigned int bar_index) {
    // BAR0 - BAR5
    if (bar_index >= 6) {
        return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
    }

    const auto addr = CalcBarAddress(bar_index);
    const auto bar = ReadConfReg(device, addr);

    // read flag, determine addr length
    // 32bit addr
    if ((bar & 0b0100u) == 0) {
        return {bar, MAKE_ERROR(Error::kSuccess)};
    }

    // 64bit addr
    if (bar_index >= 5) {
        return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
    }

    // concat
    const auto bar_upper = ReadConfReg(device, addr + 4); // content in bar_index+1
    return {
        bar | (static_cast<uint64_t>(bar_upper) << 32),
        MAKE_ERROR(Error::kSuccess)
    };
}

uint32_t ReadConfReg(const Device& device, uint8_t reg_addr) {
    WriteAddress(makeAddress(device.bus, device.device, device.function, reg_addr));
    return ReadData();
}

void WriteConfigReg(const Device& device, uint8_t reg_addr, uint32_t value) {
    WriteAddress(makeAddress(device.bus, device.device, device.function, reg_addr));
    WriteData(value);
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
    for (uint8_t func = 0; func < 8; ++func) {
        if (ReadVendorID(0, 0, func) == 0xffffu) {
            // invalid vendor ID, not valid function
            continue;
        }
        
        if (auto err = scanBus(func)) {
            return err;
        }
    }

    return MAKE_ERROR(Error::kSuccess);
}

CapabilityHeader ReadCapabilityHeader(const Device& dev, uint8_t addr) {
    CapabilityHeader header;
    header.data = pci::ReadConfReg(dev, addr);
    return header;
}

Error ConfigureMSI(
    const Device& dev,
    uint32_t msg_addr,
    uint32_t msg_data,
    unsigned int num_vector_exponent
) {
    // capabilities pointer from PCI configuration space 0x34-0x37
    uint8_t cap_addr = ReadConfReg(dev, 0x34u) & 0xffu;
    uint8_t msi_cap_addr = 0;
    uint8_t msix_cap_addr = 0;
    while (cap_addr != 0) {
        auto header = ReadCapabilityHeader(dev, cap_addr);
        if (header.bits.cap_id == kCapabilityMSI) {
            msi_cap_addr = cap_addr;
        } else if (header.bits.cap_id == kCapabilityMSIX) {
            msix_cap_addr = cap_addr;
        }

        cap_addr = header.bits.next_ptr;
    }

    if (msi_cap_addr) {
        return configureMSIRegister(dev, msi_cap_addr, msg_addr, msg_data, num_vector_exponent);
    } else if (msix_cap_addr) {
        return configureMSIXRegister(dev, msix_cap_addr, msg_addr, msg_data, num_vector_exponent);
    }

    return MAKE_ERROR(Error::kNoPCIMSI);
}

Error ConfigureMSIFixedDestination(
    const Device& dev,
    uint8_t apic_id,
    MSITriggerMode trigger_mode,
    MSIDeliveryMode delivery_mode,
    uint8_t vector,
    unsigned int num_vector_exponent
) {
    // Message Address (CPU register)
    uint32_t msg_addr = 0xfee00000u | (apic_id << 12); // bit 12-19 Destination ID
    // Message Data
    uint32_t msg_data = (static_cast<uint32_t>(delivery_mode) << 8) | vector;

    if (trigger_mode == MSITriggerMode::kLevel) {
        msg_data |= (0b1u << 15);
    }

    return ConfigureMSI(dev, msg_addr, msg_data, num_vector_exponent);
}

}


void InitializePCI() {
    // initialize event queue
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
}
