// Game.cpp
#include "Game.h"
#include "Csv.h"
#include "raylib.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <algorithm>
#include <fstream>
#include <sstream>

void Game::init() {
    font = LoadFontEx("data/fonts/arial.ttf", 48, nullptr, 0);
    if (font.texture.id == 0) {
        TraceLog(LOG_WARNING, "arial.ttf not found, using default font");
        font = GetFontDefault();
    }

    auto particles = loadCsv("data/particles.csv");
    auto squares = loadCsv("data/squares.csv");
    auto cubes = loadCsv("data/cubes.csv");

    t1 = std::make_unique<NBody>(particles, 1000);
    t2 = std::make_unique<Squares2D>(squares, 1000);
    t3 = std::make_unique<Cubes3D>(cubes, 200);

    firstFrame = true;
}

void Game::shutdown() {
    if (font.texture.id != 0 && font.texture.id != GetFontDefault().texture.id) {
        UnloadFont(font);
        font = GetFontDefault();
    }
}

void Game::handleInput() {
    if (IsKeyPressed(KEY_ONE)) { sceneIdx = 0; resetMax(); }
    if (IsKeyPressed(KEY_TWO)) { sceneIdx = 1; resetMax(); }
    if (IsKeyPressed(KEY_THREE)) { sceneIdx = 2; resetMax(); }
    if (IsKeyPressed(KEY_SPACE)) paused = !paused;
    if (IsKeyPressed(KEY_R)) {
        switch (sceneIdx) {
            case 0: t1->reset(); break;
            case 1: t2->reset(); break;
            case 2: t3->reset(); break;
        }
        resetMax();
    }
    if (IsKeyPressed(KEY_I)) hudShow = !hudShow;
    if (IsKeyPressed(KEY_S)) {
        if (!measurer.active) {
            measurer.start();
            resetMax();
            TraceLog(LOG_INFO, "Measuring started...");
        }
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (!measurer.frames.empty()) writeResult();
        CloseWindow();
    }
    if (IsKeyPressed(KEY_UP) && sceneIdx == 0) {
        t1->G *= 1.5;
        TraceLog(LOG_INFO, "G = %f", t1->G);
    }
    if (IsKeyPressed(KEY_DOWN) && sceneIdx == 0) {
        t1->G /= 1.5;
        TraceLog(LOG_INFO, "G = %f", t1->G);
    }
}

void Game::update(float dt) {
    float dtMs = dt * 1000.0f;
    frameTimes[frameHead] = dtMs;
    frameHead = (frameHead + 1) % FT_SIZE;
    if (frameCount < FT_SIZE) frameCount++;
    float instFps = 1000.0f / dtMs;
    if (instFps > maxFps) maxFps = instFps;

    if (measurer.active) {
        measurer.tick(dt);
        if (measurer.justFinished) {
            measurer.justFinished = false;
            writeResult();
        }
    }

    handleInput();

    if (!paused) {
        switch (sceneIdx) {
            case 0: t1->update(dt); break;
            case 1: t2->update(dt); break;
            case 2: t3->update(dt); break;
        }
    }
}

void Game::render() {
    BeginDrawing();
    ClearBackground(Color{20, 20, 28, 255});

    switch (sceneIdx) {
        case 0: t1->draw(); break;
        case 1: t2->draw(); break;
        case 2: t3->draw(); break;
    }

    drawHud();

    EndDrawing();

    if (firstFrame) {
        startupMs = (float)(GetTime() * 1000.0);
        firstFrame = false;
    }
}

