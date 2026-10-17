// Squares2D.cpp
#include "Squares2D.h"
#include "Game.h"
#include <cmath>
#include <algorithm>

Squares2D::Squares2D(const std::vector<std::vector<double>>& data, int n) : all(data), n(n) {
    x0 = Game::WIDTH / 2 - 200; x1 = Game::WIDTH / 2 + 200;
    y0 = Game::HEIGHT / 2 - 200; y1 = Game::HEIGHT / 2 + 200;
    reset();
}

void Squares2D::reset() {
    x.resize(n); y.resize(n); vx.resize(n); vy.resize(n); size.resize(n); colors.resize(n);
    for (int i = 0; i < n; i++) {
        x[i] = all[i][0] + x0; y[i] = all[i][1] + y0;
        vx[i] = all[i][2]; vy[i] = all[i][3]; size[i] = all[i][4];
        colors[i] = sf::Color((uint8_t)all[i][5], (uint8_t)all[i][6], (uint8_t)all[i][7]);
    }
}

void Squares2D::update(double dt) {
    for (int i = 0; i < n; i++) {
        x[i] += vx[i]; y[i] += vy[i];
        if (x[i] < x0 || x[i] > x1) { vx[i] = -vx[i]; x[i] = std::max((double)x0, std::min((double)x1, x[i])); }
        if (y[i] < y0 || y[i] > y1) { vy[i] = -vy[i]; y[i] = std::max((double)y0, std::min((double)y1, y[i])); }
    }
}

void Squares2D::draw(sf::RenderWindow& window) const {
    sf::RectangleShape shape;
    for (int i = 0; i < n; i++) {
        shape.setSize({(float)size[i], (float)size[i]});
        shape.setPosition({(float)x[i], (float)y[i]});
        shape.setFillColor(colors[i]);
        window.draw(shape);
    }
}