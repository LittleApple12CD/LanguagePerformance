#pragma once
#include <vector>
#include <SFML/Graphics.hpp>

class NBody {
public:
    int n;
    std::vector<double> x, y, vx, vy, mass;
    std::vector<double> ax, ay;
    double G = 0.0002;
    double dt = 0.016;
    double aMax = 3.0;
    double scale = 0.4;
    std::vector<std::vector<double>> all;

    NBody(const std::vector<std::vector<double>>& data, int n);
    void reset();
    void update(double frameDt);
    void draw(sf::RenderWindow& window) const;
};