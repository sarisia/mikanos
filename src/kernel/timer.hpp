#pragma once

#include <cstdint>

// initialize Local APIC timer
void InitializeLAPICTimer();
void StartLAPICTimer();
uint32_t LAPICTimerElapsed();
void StopLAPICTimer();
