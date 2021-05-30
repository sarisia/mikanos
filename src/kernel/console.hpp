#pragma once

#include "graphics.hpp"
#include "layer.hpp"

class Console {
public:
    static const int kRows = 25;
    static const int kColumns = 80;

    Console(const PixelColor& fg_color,
            const PixelColor& bg_color);
    
    void PutString(const char* s);
    void SetWriter(PixelWriter *writer);
    void SetWindow(const std::shared_ptr<Window> &window);
    void SetLayerID(unsigned int layer_id);
    
    unsigned int LayerID() const;

private:
    PixelWriter* writer_;
    std::shared_ptr<Window> window_;
    const PixelColor fg_color_, bg_color_;
    char buffer_[kRows][kColumns+1];
    int cur_row_, cur_column_;
    unsigned int layer_id_;

    void newLine();
    // refresh dumps all lines in the buffer
    void refresh();
};
