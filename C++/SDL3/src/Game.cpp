// Game.cpp
#include "Game.h"
#include "Csv.h"
#include <cmath>
#include <cstdio>
#include <cstdarg>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

void Game::init() {
    hudFont = TTF_OpenFont("data/fonts/arial.ttf", 14);
    if (!hudFont) {
        std::cerr << "Cannot load font: " << SDL_GetError() << "\n";
    }

    auto particles = loadCsv("data/particles.csv");
    auto squares   = loadCsv("data/squares.csv");
    auto cubes     = loadCsv("data/cubes.csv");

    t1 = std::make_unique<NBody>(particles, 1000);
    t2 = std::make_unique<Squares2D>(squares, 1000);
    t3 = std::make_unique<Cubes3D>(cubes, 200);
    firstFrame = true;
}

void Game::handleInput(const SDL_Event& e) {
    if (e.type == SDL_EVENT_QUIT) quit = true;
    if (e.type == SDL_EVENT_KEY_DOWN) {
        switch (e.key.key) {
            case SDLK_1: sceneIdx=0; resetMax(); break;
            case SDLK_2: sceneIdx=1; resetMax(); break;
            case SDLK_3: sceneIdx=2; resetMax(); break;
            case SDLK_SPACE: paused = !paused; break;
            case SDLK_R:
                switch (sceneIdx) { case 0: t1->reset(); break; case 1: t2->reset(); break; case 2: t3->reset(); break; }
                resetMax();
                break;
            case SDLK_I: hudShow = !hudShow; break;
            case SDLK_S:
                if (!measurer.active) { measurer.start(); resetMax(); }
                break;
            case SDLK_ESCAPE:
                if (!measurer.frames.empty()) writeResult();
                quit = true;
                break;
            case SDLK_UP:
                if (sceneIdx==0) { t1->G *= 1.5; SDL_Log("G = %f", t1->G); }
                break;
            case SDLK_DOWN:
                if (sceneIdx==0) { t1->G /= 1.5; SDL_Log("G = %f", t1->G); }
                break;
        }
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
    }

    if (measurer.justFinished) {
        measurer.justFinished = false;
        writeResult();
    }

    if (!paused) {
        switch (sceneIdx) {
            case 0: t1->update(dt); break;
            case 1: t2->update(dt); break;
            case 2: t3->update(dt); break;
        }
    }
}

void Game::render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 20, 20, 28, 255);
    SDL_RenderClear(renderer);

    switch (sceneIdx) {
        case 0: t1->draw(renderer); break;
        case 1: t2->draw(renderer); break;
        case 2: t3->draw(renderer); break;
    }

    drawHud(renderer);
    SDL_RenderPresent(renderer);

    if (firstFrame) { startupMs = 0.0f; firstFrame = false; }
}

void Game::drawHud(SDL_Renderer* renderer) {
    if (!hudShow || !hudFont) return;

    float s[4]; stats(s);
    const char* test = sceneIdx==0 ? "T1 - N-Body" : sceneIdx==1 ? "T2 - 2D Squares" : "T3 - 3D Cubes";
    int objects = sceneIdx==0 ? t1->n : sceneIdx==1 ? t2->n : t3->n;

    char buf[256];
    int y = 8;

    auto drawLine = [&](const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        SDL_Color white{230, 230, 230, 255};
        SDL_Surface* surf = TTF_RenderText_Blended(hudFont, buf, 0, white);
        if (surf) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
            if (tex) {
                SDL_FRect dst{8.f, (float)y, (float)surf->w, (float)surf->h};
                SDL_RenderTexture(renderer, tex, nullptr, &dst);
                SDL_DestroyTexture(tex);
            }
            SDL_DestroySurface(surf);
        }
        y += 18;
    };

    drawLine("Stack:    %s", STACK_NAME);
    drawLine("Test:     %s", test);
    drawLine("FPS:      %6.1f", s[0]);
    drawLine("Frame:    %5.2f ms (avg) / %5.2f ms (p99)", 1000.0f/s[0], s[1]);
    drawLine("Max FPS:  %6.1f", s[3]);
    drawLine("1%% Low:   %6.1f", s[2]);
    drawLine("Objects:  %d", objects);
    if (sceneIdx==0) drawLine("G:        %.5f", t1->G); else drawLine("G:        -");
    drawLine("Paused:   %s", paused ? "true" : "false");
    if (measurer.active) drawLine("Measuring: %ds", (int)measurer.left()); else drawLine("Measuring: no");
    drawLine("[1] T1  [2] T2  [3] T3  [Space] Pause  [R] Reset");
    drawLine("[I] Info  [S] Measure  [Esc] Quit");
    drawLine("[Up/Down] G");
}

void Game::stats(float out[4]) {
    if (frameCount == 0) { out[0]=out[1]=out[2]=0; out[3]=maxFps; return; }
    std::vector<float> copy(frameCount);
    for (int i = 0; i < frameCount; i++) copy[i] = frameTimes[i];
    std::sort(copy.begin(), copy.end());
    float sum = 0; for (float v : copy) sum += v;
    float avg = sum / frameCount;
    float p99 = copy[std::min(frameCount-1, (int)(frameCount*0.99))];
    int k = std::max(1, (int)(frameCount*0.01));
    float slowSum = 0; for (int i = frameCount-k; i < frameCount; i++) slowSum += copy[i];
    float low1 = 1000.0f / (slowSum/k);
    out[0]=1000.0f/avg; out[1]=p99; out[2]=low1; out[3]=maxFps;
}

void Game::resetMax() { maxFps = 0.0f; frameHead = 0; frameCount = 0; }

void Game::writeResult() {
    std::filesystem::create_directories("results");

    float s[4]; stats(s);
    const char* test = sceneIdx==0 ? "T1" : sceneIdx==1 ? "T2" : "T3";
    int objects = sceneIdx==0 ? t1->n : sceneIdx==1 ? t2->n : t3->n;
    float maxMs = 0;
    for (int i = 0; i < frameCount; i++)
        if (frameTimes[i] > maxMs) maxMs = frameTimes[i];

    std::ostringstream json;
    json << "{\n  \"stack\": \"" << STACK_NAME << "\",\n  \"test\": \"" << test << "\",\n"
         << "  \"objects\": " << objects << ",\n  \"resolution\": [" << WIDTH << ", " << HEIGHT << "],\n"
         << "  \"avg_fps\": " << s[0] << ",\n  \"min_fps\": " << (1000.0f/maxMs) << ",\n"
         << "  \"max_fps\": " << s[3] << ",\n  \"avg_frame_ms\": " << (1000.0f/s[0]) << ",\n"
         << "  \"p99_frame_ms\": " << s[1] << ",\n  \"one_percent_low_fps\": " << s[2] << ",\n"
         << "  \"startup_ms\": " << startupMs << ",\n  \"duration_s\": " << DURATION << ",\n"
         << "  \"runtime\": \"C++ SDL3\",\n  \"os\": \"Windows\"\n}\n";

    std::string fn = std::string("results/") + STACK_NAME + "-" + test + "-" + std::to_string(objects) + ".json";
    std::ofstream out(fn);
    if (!out.is_open()) {
        std::cerr << "Cannot open " << fn << "\n";
        return;
    }
    out << json.str();
    out.close();
    std::cout << "Wrote " << fn << "\n";
}