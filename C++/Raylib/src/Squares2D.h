// Squares2D.h
#pragma once
#include <vector>
#include "raylib.h"

class Squares2D {
public:
    int n;
    std::vector<double> x, y, vx, vy, size;
    std::vector<Color> colors;
    int x0, x1, y0, y1;

    std::vector<std::vector<double>> all;

    Squares2D(const std::vector<std::vector<double>>& data, int n);
    void reset();
    void update(double dt);
    void draw() const;
};