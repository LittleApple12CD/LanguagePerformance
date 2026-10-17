// Squares2D.cpp
#include "Squares2D.h"
#include "Game.h"
#include <cmath>
#include <algorithm>

Squares2D::Squares2D(const std::vector<std::vector<double>>& data, int n) : all(data), n(n) {
    x0=Game::WIDTH/2-200; x1=Game::WIDTH/2+200;
    y0=Game::HEIGHT/2-200; y1=Game::HEIGHT/2+200;
    reset();
}

void Squares2D::reset() {
    x.resize(n); y.resize(n); vx.resize(n); vy.resize(n); size.resize(n); colors.resize(n);
    for (int i = 0; i < n; i++) {
        x[i]=all[i][0]+x0; y[i]=all[i][1]+y0;
        vx[i]=all[i][2]; vy[i]=all[i][3]; size[i]=all[i][4];
        colors[i] = SDL_Color{(Uint8)all[i][5], (Uint8)all[i][6], (Uint8)all[i][7], 255};
    }
}

void Squares2D::update(double dt) {
    for (int i = 0; i < n; i++) {
        x[i]+=vx[i]; y[i]+=vy[i];
        if (x[i]<x0||x[i]>x1) { vx[i]=-vx[i]; x[i]=std::max((double)x0, std::min((double)x1, x[i])); }
        if (y[i]<y0||y[i]>y1) { vy[i]=-vy[i]; y[i]=std::max((double)y0, std::min((double)y1, y[i])); }
    }
}

void Squares2D::draw(SDL_Renderer* renderer) const {
    if (n <= 0) return;

    std::vector<SDL_Vertex> verts(n * 6);
    std::vector<int> indices(n * 6);

    for (int i = 0; i < n; i++) {
        float px = (float)x[i];
        float py = (float)y[i];
        float sz = (float)size[i];
        float r = (float)colors[i].r / 255.0f;
        float g = (float)colors[i].g / 255.0f;
        float b = (float)colors[i].b / 255.0f;

        int baseV = i * 6;
        int baseI = i * 6;

        verts[baseV + 0] = {{px,      py,      }, {r, g, b, 1.0f}, {0, 0}};
        verts[baseV + 1] = {{px + sz, py,      }, {r, g, b, 1.0f}, {0, 0}};
        verts[baseV + 2] = {{px,      py + sz, }, {r, g, b, 1.0f}, {0, 0}};
        verts[baseV + 3] = {{px + sz, py + sz, }, {r, g, b, 1.0f}, {0, 0}};
        verts[baseV + 4] = {{px,      py + sz, }, {r, g, b, 1.0f}, {0, 0}};
        verts[baseV + 5] = {{px + sz, py,      }, {r, g, b, 1.0f}, {0, 0}};

        indices[baseI + 0] = baseV + 0;
        indices[baseI + 1] = baseV + 1;
        indices[baseI + 2] = baseV + 2;
        
        indices[baseI + 3] = baseV + 2;
        indices[baseI + 4] = baseV + 1;
        indices[baseI + 5] = baseV + 3;
    }

    SDL_RenderGeometry(renderer, nullptr, verts.data(), (int)verts.size(), indices.data(), (int)indices.size());
}