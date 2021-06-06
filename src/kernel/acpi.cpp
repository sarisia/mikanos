#include <cstdlib>
#include <cstring>

#include "acpi.hpp"
#include "logger.hpp"
#include "asmfunc.h"


namespace {

template <typename T>
uint8_t sumBytes(const T *data, size_t bytes) {
    return sumBytes(reinterpret_cast<const uint8_t *>(data), bytes);
}

template <>
uint8_t sumBytes<uint8_t>(const uint8_t *data, size_t bytes) {
    uint8_t sum = 0;
    for (size_t i = 0; i < bytes; ++i) {
        sum += data[i];
    }

    return sum;
}

}


namespace acpi {

bool RSDP::IsValid() const {
    if (strncmp(this->signature, "RSD PTR ", 8) != 0) {
        Log(kDebug, "invalid RSDP signature: %.8s\n", this->signature);
        return false;
    }
    if (this->revision != 2) {
        Log(kDebug, "ACPI revision must be 2 (%d)\n", this->revision);
        return false;
    }
    if (auto sum = sumBytes(this, 20); sum != 0) {
        Log(kDebug, "checksum 20 bytes failed (%d)\n", sum);
        return false;
    }
    if (auto sum = sumBytes(this, 36); sum != 0) {
        Log(kDebug, "checksum 36 bytes failed (%d)\n", sum);
        return false;
    }

    return true;
}

bool DescriptionHeader::IsValid(const char *expected_signature) const {
    if (strncmp(this->signature, expected_signature, 4) != 0) {
        Log(kDebug, "invalid DescriptionHeader signature (%.4s)\n", this->signature);
        return false;
    }
    if (auto sum = sumBytes(this, this->length); sum != 0) {
        Log(kDebug, "checksum DescriptionHeader failed (%u bytes, %d)\n", this->length, sum);
        return false;
    }

    return true;
}

const DescriptionHeader &XSDT::operator [](size_t i) const {
    auto entries = reinterpret_cast<const uint64_t *>(&this->header + 1);
    return *reinterpret_cast<const DescriptionHeader *>(entries[i]);
}

size_t XSDT::Count() const {
    return (this->header.length - sizeof(DescriptionHeader)) / sizeof(uint64_t);
}


const FADT *fadt;

void Initialize(const RSDP &rsdp) {
    if (!rsdp.IsValid()) {
        Log(kError, "invalid RSDP\n");
        exit(1);
    }

    const XSDT &xsdt = *reinterpret_cast<const XSDT *>(rsdp.xsdt_address);
    if (!xsdt.header.IsValid("XSDT")) {
        Log(kError, "invalid XSDT\n");
        exit(1);
    }

    fadt = nullptr;
    for (int i = 0; i < xsdt.Count(); ++i) {
        const auto &entry = xsdt[i];
        if (entry.IsValid("FACP")) {
            fadt = reinterpret_cast<const FADT *>(&entry);
            break;
        }
    }

    if (fadt == nullptr) {
        Log(kError, "FADT not found\n");
        exit(1);
    }
}

void WaitMilliseconds(unsigned long msec) {
    const bool pm_timer_32 = (fadt->flags >> 8) & 1; // is 32bit timer?
    const uint32_t start = IoIn32(fadt->pm_tmr_blk);
    uint32_t end = start + kPMTimerFreq * msec / 1000;
    if (!pm_timer_32) {
        // 24bit timer
        end &= 0x00ffffffu;
    }

    if (end < start) {
        while (IoIn32(fadt->pm_tmr_blk) >= start);
    }
    while (IoIn32(fadt->pm_tmr_blk) < end);
}

} // namespace acpi
