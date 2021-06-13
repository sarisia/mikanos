#include "task.hpp"

#include <cstring>
#include <algorithm>

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

uint64_t Task::ID() const {
    return id_;
}

Task& Task::Sleep() {
    task_manager->Sleep(this);
    return *this;
}

Task& Task::Wakeup() {
    task_manager->Wakeup(this);
    return *this;
}


TaskManager::TaskManager() {
    // spawn task for the caller of TaskManager constructor (main task)
    // and will be initialized when SwitchContext happens
    running_.push_back(&NewTask());
}

Task &TaskManager::NewTask() {
    ++latest_id_;
    return *tasks_.emplace_back(new Task{latest_id_});
}

void TaskManager::SwitchTask(bool current_sleep) {
    Task* current_task = running_.front();
    running_.pop_front();

    if (!current_sleep) {
        running_.push_back(current_task);
    }

    Task* next_task = running_.front();
    SwitchContext(&next_task->Context(), &current_task->Context());
}

void TaskManager::Sleep(Task* task) {
    auto it = std::find(running_.begin(), running_.end(), task);
    if (it == running_.begin()) { // current task
        SwitchTask(true);
    }

    if (it == running_.end()) { // not found
        return;
    }

    running_.erase(it);
}

Error TaskManager::Sleep(uint64_t id) {
    // original book finds the item from tasks_, but why?
    auto it = std::find_if(running_.begin(), running_.end(),
        [id](const auto& t) {
            return t->ID() == id;
        }
    );
    if (it == running_.end()) { // not found
        return MAKE_ERROR(Error::kNoSuchTask);
    }

    Sleep(*it);
    return MAKE_ERROR(Error::kSuccess);
}

void TaskManager::Wakeup(Task* task) {
    auto it = std::find(running_.begin(), running_.end(), task);
    if (it == running_.end()) { // not in running queue
        running_.push_back(task);
    }
}

Error TaskManager::Wakeup(uint64_t id) {
    auto it = std::find_if(tasks_.begin(), tasks_.end(),
        [id](const auto& t) {
            return t->ID() == id;
        }
    );
    if (it == tasks_.end()) {
        return MAKE_ERROR(Error::kNoSuchTask);
    }

    Wakeup(it->get());
    return MAKE_ERROR(Error::kSuccess);
}
