#pragma once
#include <vector>
#include "raylib.h"

class Cubes3D {
public:
    int n;
    std::vector<double> px, py, pz;
    std::vector<double> ax, ay, az;
    std::vector<double> speed;
    std::vector<Color> colors;
    std::vector<double> angles;

    std::vector<std::vector<double>> all;

    Camera3D camera;

    Cubes3D(const std::vector<std::vector<double>>& data, int n);
    void reset();
    void update(double dt);
    void draw();
};