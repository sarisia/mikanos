#pragma once

#include <memory>
#include <algorithm>
#include <map>

#include "graphics.hpp"
#include "window.hpp"
#include "message.hpp"

class Layer {
private:
    unsigned int id_;
    Vector2D<int> pos_{};
    std::shared_ptr<Window> window_{};
    bool draggable_{false};

public:
    Layer(unsigned int id);
    
    // ID returns this layer's ID
    unsigned int ID() const;
    // GetWindow returns current window set to this layer
    std::shared_ptr<Window> GetWindow() const;

    // SetWindows sets window to the layer.
    // Current window will be unset.
    Layer &SetWindow(const std::shared_ptr<Window> &window);
    // Move moves layer's position to the given absolute pos.
    // This won't trigger drawing.
    Layer &Move(Vector2D<int> pos);
    Layer &MoveRelative(Vector2D<int> pos_diff);
    // DrawTo draws the current window to the given writer
    void DrawTo(FrameBuffer &screen, const Rectangle<int> &area);

    Vector2D<int> GetPosition() const;
    
    Layer &SetDraggable(bool draggable);
    bool IsDraggable() const;
};

class LayerManager {
private:
    FrameBuffer *screen_{nullptr};
    // back buffer
    mutable FrameBuffer back_buffer_{};
    std::vector<std::unique_ptr<Layer>> layers_{};
    // start is most back, end is most top
    std::vector<Layer *> layer_stack_{};
    // for generating unique id
    unsigned int latest_id_{0};

    void dumpLayerStack() const;

public:
    void SetWriter(FrameBuffer *screen);
    // Create new Layer. The new Layer will be maintained in the LayerManager
    Layer &NewLayer();
    // draw specified area in the screen
    void Draw(const Rectangle<int> &area) const;
    // draw specified layer and above
    void Draw(unsigned int id) const;
    void Draw(unsigned int id, Rectangle<int> area) const;

    // Move moves the layer specified by id, to the new position
    void Move(unsigned int id, Vector2D<int> new_position);
    void MoveRelative(unsigned int id, Vector2D<int> pos_diff);
    // move the layer to the given height
    // if new_pos is lower than 0, hide that window.
    // if new_pos is higher than layer_stack_.size(), the window goes topmost.
    void UpDown(unsigned int id, int new_height);
    void Hide(unsigned int id);

    int GetHeight(unsigned int id);

    Layer* FindLayer(unsigned int id);
    Layer *FindLayerByPosition(Vector2D<int> pos, unsigned int exclude_id) const;
};


class ActiveLayer {
public:
    ActiveLayer(LayerManager &manager);

    void SetMouseLayer(unsigned int mouse_layer);
    // Activate make layer topmost, then change color of titlebars
    void Activate(unsigned int layer_id);
    unsigned int GetActive() const;

private:
    LayerManager &manager_;
    unsigned int mouse_layer_{0};
    unsigned int active_layer_{0};
};

// global LayerManager
extern LayerManager *layer_manager;
extern ActiveLayer *active_layer;
extern std::map<unsigned int, uint64_t> *layer_task_map;

void InitializeLayer();
void ProcessLayerMessage(const Message &msg);

constexpr Message MakeLayerMessage(
    uint64_t task_id, unsigned int layer_id,
    LayerOperation op, const Rectangle<int>& area
) {
    Message msg{ Message::kLayer, task_id };
    msg.arg.layer.op = op;
    msg.arg.layer.layer_id = layer_id;
    msg.arg.layer.x = area.pos.x;
    msg.arg.layer.y = area.pos.y;
    msg.arg.layer.w = area.size.x;
    msg.arg.layer.h = area.size.y;

    return msg;
}
