import javafx.application.Application;
import javafx.application.Platform;
import javafx.scene.Scene;
import javafx.scene.canvas.Canvas;
import javafx.scene.canvas.GraphicsContext;
import javafx.scene.layout.Pane;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;
import javafx.scene.text.FontSmoothingType;
import javafx.scene.image.Image;
import javafx.stage.Stage;
import javafx.animation.AnimationTimer;

import java.io.IOException;
import java.nio.file.*;
import java.util.*;

public class Main extends Application {

    static final int WIDTH = 1280;
    static final int HEIGHT = 720;
    static final double DURATION = 30.0;
    static final String STACK_NAME = "java-javafx";
    static final String DATA_DIR = "data";
    static final String RESULTS_DIR = "results";

    NBody t1;
    Squares2D t2;
    Cubes3D t3;
    int sceneIdx = 0;
    boolean paused = false;
    boolean hudShow = true;

    double[] frameTimes = new double[240];
    int frameCount = 0;
    int frameHead = 0;
    double maxFps = 0.0;
    long lastNanos = 0;
    long startupNanos = 0;
    double startupMs = 0;
    boolean firstFrame = true;

    Measurer measurer = new Measurer(DURATION);
    Canvas canvas;
    GraphicsContext gc;

    // 缓存字体，避免每帧重建
    final Font hudFont = Font.font("Consolas", 13);

    @Override
    public void start(Stage stage) throws IOException {
        double[][] particles = Csv.load("data/particles.csv");
        double[][] squares = Csv.load("data/squares.csv");
        double[][] cubes = Csv.load("data/cubes.csv");

        t1 = new NBody(particles, 1000);
        t2 = new Squares2D(squares, 1000);
        t3 = new Cubes3D(cubes, 200);

        canvas = new Canvas(WIDTH, HEIGHT);
        gc = canvas.getGraphicsContext2D();

        Pane root = new Pane(canvas);
        Scene scene = new Scene(root, WIDTH, HEIGHT, Color.rgb(20, 20, 28));

        scene.setOnKeyPressed(e -> {
            switch (e.getCode()) {
                case DIGIT1: sceneIdx = 0; resetMax(); break;
                case DIGIT2: sceneIdx = 1; resetMax(); break;
                case DIGIT3: sceneIdx = 2; resetMax(); break;
                case SPACE: paused = !paused; break;
                case R: resetScene(); resetMax(); break;
                case I: hudShow = !hudShow; break;
                case S:
                    if (!measurer.active) {
                        measurer.start();
                        resetMax();
                        System.out.println("Measuring started...");
                    }
                    break;
                case ESCAPE:
                    if (!measurer.frames.isEmpty()) writeResult();
                    Platform.exit();
                    break;
                case UP:
                    if (sceneIdx == 0) {
                        t1.G *= 1.5;
                        System.out.println("G = " + t1.G);
                    }
                    break;
                case DOWN:
                    if (sceneIdx == 0) {
                        t1.G /= 1.5;
                        System.out.println("G = " + t1.G);
                    }
                    break;
            }
        });

        stage.getIcons().add(
            new Image(getClass().getResourceAsStream("icon.png"))
        );

        stage.setTitle("Perf Compare - " + STACK_NAME);
        stage.setScene(scene);
        stage.setResizable(false);
        stage.show();
        canvas.requestFocus();

        startupNanos = System.nanoTime();
        lastNanos = startupNanos;

        new AnimationTimer() {
            @Override
            public void handle(long now) {
                double dtMs = (now - lastNanos) / 1e6;
                lastNanos = now;
                if (dtMs <= 0) dtMs = 0.001;

                frameTimes[frameHead] = dtMs;
                frameHead = (frameHead + 1) % frameTimes.length;
                if (frameCount < frameTimes.length) frameCount++;
                double instFps = 1000.0 / dtMs;
                if (instFps > maxFps) maxFps = instFps;

                if (measurer.active) {
                    measurer.tick(dtMs / 1000.0);
                    if (measurer.justFinished) {
                        measurer.justFinished = false;
                        writeResult();
                    }
                }

                if (!paused) {
                    double dt = Math.min(dtMs / 1000.0, 0.05);
                    switch (sceneIdx) {
                        case 0: t1.update(dt); break;
                        case 1: t2.update(dt); break;
                        case 2: t3.update(dt); break;
                    }
                }

                render();

                if (firstFrame) {
                    startupMs = (System.nanoTime() - startupNanos) / 1e6;
                    firstFrame = false;
                }
            }
        }.start();
    }

