#include <cstdlib>
#include <cstring>

#include "acpi.hpp"
#include "logger.hpp"


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

void Initialize(const RSDP &rsdp) {
    if (!rsdp.IsValid()) {
        Log(kError, "invalid RSDP\n");
        exit(1);
    }
}

} // namespace acpi
