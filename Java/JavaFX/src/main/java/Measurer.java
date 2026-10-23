import java.util.*;

public class Measurer {
    double duration;
    boolean active = false;
    double elapsed = 0;
    List<Double> frames = new ArrayList<>();
    boolean justFinished = false;

    public Measurer(double duration) {
        this.duration = duration;
    }

    public void start() {
        active = true;
        elapsed = 0;
        frames.clear();
        justFinished = false;
    }

    public void tick(double dt) {
        if (!active) return;
        frames.add(dt);
        elapsed += dt;
        if (elapsed >= duration) {
            active = false;
            justFinished = true;
        }
    }

    public double left() {
        if (!active) return justFinished ? 0.0 : duration;
        return Math.max(0.0, duration - elapsed);
    }
}