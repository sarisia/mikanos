#include <memory>

#include "mouse.hpp"
#include "window.hpp"
#include "layer.hpp"
#include "usb/classdriver/mouse.hpp"

namespace {

const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "@              ",
    "@@             ",
    "@.@            ",
    "@..@           ",
    "@...@          ",
    "@....@         ",
    "@.....@        ",
    "@......@       ",
    "@.......@      ",
    "@........@     ",
    "@.........@    ",
    "@..........@   ",
    "@...........@  ",
    "@............@ ",
    "@......@@@@@@@@",
    "@......@       ",
    "@....@@.@      ",
    "@...@ @.@      ",
    "@..@   @.@     ",
    "@.@    @.@     ",
    "@@      @.@    ",
    "@       @.@    ",
    "         @.@   ",
    "         @@@   "
};

}


Mouse::Mouse(unsigned int layer_id): layer_id_{layer_id} {}

void Mouse::OnInterrupt(uint8_t buttons, int8_t delta_x, int8_t delta_y) {
    const auto oldpos = position_;
    auto newpos = position_ + Vector2D<int>{delta_x, delta_y};
    newpos = ElementMin(newpos, ScreenSize() + Vector2D<int>{-1, -1});
    position_ = ElementMax(newpos, { 0, 0 });

    const auto posdiff = position_ - oldpos;

    layer_manager->Move(layer_id_, position_);

    // check click state
    const bool previous_left_pressed = (previous_buttons_ & 0x01);
    const bool left_pressed = (buttons & 0x01);
    if (!previous_left_pressed && left_pressed) {
        auto layer = layer_manager->FindLayerByPosition(position_, layer_id_);
        if (layer && layer->IsDraggable()) {
            drag_layer_id_ = layer->ID();
            active_layer->Activate(layer->ID());
        } else {
            active_layer->Activate(0);
        }
    }
    else if (previous_left_pressed && left_pressed) {
        if (drag_layer_id_ > 0) {
            layer_manager->MoveRelative(drag_layer_id_, posdiff);
        }
    }
    else if (previous_left_pressed && !left_pressed) {
        drag_layer_id_ = 0;
    }

    previous_buttons_ = buttons;
}

unsigned int Mouse::LayerID() const {
    return layer_id_;
}

void Mouse::SetPosition(Vector2D<int> position) {
    position_ = position;
    layer_manager->Move(layer_id_, position_);
}

Vector2D<int> Mouse::Position() const {
    return position_;
}


void DrawMouseCursor(PixelWriter* writer, Vector2D<int> pos) {
    for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
        for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
            switch (mouse_cursor_shape[dy][dx]) {
            case '@':
                writer->Write(pos.x + dx, pos.y + dy, { 0, 0, 0 });
                break;
            case '.':
                writer->Write(pos.x + dx, pos.y + dy, { 255, 255, 255 });
                break;
            default:
                writer->Write(pos.x + dx, pos.y + dy, kMouseTransparentColor);
            }
        }
    }
}

void InitializeMouse() {
    auto mouse_window = std::make_shared<Window>(kMouseCursorWidth, kMouseCursorHeight, screen_config.pixel_format);
    mouse_window->SetTransparentColor(kMouseTransparentColor);
    // actually this initializes window buffer
    DrawMouseCursor(mouse_window->Writer(), { 0, 0 });

    auto mouse_layer_id = layer_manager->NewLayer()
        .SetWindow(mouse_window)
        .ID();

    auto mouse = std::make_shared<Mouse>(mouse_layer_id);
    mouse->SetPosition({200, 200});
    layer_manager->UpDown(mouse->LayerID(), std::numeric_limits<int>::max());

    // register USB event handler
    usb::HIDMouseDriver::default_observer =
        [mouse](uint8_t buttons, int8_t delta_x, int8_t delta_y) {
            mouse->OnInterrupt(buttons, delta_x, delta_y);
        };

    // register mouse to activelayer manager
    active_layer->SetMouseLayer(mouse_layer_id);
}
