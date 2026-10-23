package com.languageper;

import com.badlogic.gdx.graphics.OrthographicCamera;
import com.badlogic.gdx.graphics.glutils.ShapeRenderer;

import java.util.stream.IntStream;

public class NBody {
    double[] x, y, vx, vy, mass;
    int n;
    public double G = 0.0002;
    double dt = 0.016;
    double aMax = 3.0;
    double scale = 0.4;

    double[][] all;
    double[] ax, ay;

    public NBody(double[][] data, int n) {
        this.all = data;
        this.n = n;
        ax = new double[n];
        ay = new double[n];
        reset();
    }

    public void reset() {
        x = new double[n]; y = new double[n];
        vx = new double[n]; vy = new double[n];
        mass = new double[n];
        for (int i = 0; i < n; i++) {
            x[i] = all[i][0];
            y[i] = all[i][1];
            vx[i] = all[i][2];
            vy[i] = all[i][3];
            mass[i] = all[i][4];
        }
    }

    public void update(double frameDt) {
        double safeDt = Math.min(frameDt, 0.016); 

        IntStream.range(0, n).parallel().forEach(i -> {
            double axi = 0, ayi = 0;
            double xi = x[i], yi = y[i];
            for (int j = 0; j < n; j++) {
                double dx = x[j] - xi;
                double dy = y[j] - yi;
                double d2 = dx * dx + dy * dy + 1e-12;
                double inv = 1.0 / d2;
                double f = G * inv * Math.sqrt(inv) * mass[j];
                axi += f * dx;
                ayi += f * dy;
            }
            ax[i] = axi;
            ay[i] = ayi;
        });

        for (int i = 0; i < n; i++) {
            double a2 = ax[i] * ax[i] + ay[i] * ay[i];
            if (a2 > aMax * aMax) {
                double s = aMax / Math.sqrt(a2);
                ax[i] *= s; ay[i] *= s;
            }
            vx[i] += ax[i] * safeDt;
            vy[i] += ay[i] * safeDt;
            x[i] += vx[i] * safeDt;
            y[i] += vy[i] * safeDt;
        }
    }

    public void draw(ShapeRenderer shapes, OrthographicCamera cam) {
        shapes.begin(ShapeRenderer.ShapeType.Filled);
        shapes.setColor(0.7f, 0.85f, 1f, 1f);
        float cx = Main.WIDTH / 2f;
        float cy = Main.HEIGHT / 2f;
        float r = n <= 1000 ? 2f : 1f;
        float diameter = r * 2f;
        
        for (int i = 0; i < n; i++) {
            float px = (float) (x[i] * scale * Main.WIDTH / 2.0 + cx);
            float py = (float) (-y[i] * scale * Main.WIDTH / 2.0 + cy);
            
            if (px < 0 || px >= Main.WIDTH || py < 0 || py >= Main.HEIGHT) continue;

            shapes.rect(px - r, py - r, diameter, diameter);
        }
        shapes.end();
    }
}