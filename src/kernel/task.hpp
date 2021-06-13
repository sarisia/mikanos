#pragma once

#include <array>
#include <vector>
#include <memory>

struct TaskContext {
    uint64_t cr3, rip, rflags, reserved1; // offset 0x00
    uint64_t cs, ss, fs, gs; // offset 0x20
    uint64_t rax, rbx, rcx, rdx, rdi, rsi, rsp, rbp; // offset 0x40
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15; // offset 0x80
    std::array<uint8_t, 512> fxsave_area; // offset 0xc0
} __attribute__((packed));


void InitializeTask();
void SwitchTask();


using TaskFunc = void (uint64_t, int64_t);

class Task {
private:
    uint64_t id_;
    std::vector<uint64_t> stack_;
    alignas(16) TaskContext context_;

public:
    static const size_t kDefaultStackBytes = 4096;
    
    Task(uint64_t id);
    Task& InitContext(TaskFunc *f, int64_t data);
    TaskContext &Context();
};


class TaskManager {
private:
    std::vector<std::unique_ptr<Task>> tasks_{};
    uint64_t latest_id_{0};
    size_t current_task_index_{0};

public:
    TaskManager();
    Task &NewTask();
    void SwitchTask();
};

extern TaskManager* task_manager;
