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
}

Layer &LayerManager::NewLayer() {
    ++latest_id_;
    return *layers_.emplace_back(new Layer{latest_id_});
}

void LayerManager::Draw() const {
    for (auto layer: layer_stack_) {
        layer->DrawTo(*screen_);
    }
}

void LayerManager::Move(unsigned int id, Vector2D<int> new_position) {
    findLayer(id)->Move(new_position);
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
