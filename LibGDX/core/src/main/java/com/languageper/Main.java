package com.languageper;

import com.badlogic.gdx.ApplicationAdapter;
import com.badlogic.gdx.Gdx;
import com.badlogic.gdx.Input;
import com.badlogic.gdx.graphics.GL20;
import com.badlogic.gdx.graphics.OrthographicCamera;
import com.badlogic.gdx.graphics.g2d.BitmapFont;
import com.badlogic.gdx.graphics.g2d.SpriteBatch;
import com.badlogic.gdx.graphics.glutils.ShapeRenderer;
import com.badlogic.gdx.utils.ScreenUtils;

import java.io.IOException;

public class Main extends ApplicationAdapter {

    public static final int WIDTH = 1280;
    public static final int HEIGHT = 720;
    public static final double DURATION = 30.0;
    public static final String STACK_NAME = "java-libgdx";

    NBody t1;
    Squares2D t2;
    Cubes3D t3;
    int sceneIdx = 0;
    boolean paused = false;
    boolean hudShow = true;

    SpriteBatch batch;
    BitmapFont font;
    ShapeRenderer shapes;
    OrthographicCamera cam;

    // 帧时间
    float[] frameTimes = new float[240];
    int frameCount = 0;
    int frameHead = 0;
    float maxFps = 0f;
    float lastTime = 0f;
    float startupMs = 0f;
    boolean firstFrame = true;

    Measurer measurer = new Measurer(DURATION);
    
    private final float[] statsBuffer = new float[240];
    private final StringBuilder hudBuilder = new StringBuilder(1024);

