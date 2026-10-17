#include "Game.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

int main(int argc, char* argv[]) {
    SDL_SetHint(SDL_HINT_WINDOWS_INTRESOURCE_ICON, "IDI_ICON1");
    SDL_SetHint(SDL_HINT_WINDOWS_INTRESOURCE_ICON_SMALL, "IDI_ICON1");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    if (!TTF_Init()) {
        SDL_Log("TTF_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Perf Compare - cpp-sdl3",
        Game::WIDTH, Game::HEIGHT,
        0
    );
    if (!window) { SDL_Log("CreateWindow failed: %s", SDL_GetError()); return 1; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) { SDL_Log("CreateRenderer failed: %s", SDL_GetError()); return 1; }

    SDL_SetRenderVSync(renderer, 0);

    SDL_Surface* icon = SDL_LoadBMP("data/icon.bmp");
    if (icon) {
        SDL_SetWindowIcon(window, icon);
        SDL_DestroySurface(icon);
    } else {
        SDL_Log("icon.bmp not found: %s", SDL_GetError());
    }

    Game game;
    game.init();

    Uint64 last = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();

    while (!game.quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) game.handleInput(e);

        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)((now - last) / (double)freq);
        last = now;
        if (dt <= 0) dt = 0.001f;

        game.update(dt);
        game.render(renderer);
    }

    TTF_CloseFont(game.hudFont);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}