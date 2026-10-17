import javafx.scene.canvas.GraphicsContext;
import javafx.scene.paint.Color;

public class Cubes3D {
    double[] px, py, pz;
    double[] ax_, ay_, az_;
    double[] speed;
    Color[] colors;
    double[] angles;
    int n;

    double[][] all;

    public Cubes3D(double[][] data, int n) {
        this.all = data;
        this.n = n;
        reset();
    }

    public void reset() {
        px = new double[n]; py = new double[n]; pz = new double[n];
        ax_ = new double[n]; ay_ = new double[n]; az_ = new double[n];
        speed = new double[n];
        colors = new Color[n];
        angles = new double[n];
        for (int i = 0; i < n; i++) {
            px[i] = all[i][0]; py[i] = all[i][1]; pz[i] = all[i][2];
            ax_[i] = all[i][3]; ay_[i] = all[i][4]; az_[i] = all[i][5];
            speed[i] = all[i][6];
            colors[i] = Color.rgb(
                    (int) all[i][7], (int) all[i][8], (int) all[i][9]);
        }
    }

    public void update(double dt) {
        for (int i = 0; i < n; i++) angles[i] += speed[i] * dt;
    }

    public void draw(GraphicsContext gc) {
        gc.setLineWidth(2);
        double fov = Math.toRadians(60);
        double f = 1.0 / Math.tan(fov / 2);
        double aspect = (double) Main.WIDTH / Main.HEIGHT;
        double camZ = -8.0;
        double s = 0.25;

        double[][] verts = {
                {-s, -s, -s}, {s, -s, -s}, {s, s, -s}, {-s, s, -s},
                {-s, -s, s}, {s, -s, s}, {s, s, s}, {-s, s, s},
        };
        int[][] edges = {
                {0,1},{1,2},{2,3},{3,0},
                {4,5},{5,6},{6,7},{7,4},
                {0,4},{1,5},{2,6},{3,7},
        };

        for (int i = 0; i < n; i++) {
            double a = angles[i];
            double axx = ax_[i], ayy = ay_[i], azz = az_[i];
            double c = Math.cos(a), sn = Math.sin(a);
            double r00 = c + axx*axx*(1-c), r01 = axx*ayy*(1-c) - azz*sn, r02 = axx*azz*(1-c) + ayy*sn;
            double r10 = ayy*axx*(1-c) + azz*sn, r11 = c + ayy*ayy*(1-c), r12 = ayy*azz*(1-c) - axx*sn;
            double r20 = azz*axx*(1-c) - ayy*sn, r21 = azz*ayy*(1-c) + axx*sn, r22 = c + azz*azz*(1-c);

            double[] sx = new double[8];
            double[] sy = new double[8];
            boolean ok = true;
            for (int k = 0; k < 8; k++) {
                double bx = verts[k][0], by = verts[k][1], bz = verts[k][2];
                double wx = r00*bx + r01*by + r02*bz + px[i];
                double wy = r10*bx + r11*by + r12*bz + py[i];
                double wz = r20*bx + r21*by + r22*bz + pz[i];
                double ez = wz - camZ;
                if (ez <= 0.1) { ok = false; break; }
                double ndcX = (wx * f / aspect) / ez;
                double ndcY = (wy * f) / ez;
                sx[k] = (ndcX * 0.5 + 0.5) * Main.WIDTH;
                sy[k] = (1.0 - (ndcY * 0.5 + 0.5)) * Main.HEIGHT;
            }
            if (!ok) continue;
            gc.setStroke(colors[i]);
            for (int[] e : edges) {
                gc.strokeLine(sx[e[0]], sy[e[0]], sx[e[1]], sy[e[1]]);
            }
        }
    }
}