    @Override
    public void create() {
        try {
            t1 = new NBody(Csv.load("data/particles.csv"), 1000);
            t2 = new Squares2D(Csv.load("data/squares.csv"), 1000);
            t3 = new Cubes3D(Csv.load("data/cubes.csv"), 200);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

        batch = new SpriteBatch();
        font = new BitmapFont();
        font.getData().setScale(1.0f);
        shapes = new ShapeRenderer();
        cam = new OrthographicCamera();
        cam.setToOrtho(false, WIDTH, HEIGHT);

        lastTime = 0f;
    }

    @Override
    public void render() {
        float now = (float) (System.nanoTime() / 1e9);
        float dt = now - lastTime;
        lastTime = now;
        if (dt <= 0f) dt = 0.001f;

        float dtMs = dt * 1000f;
        frameTimes[frameHead] = dtMs;
        frameHead = (frameHead + 1) % frameTimes.length;
        if (frameCount < frameTimes.length) frameCount++;
        float instFps = 1000f / dtMs;
        if (instFps > maxFps) maxFps = instFps;

        if (measurer.active) {
            measurer.tick(dt);
            if (measurer.justFinished) {
                measurer.justFinished = false;
                writeResult();
            }
        }

        handleInput();

        if (!paused) {
            switch (sceneIdx) {
                case 0: t1.update(dt); break;
                case 1: t2.update(dt); break;
                case 2: t3.update(dt); break;
            }
        }

        ScreenUtils.clear(0.08f, 0.08f, 0.11f, 1f);

        cam.update();
        shapes.setProjectionMatrix(cam.combined);

        switch (sceneIdx) {
            case 0: t1.draw(shapes, cam); break;
            case 1: t2.draw(shapes, cam); break;
            case 2: t3.draw(); break;
        }

        drawHud();

        if (firstFrame) {
            startupMs = (float) (now * 1000.0);
            firstFrame = false;
        }
    }

    void handleInput() {
        if (Gdx.input.isKeyJustPressed(Input.Keys.NUM_1)) { sceneIdx = 0; resetMax(); }
        if (Gdx.input.isKeyJustPressed(Input.Keys.NUM_2)) { sceneIdx = 1; resetMax(); }
        if (Gdx.input.isKeyJustPressed(Input.Keys.NUM_3)) { sceneIdx = 2; resetMax(); }
        if (Gdx.input.isKeyJustPressed(Input.Keys.SPACE)) paused = !paused;
        if (Gdx.input.isKeyJustPressed(Input.Keys.R)) {
            switch (sceneIdx) {
                case 0: t1.reset(); break;
                case 1: t2.reset(); break;
                case 2: t3.reset(); break;
            }
            resetMax();
        }
        if (Gdx.input.isKeyJustPressed(Input.Keys.I)) hudShow = !hudShow;
        if (Gdx.input.isKeyJustPressed(Input.Keys.S)) {
            if (!measurer.active) {
                measurer.start();
                resetMax();
                System.out.println("Measuring started...");
            }
        }
        if (Gdx.input.isKeyJustPressed(Input.Keys.ESCAPE)) {
            if (!measurer.frames.isEmpty()) writeResult();
            Gdx.app.exit();
        }
        if (Gdx.input.isKeyJustPressed(Input.Keys.UP) && sceneIdx == 0) {
            t1.G *= 1.5;
            System.out.println("G = " + t1.G);
        }
        if (Gdx.input.isKeyJustPressed(Input.Keys.DOWN) && sceneIdx == 0) {
            t1.G /= 1.5;
            System.out.println("G = " + t1.G);
        }
    }

    void drawHud() {
        if (!hudShow) return;

        float[] s = stats();
        String test = sceneIdx == 0 ? "T1 - N-Body" : sceneIdx == 1 ? "T2 - 2D Squares" : "T3 - 3D Cubes";
        int objects = sceneIdx == 0 ? t1.n : sceneIdx == 1 ? t2.n : t3.n;

        hudBuilder.setLength(0);
        hudBuilder.append("Stack:    ").append(STACK_NAME).append('\n');
        hudBuilder.append("Test:     ").append(test).append('\n');
        hudBuilder.append("FPS:      ").append(String.format("%6.1f", s[0])).append('\n');
        hudBuilder.append("Frame:    ").append(String.format("%5.2f", 1000f / s[0]))
                  .append(" ms (avg) / ").append(String.format("%5.2f", s[1])).append(" ms (p99)\n");
        hudBuilder.append("Max FPS:  ").append(String.format("%6.1f", s[3])).append('\n');
        hudBuilder.append("1% Low:   ").append(String.format("%6.1f", s[2])).append('\n');
        hudBuilder.append("Objects:  ").append(objects).append('\n');
        if (sceneIdx == 0) hudBuilder.append("G:        ").append(String.format("%.5f", t1.G)).append('\n');
        else hudBuilder.append("G:        -\n");
        hudBuilder.append("Paused:   ").append(paused).append('\n');
        hudBuilder.append(measurer.active ? "Measuring: " + (int) measurer.left() + "s\n" : "Measuring: no\n");
        hudBuilder.append("[1] T1  [2] T2  [3] T3  [Space] Pause  [R] Reset\n");
        hudBuilder.append("[I] Info  [S] Measure  [Esc] Quit\n");
        hudBuilder.append("[Up/Down] G\n");

        Gdx.gl.glDisable(GL20.GL_DEPTH_TEST);
        Gdx.gl.glEnable(GL20.GL_BLEND);
        Gdx.gl.glBlendFunc(GL20.GL_SRC_ALPHA, GL20.GL_ONE_MINUS_SRC_ALPHA);

        batch.begin();
        font.setColor(1f, 1f, 1f, 1f);

        float startY = HEIGHT - 10;
        int lineIndex = 0;
        int start = 0;
        for (int i = 0; i < hudBuilder.length(); i++) {
            if (hudBuilder.charAt(i) == '\n') {
                font.draw(batch, hudBuilder.substring(start, i), 10, startY - lineIndex * 18);
                start = i + 1;
                lineIndex++;
            }
        }
        if (start < hudBuilder.length()) {
            font.draw(batch, hudBuilder.substring(start), 10, startY - lineIndex * 18);
        }

        batch.end();
        Gdx.gl.glDisable(GL20.GL_BLEND);
    }

    float[] stats() {
        if (frameCount == 0) return new float[]{0, 0, 0, maxFps};
        
        System.arraycopy(frameTimes, 0, statsBuffer, 0, frameCount);
        java.util.Arrays.sort(statsBuffer, 0, frameCount);
        
        float sum = 0;
        for (int i = 0; i < frameCount; i++) sum += statsBuffer[i];
        float avg = sum / frameCount;
        
        float p99 = statsBuffer[Math.min(frameCount - 1, (int) (frameCount * 0.99))];
        int k = Math.max(1, (int) (frameCount * 0.01));
        float slowSum = 0;
        for (int i = frameCount - k; i < frameCount; i++) slowSum += statsBuffer[i];
        
        float low1 = 1000f / (slowSum / k);
        return new float[]{1000f / avg, p99, low1, maxFps};
    }

    void resetMax() {
        maxFps = 0f;
        frameHead = 0;
        frameCount = 0;
    }

    void writeResult() {
        float[] s = stats();
        String test = sceneIdx == 0 ? "T1" : sceneIdx == 1 ? "T2" : "T3";
        int objects = sceneIdx == 0 ? t1.n : sceneIdx == 1 ? t2.n : t3.n;
        float maxMs = 0;
        for (int i = 0; i < frameCount; i++) if (frameTimes[i] > maxMs) maxMs = frameTimes[i];
        String json = String.format(
                "{\n  \"stack\": \"%s\",\n  \"test\": \"%s\",\n  \"objects\": %d,\n" +
                "  \"resolution\": [%d, %d],\n  \"avg_fps\": %f,\n  \"min_fps\": %f,\n" +
                "  \"max_fps\": %f,\n  \"avg_frame_ms\": %f,\n  \"p99_frame_ms\": %f,\n" +
                "  \"one_percent_low_fps\": %f,\n  \"startup_ms\": %f,\n  \"duration_s\": %f,\n" +
                "  \"runtime\": \"java %s + libgdx 1.14.2\",\n  \"os\": \"%s %s\"\n}\n",
                STACK_NAME, test, objects, WIDTH, HEIGHT,
                s[0], 1000f / maxMs, s[3], 1000f / s[0], s[1], s[2],
                startupMs, DURATION,
                System.getProperty("java.version"),
                System.getProperty("os.name"), System.getProperty("os.version"));
        try {
            java.nio.file.Files.createDirectories(java.nio.file.Paths.get("results"));
            String fn = "results/" + STACK_NAME + "-" + test + "-" + objects + ".json";
            java.nio.file.Files.write(java.nio.file.Paths.get(fn), json.getBytes("UTF-8"));
            System.out.println("Wrote " + fn);
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    @Override
    public void dispose() {
        batch.dispose();
        font.dispose();
        shapes.dispose();
        t3.dispose();
    }
}