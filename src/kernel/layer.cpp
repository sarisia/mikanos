#include "layer.hpp"

Layer::Layer(unsigned int id): id_{id} {}

unsigned int Layer::ID() const {
    return this->id_;
}

std::shared_ptr<Window> Layer::GetWindow() const {
    return this->window_;
}

Layer &Layer::SetWindow(const std::shared_ptr<Window> &window) {
    this->window_ = window;
    return *this;
}

Layer &Layer::Move(Vector2D <int> pos) {
    this->pos_ = pos;
    return *this;
}

Layer &Layer::MoveRelative(Vector2D <int> pos_diff) {
    this->pos_ += pos_diff;
    return *this;
}

void Layer::DrawTo(FrameBuffer &screen) const {
    if (this->window_) {
        this->window_->DrawTo(screen, this->pos_);
    }
}

void Layer::DrawTo(FrameBuffer &screen, const Rectangle<int> &area) {
    if (window_) {
        window_->DrawTo(screen, pos_, area); // area.pos == pos_
    }
}

Vector2D<int> Layer::GetPosition() const {
    return pos_;
}


// LayerManager

Layer *LayerManager::findLayer(unsigned int id) {
    auto pred = [id](const std::unique_ptr<Layer> &elem) {
        return elem->ID() == id;
    };

    auto it = std::find_if(layers_.begin(), layers_.end(), pred);
    if (it == layers_.end()) {
        return nullptr;
    }

    return it->get();
}

void LayerManager::SetWriter(FrameBuffer *screen) {
    screen_ = screen;

    // initialize back buffer
    FrameBufferConfig back_config = screen->Config();
    back_config.frame_buffer = nullptr;
    back_buffer_.Initialize(back_config);
}

Layer &LayerManager::NewLayer() {
    ++latest_id_;
    return *layers_.emplace_back(new Layer{latest_id_});
}

void LayerManager::Draw(const Rectangle<int> &area) const {
    for (auto layer: layer_stack_) {
        layer->DrawTo(back_buffer_, area);
    }

    // copy to front
    screen_->Copy(area.pos, back_buffer_, area);
}

void LayerManager::Draw(unsigned int id) const {
    bool draw = false;
    Rectangle<int> window_area;

    for (auto layer: layer_stack_) {
        if (layer->ID() == id) {
            window_area.size = layer->GetWindow()->Size();
            window_area.pos = layer->GetPosition();
            draw = true;
        }

        if (draw) {
            layer->DrawTo(back_buffer_, window_area);
        }
    }

    // copy to front
    screen_->Copy(window_area.pos, back_buffer_, window_area);
}

void LayerManager::Move(unsigned int id, Vector2D<int> new_position) {
    auto layer = findLayer(id);
    const auto window_size = layer->GetWindow()->Size();
    const auto old_pos = layer->GetPosition();
    layer->Move(new_position);
    Draw({old_pos, window_size});
    Draw(id);
}

void LayerManager::MoveRelative(unsigned int id, Vector2D<int> pos_diff) {
    findLayer(id)->MoveRelative(pos_diff);
}

void LayerManager::UpDown(unsigned int id, int new_height) {
    // lower than 0, hide window
    if (new_height < 0) {
        Hide(id);
        return;
    }

    // new_height is higher than size(), so it goes topmost
    // adjust value to save memory
    if (new_height > layer_stack_.size()) {
        new_height = layer_stack_.size();
    }

    auto layer = findLayer(id);
    auto old_pos = std::find(layer_stack_.begin(), layer_stack_.end(), layer);
    auto new_pos = layer_stack_.begin() + new_height;

    // layer is not in layer_stack_ (hidden) -> just insert it
    if (old_pos == layer_stack_.end()) {
        layer_stack_.insert(new_pos, layer);
        return;
    }

    // layer is in view, so remove & add
    if (new_pos == layer_stack_.end()) {
        --new_pos;
    }
    // do it
    layer_stack_.erase(old_pos);
    layer_stack_.insert(new_pos, layer);
}

void LayerManager::Hide(unsigned int id) {
    auto layer = findLayer(id);
    auto pos = std::find(layer_stack_.begin(), layer_stack_.end(), layer);
    if (pos != layer_stack_.end()) {
        layer_stack_.erase(pos);
    }
}

LayerManager *layer_manager;
