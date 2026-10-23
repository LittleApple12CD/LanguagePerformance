// Cubes3D.cpp
#include "Cubes3D.h"
#include "Game.h"
#include <cmath>
#include "rlgl.h"

Cubes3D::Cubes3D(const std::vector<std::vector<double>>& data, int n) : all(data), n(n) {
    reset();
    camera = Camera3D{
        {0.0f, 0.0f, 8.0f}, // 相机正对原点
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        60.0f,
        CAMERA_PERSPECTIVE
    };
}

void Cubes3D::reset() {
    px.resize(n); py.resize(n); pz.resize(n);
    ax.resize(n); ay.resize(n); az.resize(n);
    speed.resize(n); colors.resize(n); angles.assign(n, 0.0);
    for (int i = 0; i < n; i++) {
        px[i] = all[i][0]; py[i] = all[i][1]; pz[i] = all[i][2];
        ax[i] = all[i][3]; ay[i] = all[i][4]; az[i] = all[i][5];
        speed[i] = all[i][6];
        colors[i] = Color{
            (unsigned char)all[i][7],
            (unsigned char)all[i][8],
            (unsigned char)all[i][9],
            255
        };
    }
}

void Cubes3D::update(double dt) {
    for (int i = 0; i < n; i++) angles[i] += speed[i] * dt;
}

void Cubes3D::draw() {
    BeginMode3D(camera);

    rlBegin(RL_QUADS);
    for (int i = 0; i < n; i++) {
        rlPushMatrix();
        rlTranslatef((float)px[i], (float)py[i], (float)pz[i]);
        rlRotatef((float)(angles[i] * RAD2DEG), (float)ax[i], (float)ay[i], (float)az[i]);

        rlColor4ub(colors[i].r, colors[i].g, colors[i].b, 255);
        float s = 0.25f;

        rlVertex3f(-s, -s,  s); rlVertex3f( s, -s,  s);
        rlVertex3f( s,  s,  s); rlVertex3f(-s,  s,  s);
        rlVertex3f(-s, -s, -s); rlVertex3f(-s,  s, -s);
        rlVertex3f( s,  s, -s); rlVertex3f( s, -s, -s);
        rlVertex3f(-s,  s, -s); rlVertex3f(-s,  s,  s);
        rlVertex3f( s,  s,  s); rlVertex3f( s,  s, -s);
        rlVertex3f(-s, -s, -s); rlVertex3f( s, -s, -s);
        rlVertex3f( s, -s,  s); rlVertex3f(-s, -s,  s);
        rlVertex3f( s, -s, -s); rlVertex3f( s,  s, -s);
        rlVertex3f( s,  s,  s); rlVertex3f( s, -s,  s);
        rlVertex3f(-s, -s, -s); rlVertex3f(-s, -s,  s);
        rlVertex3f(-s,  s,  s); rlVertex3f(-s,  s, -s);

        rlPopMatrix();
    }
    rlEnd();

    for (int i = 0; i < n; i++) {
        rlPushMatrix();
        rlTranslatef((float)px[i], (float)py[i], (float)pz[i]);
        rlRotatef((float)(angles[i] * RAD2DEG), (float)ax[i], (float)ay[i], (float)az[i]);

        DrawCubeWires({0, 0, 0}, 0.5f, 0.5f, 0.5f, BLACK);

        rlPopMatrix();
    }

    EndMode3D();
}