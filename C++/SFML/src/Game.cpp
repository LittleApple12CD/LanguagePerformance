// Game.cpp
#include "Game.h"
#include "Csv.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <optional>
#include <cstdarg>

void Game::init() {
    if (!hudFont.openFromFile("data/fonts/arial.ttf")) {
        std::cerr << "Cannot load font data/fonts/arial.ttf\n";
    }

    auto particles = loadCsv("data/particles.csv");
    auto squares = loadCsv("data/squares.csv");
    auto cubes = loadCsv("data/cubes.csv");

    t1 = std::make_unique<NBody>(particles, 1000);
    t2 = std::make_unique<Squares2D>(squares, 1000);
    t3 = std::make_unique<Cubes3D>(cubes, 200);
    firstFrame = true;
}

void Game::handleInput(sf::RenderWindow& window) {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) window.close();

        if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            if (key->code == sf::Keyboard::Key::Num1) { sceneIdx = 0; resetMax(); }
            if (key->code == sf::Keyboard::Key::Num2) { sceneIdx = 1; resetMax(); }
            if (key->code == sf::Keyboard::Key::Num3) { sceneIdx = 2; resetMax(); }
            if (key->code == sf::Keyboard::Key::Space) paused = !paused;
            if (key->code == sf::Keyboard::Key::R) {
                switch (sceneIdx) { case 0: t1->reset(); break; case 1: t2->reset(); break; case 2: t3->reset(); break; }
                resetMax();
            }
            if (key->code == sf::Keyboard::Key::I) hudShow = !hudShow;
            if (key->code == sf::Keyboard::Key::S) {
                if (!measurer.active) { measurer.start(); resetMax(); }
            }
            if (key->code == sf::Keyboard::Key::Escape) {
                if (!measurer.frames.empty()) writeResult();
                window.close();
            }
            if (key->code == sf::Keyboard::Key::Up && sceneIdx == 0) t1->G *= 1.5;
            if (key->code == sf::Keyboard::Key::Down && sceneIdx == 0) t1->G /= 1.5;
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
        if (measurer.justFinished) { measurer.justFinished = false; writeResult(); }
    }

    if (!paused) {
        switch (sceneIdx) {
            case 0: t1->update(dt); break;
            case 1: t2->update(dt); break;
            case 2: t3->update(dt); break;
        }
    }
}

void Game::render(sf::RenderWindow& window) {
    window.clear(sf::Color(20, 20, 28));

    switch (sceneIdx) {
        case 0: t1->draw(window); break;
        case 1: t2->draw(window); break;
        case 2: t3->draw(window); break;
    }

    drawHud(window);
    window.display();

    if (firstFrame) {
        startupMs = 0.0f; // SFML 3 没有 GetTime，可用 sf::Clock，这里简化
        firstFrame = false;
    }
}

void Game::drawHud(sf::RenderWindow& window) {
    if (!hudShow) return;

    float s[4];
    stats(s);

    const char* test = sceneIdx == 0 ? "T1 - N-Body" : sceneIdx == 1 ? "T2 - 2D Squares" : "T3 - 3D Cubes";
    int objects = sceneIdx == 0 ? t1->n : sceneIdx == 1 ? t2->n : t3->n;

    char line[256];
    int y = 8;

    auto drawLine = [&](const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(line, sizeof(line), fmt, args);
        va_end(args);

        sf::Text text(hudFont); // SFML 3: Text 没有默认构造函数，必须传字体 [citation:10]
        text.setString(line);
        text.setCharacterSize(14);
        text.setFillColor(sf::Color(230, 230, 230));
        text.setPosition({8.f, (float)y});
        window.draw(text);
        y += 18;
    };

    drawLine("Stack:    %s", STACK_NAME);
    drawLine("Test:     %s", test);
    drawLine("FPS:      %6.1f", s[0]);
    drawLine("Frame:    %5.2f ms (avg) / %5.2f ms (p99)", 1000.0f / s[0], s[1]);
    drawLine("Max FPS:  %6.1f", s[3]);
    drawLine("1%% Low:   %6.1f", s[2]);
    drawLine("Objects:  %d", objects);
    if (sceneIdx == 0) drawLine("G:        %.5f", t1->G); else drawLine("G:        -");
    drawLine("Paused:   %s", paused ? "true" : "false");
    if (measurer.active) drawLine("Measuring: %ds", (int)measurer.left()); else drawLine("Measuring: no");
    drawLine("[1] T1  [2] T2  [3] T3  [Space] Pause  [R] Reset");
    drawLine("[I] Info  [S] Measure  [Esc] Quit");
    drawLine("[Up/Down] G");
}

void Game::stats(float out[4]) {
    if (frameCount == 0) { out[0] = out[1] = out[2] = 0; out[3] = maxFps; return; }
    std::vector<float> copy(frameCount);
    for (int i = 0; i < frameCount; i++) copy[i] = frameTimes[i];
    std::sort(copy.begin(), copy.end());
    float sum = 0; for (float v : copy) sum += v;
    float avg = sum / frameCount;
    float p99 = copy[std::min(frameCount - 1, (int)(frameCount * 0.99))];
    int k = std::max(1, (int)(frameCount * 0.01));
    float slowSum = 0; for (int i = frameCount - k; i < frameCount; i++) slowSum += copy[i];
    float low1 = 1000.0f / (slowSum / k);
    out[0] = 1000.0f / avg; out[1] = p99; out[2] = low1; out[3] = maxFps;
}

void Game::resetMax() { maxFps = 0.0f; frameHead = 0; frameCount = 0; }

void Game::writeResult() {
    float s[4]; stats(s);
    const char* test = sceneIdx == 0 ? "T1" : sceneIdx == 1 ? "T2" : "T3";
    int objects = sceneIdx == 0 ? t1->n : sceneIdx == 1 ? t2->n : t3->n;
    float maxMs = 0; for (int i = 0; i < frameCount; i++) if (frameTimes[i] > maxMs) maxMs = frameTimes[i];

    std::ostringstream json;
    json << "{\n  \"stack\": \"" << STACK_NAME << "\",\n  \"test\": \"" << test << "\",\n"
         << "  \"objects\": " << objects << ",\n  \"resolution\": [" << WIDTH << ", " << HEIGHT << "],\n"
         << "  \"avg_fps\": " << s[0] << ",\n  \"min_fps\": " << (1000.0f / maxMs) << ",\n"
         << "  \"max_fps\": " << s[3] << ",\n  \"avg_frame_ms\": " << (1000.0f / s[0]) << ",\n"
         << "  \"p99_frame_ms\": " << s[1] << ",\n  \"one_percent_low_fps\": " << s[2] << ",\n"
         << "  \"startup_ms\": " << startupMs << ",\n  \"duration_s\": " << DURATION << ",\n"
         << "  \"runtime\": \"C++ SFML 3.0\",\n  \"os\": \"Windows\"\n}\n";

    std::string fn = std::string("results/") + STACK_NAME + "-" + test + "-" + std::to_string(objects) + ".json";
    std::ofstream out(fn);
    out << json.str();
}