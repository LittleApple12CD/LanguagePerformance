import java.awt.*;

public class Hud {
    boolean show = true;

    public void draw(Graphics2D g, String stack, String test, int objects,
                     boolean paused, boolean measuring, double left,
                     double gval, double[] stats) {
        if (!show) return;
        g.setFont(new Font("Consolas", Font.PLAIN, 13));

        String[] lines = {
                String.format("Stack:    %s", stack),
                String.format("Test:     %s", test),
                String.format("FPS:      %6.1f", stats[0]),
                String.format("Frame:    %5.2f ms (avg) / %5.2f ms (p99)", 1000.0/stats[0], stats[1]),
                String.format("Max FPS:  %6.1f", stats[3]),
                String.format("1%% Low:   %6.1f", stats[2]),
                String.format("Objects:  %d", objects),
                Double.isNaN(gval) ? "G:        -" : String.format("G:        %.5f", gval),
                String.format("Paused:   %b", paused),
                measuring ? String.format("Measuring: %ds", (int) left) : "Measuring: no",
                "[1] T1  [2] T2  [3] T3  [Space] Pause  [R] Reset",
                "[I] Info  [S] Measure  [Esc] Quit",
                "[Up/Down] G",
        };

        int y = 6;
        for (String line : lines) {
            int w = g.getFontMetrics().stringWidth(line);
            g.setColor(new Color(0, 0, 0, 150));
            g.fillRect(4, y, w + 8, 16);
            g.setColor(new Color(230, 230, 230));
            g.drawString(line, 8, y + 13);
            y += 18;
        }
    }
}