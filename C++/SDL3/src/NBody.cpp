// NBody.cpp
#include "NBody.h"
#include "Game.h"
#include <cmath>
#include <algorithm>

NBody::NBody(const std::vector<std::vector<double>>& data, int n) : all(data), n(n) { reset(); }

void NBody::reset() {
    x.resize(n); y.resize(n); vx.resize(n); vy.resize(n); mass.resize(n);
    ax.resize(n); ay.resize(n);
    for (int i = 0; i < n; i++) {
        x[i]=all[i][0]; y[i]=all[i][1]; vx[i]=all[i][2]; vy[i]=all[i][3]; mass[i]=all[i][4];
    }
}

void NBody::update(double frameDt) {
	#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++) {
        double axi=0, ayi=0, xi=x[i], yi=y[i];
        for (int j = 0; j < n; j++) {
            double dx=x[j]-xi, dy=y[j]-yi;
            double d2=dx*dx+dy*dy+1e-12;
            double inv=1.0/d2;
            double f=G*inv*std::sqrt(inv)*mass[j];
            axi+=f*dx; ayi+=f*dy;
        }
        ax[i]=axi; ay[i]=ayi;
    }
    for (int i = 0; i < n; i++) {
        double a2=ax[i]*ax[i]+ay[i]*ay[i];
        if (a2>aMax*aMax) { double s=aMax/std::sqrt(a2); ax[i]*=s; ay[i]*=s; }
        vx[i]+=ax[i]*frameDt; vy[i]+=ay[i]*frameDt; 
        x[i]+=vx[i]*frameDt; y[i]+=vy[i]*frameDt;
    }
}

void NBody::draw(SDL_Renderer* renderer) const {
    SDL_SetRenderDrawColor(renderer, 180, 220, 255, 255);
    double cx = Game::WIDTH/2.0, cy = Game::HEIGHT/2.0;
    for (int i = 0; i < n; i++) {
        float px = (float)(x[i]*scale*Game::WIDTH/2.0 + cx);
        float py = (float)(-y[i]*scale*Game::WIDTH/2.0 + cy);
        if (px<0||px>=Game::WIDTH||py<0||py>=Game::HEIGHT) continue;
        SDL_FRect r{px-1.5f, py-1.5f, 3.f, 3.f};
        SDL_RenderFillRect(renderer, &r);
    }
}