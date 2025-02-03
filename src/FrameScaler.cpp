#include "FrameScaler.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <immintrin.h>  // For SSE/AVX

constexpr int BLOCK_SIZE = 16;  // Process pixels in blocks
constexpr float PI = 3.14159265358979323846f;
constexpr int LANCZOS_RADIUS = 2;  // Reduced radius for speed
constexpr int LANCZOS_TABLE_SIZE = 1024;

class LanczosFilter {
public:
    LanczosFilter() {
        table.resize(LANCZOS_TABLE_SIZE);
        for (int i = 0; i < LANCZOS_TABLE_SIZE; ++i) {
            float x = static_cast<float>(i) * LANCZOS_RADIUS / LANCZOS_TABLE_SIZE;
            if (x == 0.0f) table[i] = 1.0f;
            else {
                float px = PI * x;
                table[i] = LANCZOS_RADIUS * std::sin(px) * std::sin(px / LANCZOS_RADIUS) / (px * px);
            }
        }
    }

    float operator()(float x) const {
        x = std::abs(x);
        if (x >= LANCZOS_RADIUS) return 0.0f;
        int index = static_cast<int>(x * LANCZOS_TABLE_SIZE / LANCZOS_RADIUS);
        return table[index];
    }

private:
    std::vector<float> table;
};

static const LanczosFilter lanczos;

// Fast pixel processing using SSE
inline void processPixelBlock(const uint8_t* src, int stride, float* weights, 
                            int count, __m128& sumR, __m128& sumG, __m128& sumB) {
    __m128 w = _mm_load_ps(weights);
    for (int i = 0; i < count; i += 4) {
        __m128 r = _mm_set_ps(src[0], src[3], src[6], src[9]);
        __m128 g = _mm_set_ps(src[1], src[4], src[7], src[10]);
        __m128 b = _mm_set_ps(src[2], src[5], src[8], src[11]);
        
        sumR = _mm_add_ps(sumR, _mm_mul_ps(r, w));
        sumG = _mm_add_ps(sumG, _mm_mul_ps(g, w));
        sumB = _mm_add_ps(sumB, _mm_mul_ps(b, w));
        
        src += 12;  // Move to next 4 pixels
    }
}

inline void getFilteredPixel(const uint8_t* data, int stride, float x, float y,
                           int srcWidth, int srcHeight, uint8_t* output) {
    alignas(16) float sumR[4] = {0}, sumG[4] = {0}, sumB[4] = {0};
    float weightSum = 0.0f;

    int x1 = std::max(0, static_cast<int>(x - LANCZOS_RADIUS));
    int x2 = std::min(srcWidth - 1, static_cast<int>(x + LANCZOS_RADIUS));
    int y1 = std::max(0, static_cast<int>(y - LANCZOS_RADIUS));
    int y2 = std::min(srcHeight - 1, static_cast<int>(y + LANCZOS_RADIUS));

    alignas(16) float yweights[8];  // Aligned for SSE
    for (int sy = y1; sy <= y2; sy++) {
        yweights[sy - y1] = lanczos(y - sy);
    }

    __m128 sumR_v = _mm_setzero_ps();
    __m128 sumG_v = _mm_setzero_ps();
    __m128 sumB_v = _mm_setzero_ps();

    for (int sy = y1; sy <= y2; sy++) {
        float wy = yweights[sy - y1];
        const uint8_t* row = data + sy * stride;
        alignas(16) float xweights[8];

        for (int sx = x1; sx <= x2; sx += 4) {
            int count = std::min(4, x2 - sx + 1);
            for (int i = 0; i < count; i++) {
                xweights[i] = lanczos(x - (sx + i)) * wy;
                weightSum += xweights[i];
            }
            processPixelBlock(row + sx * 3, stride, xweights, count, sumR_v, sumG_v, sumB_v);
        }
    }

    _mm_store_ps(sumR, sumR_v);
    _mm_store_ps(sumG, sumG_v);
    _mm_store_ps(sumB, sumB_v);

    if (weightSum > 0.0f) {
        float invWeight = 1.0f / weightSum;
        output[0] = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, (sumR[0] + sumR[1] + sumR[2] + sumR[3]) * invWeight)));
        output[1] = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, (sumG[0] + sumG[1] + sumG[2] + sumG[3]) * invWeight)));
        output[2] = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, (sumB[0] + sumB[1] + sumB[2] + sumB[3]) * invWeight)));
    }
}

Frame FrameScaler::resize(const Frame& inputFrame, int newWidth, int newHeight) {
    if (inputFrame.empty()) {
        std::cerr << "Error: Input frame is empty\n";
        return Frame();
    }

    int srcWidth = inputFrame.getWidth();
    int srcHeight = inputFrame.getHeight();
    Frame outputFrame(newHeight, newWidth);

    const uint8_t* srcData = inputFrame.getData();
    uint8_t* dstData = outputFrame.getData();
    int srcStride = inputFrame.getStride();
    int dstStride = outputFrame.getStride();

    float scaleX = static_cast<float>(srcWidth) / newWidth;
    float scaleY = static_cast<float>(srcHeight) / newHeight;

    // Process in blocks for better cache utilization
    constexpr int BLOCK_HEIGHT = 32;
    #pragma omp parallel for collapse(2) schedule(dynamic, 1)
    for (int by = 0; by < newHeight; by += BLOCK_HEIGHT) {
        for (int bx = 0; bx < newWidth; bx += BLOCK_SIZE) {
            int endY = std::min(by + BLOCK_HEIGHT, newHeight);
            int endX = std::min(bx + BLOCK_SIZE, newWidth);
            
            for (int y = by; y < endY; y++) {
                float srcY = (y + 0.5f) * scaleY;
                uint8_t* row = dstData + y * dstStride;
                
                for (int x = bx; x < endX; x++) {
                    float srcX = (x + 0.5f) * scaleX;
                    getFilteredPixel(srcData, srcStride, srcX, srcY, srcWidth, srcHeight, row + x * 3);
                }
            }
        }
    }

    return outputFrame;
}