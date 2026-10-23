#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <cstdint>

struct CubeInstance {
    float axis[3];
    float center[3];
    float angle;
    float color[3];
    float _pad;
};

class GpuContext {
public:
    SDL_Window* window = nullptr;
    SDL_GPUDevice* device = nullptr;
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    SDL_GPUBuffer* instanceBuffer = nullptr;
    SDL_GPUTransferBuffer* transferBuffer = nullptr;
    SDL_GPUTexture* depthTexture = nullptr;
    uint32_t depthWidth = 0, depthHeight = 0;

    bool init(SDL_Window* win);
    void draw(const std::vector<CubeInstance>& instances, const float viewProj[16], float scale);
};