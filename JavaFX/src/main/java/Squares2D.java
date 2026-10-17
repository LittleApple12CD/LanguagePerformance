import javafx.scene.canvas.GraphicsContext;
import javafx.scene.paint.Color;

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
            colors[i] = Color.rgb(
                    (int) all[i][5], (int) all[i][6], (int) all[i][7]);
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

    public void draw(GraphicsContext gc) {
        for (int i = 0; i < n; i++) {
            gc.setFill(colors[i]);
            gc.fillRect(x[i], y[i], size[i], size[i]);
        }
    }
}