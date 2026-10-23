// Cubes3D.cpp
#include "Cubes3D.h"
#include "Game.h"
#include <cmath>

Cubes3D::Cubes3D(const std::vector<std::vector<double>>& data, int n) : all(data), n(n) { reset(); }

void Cubes3D::reset() {
    px.resize(n); py.resize(n); pz.resize(n);
    ax.resize(n); ay.resize(n); az.resize(n);
    speed.resize(n); colors.resize(n); angles.assign(n, 0.0);
    for (int i = 0; i < n; i++) {
        px[i]=all[i][0]; py[i]=all[i][1]; pz[i]=all[i][2];
        ax[i]=all[i][3]; ay[i]=all[i][4]; az[i]=all[i][5];
        speed[i]=all[i][6];
        colors[i] = SDL_Color{(Uint8)all[i][7], (Uint8)all[i][8], (Uint8)all[i][9], 255};
    }
}

void Cubes3D::update(double dt) {
    for (int i = 0; i < n; i++) angles[i] += speed[i]*dt;
}

void Cubes3D::drawGpu() {
    if (!gpu) return;
    std::vector<CubeInstance> inst(n);
    for (int i = 0; i < n; i++) {
        inst[i].axis[0] = (float)ax[i]; inst[i].axis[1] = (float)ay[i]; inst[i].axis[2] = (float)az[i];
        inst[i].center[0] = (float)px[i]; inst[i].center[1] = (float)py[i]; inst[i].center[2] = (float)pz[i];
        inst[i].angle = (float)angles[i];
        inst[i].color[0] = colors[i].r / 255.f; inst[i].color[1] = colors[i].g / 255.f; inst[i].color[2] = colors[i].b / 255.f;
    }
    
    float fov = 60.0f * 3.14159265f / 180.0f;
    float aspect = (float)Game::WIDTH / Game::HEIGHT;
    float f = 1.0f / std::tan(fov / 2.0f);
    float zn = 0.1f, zf = 100.0f;
    float proj[16] = {
        f/aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, zf/(zf-zn), 1,
        0, 0, -zn*zf/(zf-zn), 0
    };

    float view[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 8, 1
    };
    
    float vp[16] = {0};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                vp[i + j*4] += proj[i + k*4] * view[k + j*4];
            }
        }
    }

    gpu->draw(inst, vp, 0.5f); 
}