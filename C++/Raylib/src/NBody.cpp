// NBody.cpp
#include "NBody.h"
#include "Game.h"
#include <cmath>

NBody::NBody(const std::vector<std::vector<double>>& data, int n)
    : all(data), n(n) {
    reset();
}

void NBody::reset() {
    x.resize(n); y.resize(n);
    vx.resize(n); vy.resize(n);
    mass.resize(n);
    for (int i = 0; i < n; i++) {
        x[i] = all[i][0];
        y[i] = all[i][1];
        vx[i] = all[i][2];
        vy[i] = all[i][3];
        mass[i] = all[i][4];
    }
}

void NBody::update(double frameDt) {
    std::vector<double> ax(n, 0.0), ay(n, 0.0);
    for (int i = 0; i < n; i++) {
        double axi = 0, ayi = 0;
        double xi = x[i], yi = y[i];
        for (int j = 0; j < n; j++) {
            if (i == j) continue;
            double dx = x[j] - xi;
            double dy = y[j] - yi;
            double d2 = dx * dx + dy * dy;
            if (d2 < 1e-12) continue;
            double inv = 1.0 / d2;
            double f = G * inv * std::sqrt(inv) * mass[j];
            axi += f * dx;
            ayi += f * dy;
        }
        ax[i] = axi; ay[i] = ayi;
    }
    for (int i = 0; i < n; i++) {
        double a2 = ax[i] * ax[i] + ay[i] * ay[i];
        if (a2 > aMax * aMax) {
            double s = aMax / std::sqrt(a2);
            ax[i] *= s; ay[i] *= s;
        }
        vx[i] += ax[i] * dt;
        vy[i] += ay[i] * dt;
        x[i] += vx[i] * dt;
        y[i] += vy[i] * dt;
    }
}

void NBody::draw() const {
    double cx = Game::WIDTH / 2.0;
    double cy = Game::HEIGHT / 2.0;
    int r = (n <= 1000) ? 2 : 1;
    for (int i = 0; i < n; i++) {
        double px = x[i] * scale * Game::WIDTH / 2.0 + cx;
        double py = -y[i] * scale * Game::WIDTH / 2.0 + cy;
        if (px < 0 || px >= Game::WIDTH || py < 0 || py >= Game::HEIGHT) continue;
        DrawCircle((int)px, (int)py, (float)r, Color{180, 220, 255, 255});
    }
}