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
        px[i] = all[i][0]; py[i] = all[i][1]; pz[i] = all[i][2];
        ax[i] = all[i][3]; ay[i] = all[i][4]; az[i] = all[i][5];
        speed[i] = all[i][6];
        colors[i] = sf::Color((uint8_t)all[i][7], (uint8_t)all[i][8], (uint8_t)all[i][9]);
    }
}

void Cubes3D::update(double dt) {
    for (int i = 0; i < n; i++) angles[i] += speed[i] * dt;
}

void Cubes3D::draw(sf::RenderWindow& window) const {
    sf::VertexArray lines(sf::PrimitiveType::Lines, n * 24);
    double fov = 60.0 * 3.14159265358979323846 / 180.0;
    double f = 1.0 / std::tan(fov / 2.0);
    double aspect = (double)Game::WIDTH / Game::HEIGHT;
    double camZ = -8.0;
    double s = 0.25;
    sf::Vector2f texCoords(0.f, 0.f);

    const double verts[8][3] = {
        {-s,-s,-s},{s,-s,-s},{s,s,-s},{-s,s,-s},
        {-s,-s,s},{s,-s,s},{s,s,s},{-s,s,s}
    };
    const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}
    };

    int vIdx = 0;
    for (int i = 0; i < n; i++) {
        double a = angles[i];
        double axx = ax[i], ayy = ay[i], azz = az[i];
        double c = std::cos(a), sn = std::sin(a);
        double r00 = c + axx*axx*(1-c), r01 = axx*ayy*(1-c) - azz*sn, r02 = axx*azz*(1-c) + ayy*sn;
        double r10 = ayy*axx*(1-c) + azz*sn, r11 = c + ayy*ayy*(1-c), r12 = ayy*azz*(1-c) - axx*sn;
        double r20 = azz*axx*(1-c) - ayy*sn, r21 = azz*ayy*(1-c) + axx*sn, r22 = c + azz*azz*(1-c);

        sf::Vector2f pts[8];
        bool ok = true;
        for (int k = 0; k < 8; k++) {
            double bx = verts[k][0], by = verts[k][1], bz = verts[k][2];
            double wx = r00*bx + r01*by + r02*bz + px[i];
            double wy = r10*bx + r11*by + r12*bz + py[i];
            double wz = r20*bx + r21*by + r22*bz + pz[i];
            double ez = wz - camZ;
            if (ez <= 0.1) { ok = false; break; }
            double ndcX = (wx * f / aspect) / ez;
            double ndcY = (wy * f) / ez;
            pts[k].x = (float)((ndcX * 0.5 + 0.5) * Game::WIDTH);
            pts[k].y = (float)((1.0 - (ndcY * 0.5 + 0.5)) * Game::HEIGHT);
        }
        if (!ok) continue;

        sf::Color col = colors[i];
        for (int e = 0; e < 12; e++) {
            lines[vIdx].position = pts[edges[e][0]];
            lines[vIdx].color = col;
            lines[vIdx].texCoords = texCoords;
            vIdx++;

            lines[vIdx].position = pts[edges[e][1]];
            lines[vIdx].color = col;
            lines[vIdx].texCoords = texCoords;
            vIdx++;
        }
    }
    
    lines.resize(vIdx);
    window.draw(lines);
}