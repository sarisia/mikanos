#pragma once

enum class LayerOperation {
    Move,
    MoveRelative,
    Draw,
    DrawArea,
};

struct Message {
    enum Type {
        kInterruptXHCI,
        kInterruptLAPICTimer,
        kTimerTimeout,
        kKeyPush,
        kLayer,
        kLayerFinish,
    } type;

    uint64_t src_task;

    union {
        struct {
            unsigned long timeout;
            int value;
        } timer; // kTimerTimeout

        struct {
            uint8_t modifier;
            uint8_t keycode;
            char ascii;
        } keyboard; // kKeyPush

        struct {
            LayerOperation op;
            unsigned int layer_id;
            int x, y;
            int w, h;
        } layer; // kLayer
    } arg;
};
