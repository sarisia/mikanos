#include "timer.hpp"

#include "interrupt.hpp"
#include "acpi.hpp"
#include "task.hpp"


TimerManager::TimerManager() {
    timers_.push(Timer{std::numeric_limits<unsigned long>::max(), -1});
}

void TimerManager::AddTimer(const Timer &timer) {
    timers_.push(timer);
}

bool TimerManager::Tick() {
    ++tick_;

    bool is_task_timer = false;
    while (true) {
        const auto &t = timers_.top();
        if (t.Timeout() > tick_) {
            // timeout not arrived, skip
            break;
        }

        // handle task timer
        if (t.Value() == kTaskTimerValue) {
            is_task_timer = true;
            timers_.pop();
            timers_.push(Timer{tick_+kTaskTimerPeriod, kTaskTimerValue});
            continue;
        }

        Message msg{Message::kTimerTimeout};
        msg.arg.timer.timeout = t.Timeout();
        msg.arg.timer.value = t.Value();
        task_manager->SendMessage(1, msg);

        timers_.pop();
    }

    return is_task_timer;
}

unsigned long TimerManager::CurrentTick() const {
    return tick_;
}


Timer::Timer(unsigned long timeout, int value):
    timeout_{timeout}, value_{value} {}

unsigned long Timer::Timeout() const {
    return timeout_;
}

int Timer::Value() const {
    return value_;
}

TimerManager* timer_manager;
unsigned long lapic_timer_freq;


namespace {
    const uint32_t kCountMax = 0xffffffffu;
    volatile uint32_t &lvt_timer = *reinterpret_cast<uint32_t *>(0xfee00320);
    volatile uint32_t &initial_count = *reinterpret_cast<uint32_t *>(0xfee00380);
    volatile uint32_t &current_count = *reinterpret_cast<uint32_t *>(0xfee00390);
    volatile uint32_t &divide_config = *reinterpret_cast<uint32_t *>(0xfee003e0);
}

void InitializeLAPICTimer() {
    timer_manager = new TimerManager();

    // set divide configuration (分周比)
    divide_config = 0b1011u; // 1:1
    lvt_timer = (0b001u << 16); // ?????

    StartLAPICTimer();
    acpi::WaitMilliseconds(100);
    const auto elapsed = LAPICTimerElapsed();
    StopLAPICTimer();

    lapic_timer_freq = static_cast<unsigned long>(elapsed) * 10;

    divide_config = 0b1011u; // 1:1
    lvt_timer = (0b010u << 16) | InterruptVector::kLAPICTimer; // ?????
    initial_count = lapic_timer_freq / kTimerFreq;
}

void StartLAPICTimer() {
    initial_count = kCountMax;
}

uint32_t LAPICTimerElapsed() {
    return kCountMax - current_count;
}

void StopLAPICTimer() {
    initial_count = 0;
}


void LAPICTimerOnInterrupt() {
    const bool is_task_timer = timer_manager->Tick();
    NotifyEndOfInterrupt();

    if (is_task_timer) {
        task_manager->SwitchTask();
    }
}
