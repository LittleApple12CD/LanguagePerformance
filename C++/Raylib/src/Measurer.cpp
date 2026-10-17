// Measurer.cpp
#include "Measurer.h"
#include <algorithm>

void Measurer::start() {
    active = true;
    elapsed = 0.0;
    frames.clear();
    justFinished = false;
}

void Measurer::tick(double dt) {
    if (!active) return;
    frames.push_back(dt);
    elapsed += dt;
    if (elapsed >= duration) {
        active = false;
        justFinished = true;
    }
}

double Measurer::left() const {
    if (!active) return justFinished ? 0.0 : duration;
    return std::max(0.0, duration - elapsed);
}