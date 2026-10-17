// Game.h
#pragma once
#include "NBody.h"
#include "Squares2D.h"
#include "Cubes3D.h"
#include "Measurer.h"
#include "raylib.h"
#include <memory>
#include <vector>
#include <string>

class Game {
public:
    static constexpr int WIDTH = 1280;
    static constexpr int HEIGHT = 720;
    static constexpr double DURATION = 30.0;
    static constexpr const char* STACK_NAME = "cpp-raylib";

    std::unique_ptr<NBody> t1;
    std::unique_ptr<Squares2D> t2;
    std::unique_ptr<Cubes3D> t3;
    int sceneIdx = 0;
    bool paused = false;
    bool hudShow = true;

    Font font{};

    static constexpr int FT_SIZE = 240;
    float frameTimes[FT_SIZE] = {0};
    int frameCount = 0;
    int frameHead = 0;
    float maxFps = 0.0f;
    float startupMs = 0.0f;
    bool firstFrame = true;

    Measurer measurer{DURATION};

    void init();
    void shutdown();
    void update(float dt);
    void render();
    void handleInput();
    void drawHud();

    void writeResult();
    float* stats(float out[4]);
    void resetMax();
};