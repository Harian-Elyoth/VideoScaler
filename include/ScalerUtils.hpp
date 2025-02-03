#ifndef SCALERUTILS_HPP
#define SCALERUTILS_HPP

#include <vector>
#include <cstdint>

struct Pixel {
    uint8_t r, g, b;
};

class Frame {
public:
    Frame() : width(0), height(0) {}
    Frame(int h, int w) : width(w), height(h), data(w * h * 3) {}
    
    // Direct access to raw data
    uint8_t* getData() { return data.data(); }
    const uint8_t* getData() const { return data.data(); }
    
    // Get dimensions
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getStride() const { return width * 3; }
    
    // Access pixels
    Pixel getPixel(int y, int x) const {
        int idx = (y * width + x) * 3;
        return Pixel{data[idx], data[idx+1], data[idx+2]};
    }
    
    void setPixel(int y, int x, const Pixel& p) {
        int idx = (y * width + x) * 3;
        data[idx] = p.r;
        data[idx+1] = p.g;
        data[idx+2] = p.b;
    }
    
    bool empty() const { return data.empty(); }

private:
    int width, height;
    std::vector<uint8_t> data;  // Contiguous RGB data
};

#endif // SCALERUTILS_HPP