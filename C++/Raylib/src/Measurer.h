// Measurer.h
#pragma once
#include <vector>

class Measurer {
public:
    double duration;
    bool active = false;
    double elapsed = 0.0;
    std::vector<double> frames;
    bool justFinished = false;

    Measurer(double d) : duration(d) {}
    void start();
    void tick(double dt);
    double left() const;
};