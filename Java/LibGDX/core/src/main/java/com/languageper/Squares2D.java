package com.languageper;

import com.badlogic.gdx.graphics.Color;
import com.badlogic.gdx.graphics.OrthographicCamera;
import com.badlogic.gdx.graphics.glutils.ShapeRenderer;

public class Squares2D {
    double[] x, y, vx, vy, size;
    Color[] colors;
    int n;
    int x0, x1, y0, y1;
    double[][] all;

    public Squares2D(double[][] data, int n) {
        this.all = data;
        this.n = n;
        x0 = Main.WIDTH / 2 - 200;
        x1 = Main.WIDTH / 2 + 200;
        y0 = Main.HEIGHT / 2 - 200;
        y1 = Main.HEIGHT / 2 + 200;
        reset();
    }

    public void reset() {
        x = new double[n]; y = new double[n];
        vx = new double[n]; vy = new double[n];
        size = new double[n];
        colors = new Color[n];
        for (int i = 0; i < n; i++) {
            x[i] = all[i][0] + x0;
            y[i] = all[i][1] + y0;
            vx[i] = all[i][2];
            vy[i] = all[i][3];
            size[i] = all[i][4];
            colors[i] = new Color(
                    (float) (all[i][5] / 255.0),
                    (float) (all[i][6] / 255.0),
                    (float) (all[i][7] / 255.0), 1f);
        }
    }

    public void update(double dt) {
        for (int i = 0; i < n; i++) {
            x[i] += vx[i];
            y[i] += vy[i];
            if (x[i] < x0 || x[i] > x1) {
                vx[i] = -vx[i];
                x[i] = Math.max(x0, Math.min(x1, x[i]));
            }
            if (y[i] < y0 || y[i] > y1) {
                vy[i] = -vy[i];
                y[i] = Math.max(y0, Math.min(y1, y[i]));
            }
        }
    }

    public void draw(ShapeRenderer shapes, OrthographicCamera cam) {
        shapes.begin(ShapeRenderer.ShapeType.Filled);
        for (int i = 0; i < n; i++) {
            shapes.setColor(colors[i]);
            shapes.rect((float) x[i], (float) y[i], (float) size[i], (float) size[i]);
        }
        shapes.end();
    }
}