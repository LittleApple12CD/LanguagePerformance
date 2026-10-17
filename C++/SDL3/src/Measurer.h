#pragma once
#include <vector>
#include <algorithm>

class Measurer {
public:
    double duration;
    bool active = false;
    double elapsed = 0.0;
    std::vector<double> frames;
    bool justFinished = false;

    Measurer(double d) : duration(d) {}
    void start() {
        active = true; elapsed = 0.0; frames.clear(); justFinished = false;
    }
    void tick(double dt) {
        if (!active) return;
        frames.push_back(dt);
        elapsed += dt;
        if (elapsed >= duration) { active = false; justFinished = true; }
    }
    double left() const {
        if (!active) return justFinished ? 0.0 : duration;
        return std::max(0.0, duration - elapsed);
    }
};