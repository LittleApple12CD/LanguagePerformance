import java.awt.*;

public class NBody {
    double[] x, y, vx, vy, mass;
    int n;
    double G = 0.0002;
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
        double[] ax = new double[n];
        double[] ay = new double[n];
        for (int i = 0; i < n; i++) {
            double axi = 0, ayi = 0;
            double xi = x[i], yi = y[i];
            for (int j = 0; j < n; j++) {
                if (i == j) continue;
                double dx = x[j] - xi;
                double dy = y[j] - yi;
                double d2 = dx * dx + dy * dy;
                if (d2 < 1e-12) continue;
                double inv = 1.0 / d2;
                double f = G * inv * Math.sqrt(inv) * mass[j];
                axi += f * dx;
                ayi += f * dy;
            }
            ax[i] = axi;
            ay[i] = ayi;
        }
        for (int i = 0; i < n; i++) {
            double a2 = ax[i] * ax[i] + ay[i] * ay[i];
            if (a2 > aMax * aMax) {
                double s = aMax / Math.sqrt(a2);
                ax[i] *= s; ay[i] *= s;
            }
            vx[i] += ax[i] * dt;
            vy[i] += ay[i] * dt;
            x[i] += vx[i] * dt;
            y[i] += vy[i] * dt;
        }
    }

    public void draw(Graphics2D g) {
        g.setColor(new Color(180, 220, 255));
        int r = n <= 1000 ? 2 : 1;
        double cx = Main.WIDTH / 2.0;
        double cy = Main.HEIGHT / 2.0;
        for (int i = 0; i < n; i++) {
            int px = (int) (x[i] * scale * Main.WIDTH / 2.0 + cx);
            int py = (int) (-y[i] * scale * Main.WIDTH / 2.0 + cy);
            if (px < 0 || px >= Main.WIDTH || py < 0 || py >= Main.HEIGHT) continue;
            g.fillOval(px - r, py - r, r * 2, r * 2);
        }
    }
}