// NBody.cpp
#include "NBody.h"
#include "Game.h"
#include <cmath>
#include <algorithm>

NBody::NBody(const std::vector<std::vector<double>>& data, int n) : all(data), n(n) { reset(); }

void NBody::reset() {
    x.resize(n); y.resize(n); vx.resize(n); vy.resize(n); mass.resize(n);
    ax.resize(n); ay.resize(n);
    for (int i = 0; i < n; i++) {
        x[i] = all[i][0]; y[i] = all[i][1]; vx[i] = all[i][2]; vy[i] = all[i][3]; mass[i] = all[i][4];
    }
}

void NBody::update(double frameDt) {
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++) {
        double axi = 0, ayi = 0, xi = x[i], yi = y[i];
        for (int j = 0; j < n; j++) {
            double dx = x[j] - xi, dy = y[j] - yi;
            double d2 = dx * dx + dy * dy + 1e-12;
            double inv = 1.0 / d2;
            double f = G * inv * std::sqrt(inv) * mass[j];
            axi += f * dx; ayi += f * dy;
        }
        ax[i] = axi; ay[i] = ayi;
    }
    for (int i = 0; i < n; i++) {
        double a2 = ax[i] * ax[i] + ay[i] * ay[i];
        if (a2 > aMax * aMax) { double s = aMax / std::sqrt(a2); ax[i] *= s; ay[i] *= s; }
        vx[i] += ax[i] * frameDt; vy[i] += ay[i] * frameDt; 
        x[i] += vx[i] * frameDt; y[i] += vy[i] * frameDt;
    }
}

void NBody::draw(sf::RenderWindow& window) const {
    sf::VertexArray points(sf::PrimitiveType::Points, n);
    double cx = Game::WIDTH / 2.0, cy = Game::HEIGHT / 2.0;
    sf::Color col(180, 220, 255);
    sf::Vector2f texCoords(0.f, 0.f);

    for (int i = 0; i < n; i++) {
        float px = (float)(x[i] * scale * Game::WIDTH / 2.0 + cx);
        float py = (float)(-y[i] * scale * Game::WIDTH / 2.0 + cy);
        
        points[i].position = sf::Vector2f(px, py);
        points[i].color = col;
        points[i].texCoords = texCoords;
    }
    window.draw(points);
}