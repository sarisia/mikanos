#pragma once

#include <memory>
#include <algorithm>

#include "graphics.hpp"
#include "window.hpp"

class Layer {
private:
    unsigned int id_;
    Vector2D<int> pos_;
    std::shared_ptr<Window> window_;

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
    void DrawTo(PixelWriter &writer) const;
};

class LayerManager {
private:
    PixelWriter *writer_{nullptr};
    std::vector<std::unique_ptr<Layer>> layers_{};
    // start is most back, end is most top
    std::vector<Layer *> layer_stack_{};
    // for generating unique id
    unsigned int latest_id_{0};

    Layer *findLayer(unsigned int id);

public:
    void SetWriter(PixelWriter *writer);
    // Create new Layer. The new Layer will be maintained in the LayerManager
    Layer &NewLayer();
    void Draw() const;
    // Move moves the layer specified by id, to the new position
    void Move(unsigned int id, Vector2D<int> new_position);
    void MoveRelative(unsigned int id, Vector2D<int> pos_diff);
    // move the layer to the given height
    // if new_pos is lower than 0, hide that window.
    // if new_pos is higher than layer_stack_.size(), the window goes topmost.
    void UpDown(unsigned int id, int new_height);
    void Hide(unsigned int id);
};

// global LayerManager
extern LayerManager *layer_manager;
