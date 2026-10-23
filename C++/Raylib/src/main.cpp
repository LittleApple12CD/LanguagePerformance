#include "Game.h"
#include "raylib.h"

int main() {
    InitWindow(Game::WIDTH, Game::HEIGHT, "Perf Compare - cpp-raylib");
    ClearWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(0);

    Image icon = LoadImage("data/icon.png");
    if (icon.data != nullptr) {
        SetWindowIcon(icon);
        UnloadImage(icon);
    } else {
        TraceLog(LOG_WARNING, "icon.png not found");
    }

    Game game;
    game.init();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt <= 0) dt = 0.001f;

        game.update(dt);
        game.render();
    }

    game.shutdown();
    CloseWindow();
    return 0;
}