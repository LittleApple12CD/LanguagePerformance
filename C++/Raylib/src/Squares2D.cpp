// Squares2D.cpp
#include "Squares2D.h"
#include "Game.h"
#include <cmath>

Squares2D::Squares2D(const std::vector<std::vector<double>>& data, int n)
    : all(data), n(n) {
    x0 = Game::WIDTH / 2 - 200;
    x1 = Game::WIDTH / 2 + 200;
    y0 = Game::HEIGHT / 2 - 200;
    y1 = Game::HEIGHT / 2 + 200;
    reset();
}

void Squares2D::reset() {
    x.resize(n); y.resize(n);
    vx.resize(n); vy.resize(n);
    size.resize(n);
    colors.resize(n);
    for (int i = 0; i < n; i++) {
        x[i] = all[i][0] + x0;
        y[i] = all[i][1] + y0;
        vx[i] = all[i][2];
        vy[i] = all[i][3];
        size[i] = all[i][4];
        colors[i] = Color{
            (unsigned char)all[i][5],
            (unsigned char)all[i][6],
            (unsigned char)all[i][7],
            255
        };
    }
}

void Squares2D::update(double dt) {
    for (int i = 0; i < n; i++) {
        x[i] += vx[i];
        y[i] += vy[i];
        if (x[i] < x0 || x[i] > x1) {
            vx[i] = -vx[i];
            x[i] = std::max((double)x0, std::min((double)x1, x[i]));
        }
        if (y[i] < y0 || y[i] > y1) {
            vy[i] = -vy[i];
            y[i] = std::max((double)y0, std::min((double)y1, y[i]));
        }
    }
}

void Squares2D::draw() const {
    for (int i = 0; i < n; i++) {
        DrawRectangle((int)x[i], (int)y[i], (int)size[i], (int)size[i], colors[i]);
    }
}