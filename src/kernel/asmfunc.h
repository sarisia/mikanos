#pragma once

#include <stdint.h>

extern "C" {
    void IoOut32(uint16_t addr, uint32_t data);
    uint32_t IoIn32(uint16_t addr);
    // get current code segment selector value
    uint64_t GetCS(void);
    // load Interrupt Descriptor Table register
    void LoadIDT(uint64_t limit, uint64_t offset);

    // load Global Descriptor Table
    void LoadGDT(uint16_t limit, uint64_t offset);
    // set segment registers
    void SetDSAll(uint16_t value);
    // set cs and ss
    void SetCSSS(uint16_t cs, uint16_t ss);

    // set page table
    void SetCR3(uint64_t value);

    uint64_t GetCR3();

    void SwitchContext(void *next_ctx, void *current_ctx);
}