    void render() {
        gc.setFill(Color.rgb(20, 20, 28));
        gc.fillRect(0, 0, WIDTH, HEIGHT);

        switch (sceneIdx) {
            case 0: t1.draw(gc); break;
            case 1: t2.draw(gc); break;
            case 2: t3.draw(gc); break;
        }

        drawHud();
    }

    void drawHud() {
        if (!hudShow) return;

        double[] s = stats();
        String test = sceneIdx == 0 ? "T1 - N-Body"
                    : sceneIdx == 1 ? "T2 - 2D Squares"
                    : "T3 - 3D Cubes";
        int objects = sceneIdx == 0 ? t1.n
                    : sceneIdx == 1 ? t2.n
                    : t3.n;

        String[] lines = {
                String.format("Stack:    %s", STACK_NAME),
                String.format("Test:     %s", test),
                String.format("FPS:      %6.1f", s[0]),
                String.format("Frame:    %5.2f ms (avg) / %5.2f ms (p99)",
                        1000.0 / s[0], s[1]),
                String.format("Max FPS:  %6.1f", s[3]),
                String.format("1%% Low:   %6.1f", s[2]),
                String.format("Objects:  %d", objects),
                sceneIdx == 0 ? String.format("G:        %.5f", t1.G) : "G:        -",
                String.format("Paused:   %b", paused),
                measurer.active
                        ? String.format("Measuring: %ds", (int) measurer.left())
                        : "Measuring: no",
                "[1] T1  [2] T2  [3] T3  [Space] Pause  [R] Reset",
                "[I] Info  [S] Measure  [Esc] Quit",
                "[Up/Down] G",
        };

        gc.setFont(hudFont);

        int y = 6;
        for (String line : lines) {
            gc.setFill(Color.rgb(230, 230, 230));
            gc.fillText(line, 8, y + 13);
            y += 18;
        }
    }

    double[] stats() {
        if (frameCount == 0) return new double[]{0, 0, 0, maxFps};
        double[] copy = new double[frameCount];
        System.arraycopy(frameTimes, 0, copy, 0, frameCount);
        Arrays.sort(copy);
        double sum = 0;
        for (double v : copy) sum += v;
        double avg = sum / frameCount;
        double p99 = copy[Math.min(frameCount - 1, (int) (frameCount * 0.99))];
        int k = Math.max(1, (int) (frameCount * 0.01));
        double slowSum = 0;
        for (int i = frameCount - k; i < frameCount; i++) slowSum += copy[i];
        double low1 = 1000.0 / (slowSum / k);
        return new double[]{1000.0 / avg, p99, low1, maxFps};
    }

    void resetScene() {
        switch (sceneIdx) {
            case 0: t1.reset(); break;
            case 1: t2.reset(); break;
            case 2: t3.reset(); break;
        }
    }

    void resetMax() {
        maxFps = 0;
        frameHead = 0;
        frameCount = 0;
    }

    void writeResult() {
        double[] s = stats();
        String test = sceneIdx == 0 ? "T1" : sceneIdx == 1 ? "T2" : "T3";
        int objects = sceneIdx == 0 ? t1.n : sceneIdx == 1 ? t2.n : t3.n;
        String json = String.format(
                "{\n  \"stack\": \"%s\",\n  \"test\": \"%s\",\n  \"objects\": %d,\n" +
                "  \"resolution\": [%d, %d],\n  \"avg_fps\": %f,\n  \"min_fps\": %f,\n" +
                "  \"max_fps\": %f,\n  \"avg_frame_ms\": %f,\n  \"p99_frame_ms\": %f,\n" +
                "  \"one_percent_low_fps\": %f,\n  \"startup_ms\": %f,\n  \"duration_s\": %f,\n" +
                "  \"runtime\": \"java %s + javafx 21\",\n  \"os\": \"%s %s\"\n}\n",
                STACK_NAME, test, objects, WIDTH, HEIGHT,
                s[0], 1000.0 / maxFrameMs(), s[3], 1000.0 / s[0], s[1], s[2],
                startupMs, DURATION,
                System.getProperty("java.version"),
                System.getProperty("os.name"), System.getProperty("os.version"));
        try {
            Files.createDirectories(Paths.get(RESULTS_DIR));
            String fn = RESULTS_DIR + "/" + STACK_NAME + "-" + test + "-" + objects + ".json";
            Files.write(Paths.get(fn), json.getBytes("UTF-8"));
            System.out.println("Wrote " + fn);
        } catch (IOException ex) {
            ex.printStackTrace();
        }
    }

    double maxFrameMs() {
        double m = 0;
        for (int i = 0; i < frameCount; i++) if (frameTimes[i] > m) m = frameTimes[i];
        return m > 0 ? m : 1;
    }
    
    public static void main(String[] args) {
        launch(args);
    }
}