#include "task.hpp"

#include <cstring>

#include "timer.hpp"
#include "asmfunc.h"
#include "segment.hpp"


TaskManager *task_manager;

void InitializeTask() {
    task_manager = new TaskManager();

    __asm__("cli");
    timer_manager->AddTimer(
        Timer{timer_manager->CurrentTick()+kTaskTimerPeriod, kTaskTimerValue}
    );
    __asm__("sti");
}


Task::Task(uint64_t id): id_{id} {};

Task &Task::InitContext(TaskFunc *f, int64_t data) {
    // stack
    const size_t stack_size = kDefaultStackBytes / sizeof(stack_[0]);
    stack_.resize(stack_size);
    uint64_t stack_end = reinterpret_cast<uint64_t>(&stack_[stack_size]);

    // context
    memset(&context_, 0, sizeof(context_));
    context_.rip = reinterpret_cast<uint64_t>(f);
    context_.rdi = id_; // arg0 task_id
    context_.rsi = data; // arg1 data

    context_.cr3 = GetCR3();
    context_.rflags = 0x202; // bit 9: interrupt flag
    context_.cs = kKernelCS;
    context_.ss = kKernelSS;
    context_.rsp = (stack_end & ~0xflu) - 8; // stack pointer initial value

    // MXCSR 例外マスク (???)
    // 浮動小数点計算
    *reinterpret_cast<uint32_t*>(&context_.fxsave_area[24]) = 0x1f80;

    return *this;
}

TaskContext &Task::Context() {
    return context_;
}


TaskManager::TaskManager() {
    // spawn task for the caller of TaskManager constructor (main task)
    // and will be initialized when SwitchContext happens
    NewTask();
}

Task &TaskManager::NewTask() {
    ++latest_id_;
    return *tasks_.emplace_back(new Task{latest_id_});
}

void TaskManager::SwitchTask() {
    size_t next_task_index = current_task_index_ + 1;
    if (next_task_index >= tasks_.size()) {
        next_task_index = 0;
    }

    Task &current_task = *tasks_[current_task_index_];
    Task &next_task = *tasks_[next_task_index];
    current_task_index_ = next_task_index;

    SwitchContext(&next_task.Context(), &current_task.Context());
}
