#include "task.hpp"

#include <cstring>
#include <algorithm>

#include "timer.hpp"
#include "asmfunc.h"
#include "segment.hpp"
#include "logger.hpp"


namespace {
    template <class T, class U>
    void erase(T& container, const U& value) {
        auto it = std::remove(container.begin(), container.end(), value);
        container.erase(it, container.end());
    }

    void taskIdle(uint64_t task_id, int64_t data) {
        while (true) {
            __asm__("hlt");
        }
    }
} // namespace


TaskManager *task_manager;

void InitializeTask() {
    task_manager = new TaskManager();

    __asm__("cli");
    timer_manager->AddTimer(
        Timer{timer_manager->CurrentTick()+kTaskTimerPeriod, kTaskTimerValue}
    );
    __asm__("sti");
}

Task& Task::setLevel(int level) {
    level_ = level;
    return *this;
}

Task& Task::setRunning(bool running) {
    running_ = running;
    return *this;
}

Task::Task(uint64_t id): id_{id}, msgs_{} {};

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

bool Task::Running() const {
    return running_;
}

int Task::Level() const {
    return level_;
}

Task& Task::Sleep() {
    task_manager->Sleep(this);
    return *this;
}

Task& Task::Wakeup() {
    task_manager->Wakeup(this);
    return *this;
}

void Task::SendMessage(const Message& msg) {
    msgs_.push_back(msg);
    Wakeup();
}

std::optional<Message> Task::ReceiveMessage() {
    if (msgs_.empty()) {
        return std::nullopt;
    }

    auto m = msgs_.front();
    msgs_.pop_front();
    return m;
}


TaskManager::TaskManager() {
    // spawn task for the caller of TaskManager constructor (main task)
    // and will be initialized when SwitchContext happens
    // using auto and you'll die.
    Task& task = NewTask()
        .setLevel(current_level_)
        .setRunning(true);
    running_[current_level_].push_back(&task);

    // add idle task that won't go sleep forever
    Task& idle = NewTask()
        .InitContext(taskIdle, 0)
        .setLevel(0)
        .setRunning(true);
    running_[0].push_back(&idle);
}

Task &TaskManager::NewTask() {
    ++latest_id_; // starts from 1
    return *tasks_.emplace_back(new Task{latest_id_});
}

void TaskManager::SwitchTask(bool current_sleep) {
    auto& level_queue = running_[current_level_];
    Task* current_task = level_queue.front();
    level_queue.pop_front();

    if (!current_sleep) {
        // current running task wants to go sleep
        // so we need to reschedule
        level_queue.push_back(current_task);
    }
    
    if (level_queue.empty()) {
        level_changed_ = true;
    }

    if (level_changed_) {
        level_changed_ = false;
        // find non-empty level
        for (int lv = kMaxLevel; lv >= 0; --lv) {
            if (!running_[lv].empty()) {
                current_level_ = lv;
                break;
            }
        }
    }

    Task* next_task = running_[current_level_].front();
    SwitchContext(&next_task->Context(), &current_task->Context());
}

void TaskManager::Sleep(Task* task) {
    if (!task->Running()) {
        return;
    }

    task->setRunning(false);

    if (task == running_[current_level_].front()) {
        // currently running
        SwitchTask(true);
        return;
    }

    erase(running_[task->Level()], task);
}

Error TaskManager::Sleep(uint64_t id) {
    // original book finds the item from tasks_, but why?
    // yes, i'm sure now.
    auto it = std::find_if(tasks_.begin(), tasks_.end(),
        [id](const auto& t) {
            return t->ID() == id;
        }
    );
    if (it == tasks_.end()) { // not found
        return MAKE_ERROR(Error::kNoSuchTask);
    }

    Sleep(it->get());
    return MAKE_ERROR(Error::kSuccess);
}

void TaskManager::Wakeup(Task* task, int level) {
    if (task->Running()) {
        changeRunLevel(task, level);
        return;
    }

    if (level < 0) {
        level = task->Level();
    }

    task->setLevel(level);
    task->setRunning(true);

    running_[level].push_back(task);
    if (level > current_level_) {
        level_changed_ = true;
    }

    // Log(kDebug, "wakeup task %p as level %d\n", task, level);
    // Log(kDebug, "dump running_ %d, %d, %d, %d\n", running_[0].size(), running_[1].size(), running_[2].size(), running_[3].size());
}

Error TaskManager::Wakeup(uint64_t id, int level) {
    auto it = std::find_if(tasks_.begin(), tasks_.end(),
        [id](const auto& t) {
            return t->ID() == id;
        }
    );
    if (it == tasks_.end()) {
        return MAKE_ERROR(Error::kNoSuchTask);
    }

    Wakeup(it->get(), level);
    return MAKE_ERROR(Error::kSuccess);
}

Task& TaskManager::CurrentTask() {
    return *running_[current_level_].front();
}

Error TaskManager::SendMessage(uint64_t id, const Message& msg) {
    auto it = std::find_if(tasks_.begin(), tasks_.end(),
        [id](const auto& t) {
            return t->ID() == id;
        }
    );

    if (it == tasks_.end()) {
        return MAKE_ERROR(Error::kNoSuchTask);
    }

    (*it)->SendMessage(msg);
    return MAKE_ERROR(Error::kSuccess);
}

void TaskManager::changeRunLevel(Task* task, int level) {
    if (level < 0 || level == task->Level()) {
        return;
    }

    if (task != running_[current_level_].front()) {
        // the task trying to change is not itself
        erase(running_[task->Level()], task);
        running_[level].push_back(task);
        task->setLevel(level);
        if (level > current_level_) {
            level_changed_ = true;
        }

        return;
    }

    // itself
    running_[current_level_].pop_front();
    running_[level].push_front(task);
    task->setLevel(level);

    if (level < current_level_) {
        // if this task goes lower, let SwitchTask to check higher tasks
        level_changed_ = true;
    }
    
    current_level_ = level;
}