void Game::drawHud() {
    if (!hudShow) return;

    float s[4];
    stats(s);

    const char* test = sceneIdx == 0 ? "T1 - N-Body"
                     : sceneIdx == 1 ? "T2 - 2D Squares"
                     : "T3 - 3D Cubes";
    int objects = sceneIdx == 0 ? t1->n
                : sceneIdx == 1 ? t2->n
                : t3->n;

    char line[256];
    int y = 8;
    Color white{230, 230, 230, 255};

    auto drawLine = [&](const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(line, sizeof(line), fmt, args);
        va_end(args);
        DrawTextEx(font, line, {(float)8, (float)y}, 18, 0, white);
        y += 18;
    };

    drawLine("Stack:    %s", STACK_NAME);
    drawLine("Test:     %s", test);
    drawLine("FPS:      %6.1f", s[0]);
    drawLine("Frame:    %5.2f ms (avg) / %5.2f ms (p99)", 1000.0f / s[0], s[1]);
    drawLine("Max FPS:  %6.1f", s[3]);
    drawLine("1%% Low:   %6.1f", s[2]);
    drawLine("Objects:  %d", objects);
    if (sceneIdx == 0) drawLine("G:        %.5f", t1->G);
    else               drawLine("G:        -");
    drawLine("Paused:   %s", paused ? "true" : "false");
    if (measurer.active) drawLine("Measuring: %ds", (int)measurer.left());
    else                 drawLine("Measuring: no");
    drawLine("[1] T1  [2] T2  [3] T3  [Space] Pause  [R] Reset");
    drawLine("[I] Info  [S] Measure  [Esc] Quit");
    drawLine("[Up/Down] G");
}

float* Game::stats(float out[4]) {
    if (frameCount == 0) {
        out[0] = out[1] = out[2] = 0;
        out[3] = maxFps;
        return out;
    }
    std::vector<float> copy(frameCount);
    for (int i = 0; i < frameCount; i++) copy[i] = frameTimes[i];
    std::sort(copy.begin(), copy.end());
    float sum = 0;
    for (float v : copy) sum += v;
    float avg = sum / frameCount;
    float p99 = copy[std::min(frameCount - 1, (int)(frameCount * 0.99))];
    int k = std::max(1, (int)(frameCount * 0.01));
    float slowSum = 0;
    for (int i = frameCount - k; i < frameCount; i++) slowSum += copy[i];
    float low1 = 1000.0f / (slowSum / k);
    out[0] = 1000.0f / avg;
    out[1] = p99;
    out[2] = low1;
    out[3] = maxFps;
    return out;
}

void Game::resetMax() {
    maxFps = 0.0f;
    frameHead = 0;
    frameCount = 0;
}

void Game::writeResult() {
    float s[4];
    stats(s);
    const char* test = sceneIdx == 0 ? "T1" : sceneIdx == 1 ? "T2" : "T3";
    int objects = sceneIdx == 0 ? t1->n : sceneIdx == 1 ? t2->n : t3->n;
    float maxMs = 0;
    for (int i = 0; i < frameCount; i++) if (frameTimes[i] > maxMs) maxMs = frameTimes[i];

    std::ostringstream json;
    json << "{\n"
         << "  \"stack\": \"" << STACK_NAME << "\",\n"
         << "  \"test\": \"" << test << "\",\n"
         << "  \"objects\": " << objects << ",\n"
         << "  \"resolution\": [" << WIDTH << ", " << HEIGHT << "],\n"
         << "  \"avg_fps\": " << s[0] << ",\n"
         << "  \"min_fps\": " << (1000.0f / maxMs) << ",\n"
         << "  \"max_fps\": " << s[3] << ",\n"
         << "  \"avg_frame_ms\": " << (1000.0f / s[0]) << ",\n"
         << "  \"p99_frame_ms\": " << s[1] << ",\n"
         << "  \"one_percent_low_fps\": " << s[2] << ",\n"
         << "  \"startup_ms\": " << startupMs << ",\n"
         << "  \"duration_s\": " << DURATION << ",\n"
         << "  \"runtime\": \"C++ raylib\",\n"
         << "  \"os\": \"" << "Windows" << "\"\n"
         << "}\n";

    system("if not exist results mkdir results");
    std::string fn = std::string("results/") + STACK_NAME + "-" + test + "-"
                   + std::to_string(objects) + ".json";
    std::ofstream out(fn);
    out << json.str();
    TraceLog(LOG_INFO, "Wrote %s", fn.c_str());
}