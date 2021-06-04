#pragma once

#include "graphics.hpp"


class Mouse {
private:
    unsigned int layer_id_;
    Vector2D<int> position_{};
    unsigned int drag_layer_id_{0};
    uint8_t previous_buttons_{0};

public:
    Mouse(unsigned int layer_id);
    void OnInterrupt(uint8_t buttons, int8_t delta_x, int8_t delta_y);

    unsigned int LayerID() const;
    void SetPosition(Vector2D<int> position);
    Vector2D<int> Position() const;
};


const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const PixelColor kMouseTransparentColor{0, 0, 1};

void DrawMouseCursor(PixelWriter *writer, Vector2D<int> position);
void InitializeMouse();
