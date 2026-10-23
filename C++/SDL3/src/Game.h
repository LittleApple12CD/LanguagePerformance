// Game.h
#pragma once
#include "NBody.h"
#include "Squares2D.h"
#include "Cubes3D.h"
#include "Measurer.h"
#include "GpuContext.h"
#include <memory>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

class Game {
public:
    static constexpr int WIDTH = 1280;
    static constexpr int HEIGHT = 720;
    static constexpr double DURATION = 30.0;
    static constexpr const char* STACK_NAME = "cpp-sdl3";
	
    SDL_Window* window2D = nullptr;
    SDL_Window* window3D = nullptr;
    GpuContext gpu3D;
    int currentWindow = 1;

    std::unique_ptr<NBody> t1;
    std::unique_ptr<Squares2D> t2;
    std::unique_ptr<Cubes3D> t3;
    int sceneIdx = 0;
    bool paused = false;
    bool hudShow = true;
    bool quit = false;

    static constexpr int FT_SIZE = 240;
    float frameTimes[FT_SIZE] = {0};
    int frameCount = 0;
    int frameHead = 0;
    float maxFps = 0.0f;
    float startupMs = 0.0f;
    bool firstFrame = true;

    Measurer measurer{DURATION};
    TTF_Font* hudFont = nullptr;

    float titleTimer = 0.0f;

    void init();
    void update(float dt);
    void render(SDL_Renderer* renderer);
    void handleInput(const SDL_Event& e);
    void drawHud(SDL_Renderer* renderer);

    void writeResult();
    void stats(float out[4]);
    void resetMax();
};