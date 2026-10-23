import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.awt.image.BufferStrategy;

public class Main extends Canvas implements KeyListener {

    static final int WIDTH = 1280;
    static final int HEIGHT = 720;
    static final double DURATION = 30.0;
    static final String STACK_NAME = "java-swing";
    static final String DATA_DIR = "data";
    static final String RESULTS_DIR = "results";

    // 场景
    NBody t1;
    Squares2D t2;
    Cubes3D t3;
    volatile int sceneIdx = 0;
    volatile boolean paused = false;

    // 计时
    long lastNanos;
    double[] frameTimes = new double[240];
    int frameCount = 0;
    int frameHead = 0;
    double maxFps = 0.0;

    Measurer measurer = new Measurer(DURATION);
    long startupNanos;
    boolean firstFrame = true;
    double startupMs = 0.0;

    Hud hud;
    volatile boolean running = true;

    public Main() throws IOException {
        setPreferredSize(new Dimension(WIDTH, HEIGHT));
        setBackground(new Color(20, 20, 28));
        setFocusable(true);
        addKeyListener(this);

        // 加载数据
        double[][] particles = Csv.load(Paths.get(DATA_DIR, "particles.csv").toString());
        double[][] squares = Csv.load(Paths.get(DATA_DIR, "squares.csv").toString());
        double[][] cubes = Csv.load(Paths.get(DATA_DIR, "cubes.csv").toString());

        t1 = new NBody(particles, 1000);
        t2 = new Squares2D(squares, 1000);
        t3 = new Cubes3D(cubes, 200);

        hud = new Hud();

        lastNanos = System.nanoTime();
        startupNanos = lastNanos;

        // 启动独立的渲染线程（打破 AWT 的帧率限制）
        Thread gameThread = new Thread(this::gameLoop);
        gameThread.setPriority(Thread.MAX_PRIORITY);
        gameThread.start();
    }

