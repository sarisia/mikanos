#include "layer.hpp"
#include "console.hpp"
#include "logger.hpp"

extern int printk(const char* format, ...);

Layer::Layer(unsigned int id): id_{id} {}

unsigned int Layer::ID() const {
    return this->id_;
}

std::shared_ptr<Window> Layer::GetWindow() const {
    return window_;
}

Layer &Layer::SetWindow(const std::shared_ptr<Window> &window) {
    window_ = window;
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

void Layer::DrawTo(FrameBuffer &screen, const Rectangle<int> &area) {
    if (window_) {
        window_->DrawTo(screen, pos_, area); // area.pos == pos_
    }
}

Vector2D<int> Layer::GetPosition() const {
    return pos_;
}

Layer &Layer::SetDraggable(bool draggable) {
    draggable_ = draggable;
    return *this;
}

bool Layer::IsDraggable() const {
    return draggable_;
}


// LayerManager

Layer *LayerManager::FindLayer(unsigned int id) {
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
    return Draw(id, {{0, 0}, {-1, -1}});
}

void LayerManager::Draw(unsigned int id, Rectangle<int> area) const {
    bool draw = false;
    Rectangle<int> window_area;

    for (auto layer: layer_stack_) {
        if (layer->ID() == id) {
            window_area.size = layer->GetWindow()->Size();
            window_area.pos = layer->GetPosition();
            draw = true;
            
            if (area.size.x >= 0 || area.size.y >= 0) {
                // area is a point from top left of window
                // while window_area is a point from top left of screen,
                // so sum up!
                area.pos = area.pos + window_area.pos;
                window_area = window_area & area;
            }
        }

        if (draw) {
            layer->DrawTo(back_buffer_, window_area);
        }
    }

    // copy to front
    screen_->Copy(window_area.pos, back_buffer_, window_area);
}

void LayerManager::Move(unsigned int id, Vector2D<int> new_position) {
    auto layer = FindLayer(id);
    const auto window_size = layer->GetWindow()->Size();
    const auto old_pos = layer->GetPosition();
    layer->Move(new_position);
    Draw({old_pos, window_size});
    Draw(id);
}

void LayerManager::MoveRelative(unsigned int id, Vector2D<int> pos_diff) {
    auto layer = FindLayer(id);
    const auto window_size = layer->GetWindow()->Size();
    const auto old_pos = layer->GetPosition();
    layer->MoveRelative(pos_diff);
    Draw({ old_pos, window_size });
    Draw(id);
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

    auto layer = FindLayer(id);
    auto old_pos = std::find(layer_stack_.begin(), layer_stack_.end(), layer);
    auto new_pos = layer_stack_.begin() + new_height;

    // layer is not in layer_stack_ (hidden) -> just insert it
    if (old_pos == layer_stack_.end()) {
        layer_stack_.insert(new_pos, layer);
        // dumpLayerStack(); // DEBUG
        return;
    }

    // layer is in view, so remove & add
    if (new_pos == layer_stack_.end()) {
        --new_pos;
    }
    // do it
    layer_stack_.erase(old_pos);
    layer_stack_.insert(new_pos, layer);

    // dumpLayerStack(); // DEBUG
}

void LayerManager::Hide(unsigned int id) {
    auto layer = FindLayer(id);
    auto pos = std::find(layer_stack_.begin(), layer_stack_.end(), layer);
    if (pos != layer_stack_.end()) {
        layer_stack_.erase(pos);
    }
}

int LayerManager::GetHeight(unsigned int id) {
    for (int h = 0; h < layer_stack_.size(); ++h ) {
        if (layer_stack_[h]->ID() == id) {
            return h;
        }
    }

    return -1;
}

Layer *LayerManager::FindLayerByPosition(Vector2D<int> pos, unsigned int exclude_id) const {
    auto pred = [pos, exclude_id](Layer *layer) {
        if (layer->ID() == exclude_id) {
            return false;
        }

        const auto &win = layer->GetWindow();
        if (!win) {
            return false;
        }

        const auto win_pos = layer->GetPosition();
        const auto win_pos_end = win_pos + win->Size();
        return win_pos.x <= pos.x && pos.x < win_pos_end.x &&
            win_pos.y <= pos.y && pos.y < win_pos_end.y;
    };

    auto it = std::find_if(layer_stack_.rbegin(), layer_stack_.rend(), pred);
    if (it == layer_stack_.rend()) {
        return nullptr;
    }

    return *it;
}

void LayerManager::dumpLayerStack() const {
    printk("layer_stack_ (", layer_stack_.size());
    for (auto l : layer_stack_) {
        printk("%d, ", l->ID());
    }
    printk(")\n");
}


namespace {
    FrameBuffer *screen;
}


ActiveLayer::ActiveLayer(LayerManager &manager)
    : manager_{manager} {}

void ActiveLayer::SetMouseLayer(unsigned int mouse_layer) {
    mouse_layer_ = mouse_layer;
}

void ActiveLayer::Activate(unsigned int layer_id) {
    if (layer_id == active_layer_) {
        return;
    }

    if (active_layer_ > 0) {
        Layer *layer = manager_.FindLayer(active_layer_);
        layer->GetWindow()->Deactivate();
        manager_.Draw(active_layer_);
    }

    active_layer_ = layer_id;
    if (active_layer_ > 0) {
        Layer *layer = manager_.FindLayer(active_layer_);
        layer->GetWindow()->Activate();
        manager_.UpDown(active_layer_, manager_.GetHeight(mouse_layer_)-1);
        manager_.Draw(active_layer_);
    }
}

unsigned int ActiveLayer::GetActive() const {
    return active_layer_;
}


LayerManager *layer_manager;
ActiveLayer *active_layer;
std::map<unsigned int, uint64_t> *layer_task_map;

void InitializeLayer() {
    const auto screen_size = ScreenSize();

    auto bg_window = std::make_shared<Window>(screen_size.x, screen_size.y, screen_config.pixel_format);
    auto bg_writer = bg_window->Writer();

    DrawDesktop(*bg_writer);

    // console
    auto console_window = std::make_shared<Window>(Console::kColumns * 8, Console::kRows * 16, screen_config.pixel_format);
    console->SetWindow(console_window);

    screen = new FrameBuffer();
    if (auto err = screen->Initialize(screen_config)) {
        Log(kError, "Failed to initialize frame buffer (%s) at %s:%d\n",
            err.Name(), err.File(), err.Line());
        exit(1);
    }

    layer_manager = new LayerManager();
    layer_manager->SetWriter(screen);

    auto bglayer_id = layer_manager->NewLayer()
        .SetWindow(bg_window)
        .Move({ 0, 0 })
        .ID();
    console->SetLayerID(layer_manager->NewLayer()
        .SetWindow(console_window)
        .Move({ 0, 0 })
        .ID());

    layer_manager->UpDown(bglayer_id, 0);
    layer_manager->UpDown(console->LayerID(), 1);

    active_layer = new ActiveLayer{*layer_manager};
    layer_task_map = new std::map<unsigned int, uint64_t>;
}

void ProcessLayerMessage(const Message &msg) {
    const auto &arg = msg.arg.layer;
    switch (arg.op) {
    case LayerOperation::Move:
        layer_manager->Move(arg.layer_id, {arg.x, arg.y});
        break;
    case LayerOperation::MoveRelative:
        layer_manager->MoveRelative(arg.layer_id, {arg.x, arg.y});
        break;
    case LayerOperation::Draw:
        layer_manager->Draw(arg.layer_id);
        break;
    case LayerOperation::DrawArea:
        layer_manager->Draw(arg.layer_id, {{arg.x, arg.y}, {arg.w, arg.h}});
        break;
    }
}
