#pragma once

#include <cstdint>


class TimerManager {
private:
    volatile unsigned long tick_{0};

public:
    void Tick();
    unsigned long CurrentTick() const;
};

extern TimerManager *timer_manager;

// initialize Local APIC timer
void InitializeLAPICTimer();
void StartLAPICTimer();
uint32_t LAPICTimerElapsed();
void StopLAPICTimer();

void LAPICTimerOnInterrupt();