    private void gameLoop() {
        while (running) {
            long now = System.nanoTime();
            double dtMs = (now - lastNanos) / 1e6;
            lastNanos = now;
            if (dtMs <= 0) dtMs = 0.001;

            // 记录帧时间
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
    }

    private void render() {
        if (!isDisplayable()) return;
		
        BufferStrategy bs = getBufferStrategy();
        if (bs == null) {
            try {
                createBufferStrategy(3);
            } catch (IllegalStateException e) {
                return;
            }
            return;
        }
        Graphics2D g2 = (Graphics2D) bs.getDrawGraphics();

        // 清屏
        g2.setColor(new Color(20, 20, 28));
        g2.fillRect(0, 0, WIDTH, HEIGHT);
        g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_OFF);

        // 绘制场景
        switch (sceneIdx) {
            case 0: t1.draw(g2); break;
            case 1: t2.draw(g2); break;
            case 2: t3.draw(g2); break;
        }

        // 绘制 HUD
        hud.draw(g2, STACK_NAME, currentTestName(), currentObjects(),
                paused, measurer.active, measurer.left(),
                sceneIdx == 0 ? t1.G : Double.NaN, stats());

        g2.dispose();
        bs.show();
		
        try {
            Thread.sleep(1); 
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    String currentTestName() {
        switch (sceneIdx) {
            case 0: return "T1 - N-Body";
            case 1: return "T2 - 2D Squares";
            default: return "T3 - 3D Cubes";
        }
    }

    int currentObjects() {
        switch (sceneIdx) {
            case 0: return t1.n;
            case 1: return t2.n;
            default: return t3.n;
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

    void writeResult() {
        double[] s = stats();
        StringBuilder sb = new StringBuilder();
        sb.append("{\n");
        sb.append("  \"stack\": \"").append(STACK_NAME).append("\",\n");
        sb.append("  \"test\": \"").append(currentTestName().split(" ")[0]).append("\",\n");
        sb.append("  \"objects\": ").append(currentObjects()).append(",\n");
        sb.append("  \"resolution\": [").append(WIDTH).append(", ").append(HEIGHT).append("],\n");
        sb.append("  \"avg_fps\": ").append(s[0]).append(",\n");
        sb.append("  \"min_fps\": ").append(1000.0 / maxFrameMs()).append(",\n");
        sb.append("  \"max_fps\": ").append(s[3]).append(",\n");
        sb.append("  \"avg_frame_ms\": ").append(1000.0 / s[0]).append(",\n");
        sb.append("  \"p99_frame_ms\": ").append(s[1]).append(",\n");
        sb.append("  \"one_percent_low_fps\": ").append(s[2]).append(",\n");
        sb.append("  \"startup_ms\": ").append(startupMs).append(",\n");
        sb.append("  \"duration_s\": ").append(DURATION).append(",\n");
        sb.append("  \"runtime\": \"java ").append(System.getProperty("java.version")).append("\",\n");
        sb.append("  \"os\": \"").append(System.getProperty("os.name")).append(" ")
                .append(System.getProperty("os.version")).append("\"\n");
        sb.append("}\n");

        try {
            Files.createDirectories(Paths.get(RESULTS_DIR));
            String fn = RESULTS_DIR + "/" + STACK_NAME + "-" +
                    currentTestName().split(" ")[0] + "-" + currentObjects() + ".json";
            Files.write(Paths.get(fn), sb.toString().getBytes("UTF-8"));
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

    @Override
    public void keyPressed(KeyEvent e) {
        int k = e.getKeyCode();
        if (k == KeyEvent.VK_ESCAPE) {
            if (measurer.frames.size() > 0) writeResult();
            running = false;
            System.exit(0);
        } else if (k == KeyEvent.VK_1) {
            sceneIdx = 0; resetMax();
        } else if (k == KeyEvent.VK_2) {
            sceneIdx = 1; resetMax();
        } else if (k == KeyEvent.VK_3) {
            sceneIdx = 2; resetMax();
        } else if (k == KeyEvent.VK_SPACE) {
            paused = !paused;
        } else if (k == KeyEvent.VK_R) {
            switch (sceneIdx) {
                case 0: t1.reset(); break;
                case 1: t2.reset(); break;
                case 2: t3.reset(); break;
            }
            resetMax();
        } else if (k == KeyEvent.VK_I) {
            hud.show = !hud.show;
        } else if (k == KeyEvent.VK_S) {
            if (!measurer.active) {
                measurer.start();
                resetMax();
                System.out.println("Measuring started...");
            }
        } else if (k == KeyEvent.VK_UP) {
            if (sceneIdx == 0) { t1.G *= 1.5; System.out.println("G = " + t1.G); }
        } else if (k == KeyEvent.VK_DOWN) {
            if (sceneIdx == 0) { t1.G /= 1.5; System.out.println("G = " + t1.G); }
        }
    }

    void resetMax() {
        maxFps = 0.0;
        frameHead = 0;
        frameCount = 0;
    }

    @Override public void keyReleased(KeyEvent e) {}
    @Override public void keyTyped(KeyEvent e) {}

    public static void main(String[] args) throws Exception {
        System.setProperty("sun.java2d.opengl", "true");
        System.setProperty("sun.java2d.d3d", "true");
        System.setProperty("sun.java2d.noddraw", "false");

        SwingUtilities.invokeLater(() -> {
            try {
                JFrame f = new JFrame("Perf Compare - " + STACK_NAME);
                Main m = new Main();

                try {
                    BufferedImage icon = ImageIO.read(
                        Main.class.getResourceAsStream("/icon.png")
                    );
                    if (icon != null) {
                        f.setIconImage(icon);
                    }
                } catch (Exception ex) {
				// None	
                }

                f.setResizable(false);
                f.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

                f.add(m, BorderLayout.CENTER);
                f.pack();
                f.setLocationRelativeTo(null);
                f.setVisible(true);

                m.requestFocus();
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        });
    }
}