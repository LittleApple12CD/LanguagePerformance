import os
import sys
import json
import time
import platform
import pygame.gfxdraw

import numpy as np
import pygame
from numba import njit

# ---------- 配置 ----------
WIDTH, HEIGHT = 1280, 720
FPS_CAP = 0
DURATION = 30.0
STACK_NAME = "python-pygame-numba"

RESULTS_DIR = "results"
DATA_DIR = "data"


# ============================================================
# Numba 加速函数
# ============================================================

@njit(cache=True, fastmath=True)
def nbody_step(x, y, vx, vy, mass, dt, G, a_max):
    n = x.shape[0]
    ax = np.zeros(n)
    ay = np.zeros(n)
    for i in range(n):
        for j in range(n):
            if i == j:
                continue
            dx = x[j] - x[i]
            dy = y[j] - y[i]
            d2 = dx * dx + dy * dy
            if d2 < 1e-12:
                continue
            inv = 1.0 / d2
            f = G * inv * np.sqrt(inv)
            ax[i] += f * dx * mass[j]
            ay[i] += f * dy * mass[j]
    for i in range(n):
        a2 = ax[i] * ax[i] + ay[i] * ay[i]
        if a2 > a_max * a_max:
            scale = a_max / np.sqrt(a2)
            ax[i] *= scale
            ay[i] *= scale
        vx[i] += ax[i] * dt
        vy[i] += ay[i] * dt
        x[i] += vx[i] * dt
        y[i] += vy[i] * dt
        
@njit(cache=True, fastmath=True)
def update_squares_numba(sx, sy, vx, vy, x0, x1, y0, y1):
    for i in range(sx.shape[0]):
        sx[i] += vx[i]
        sy[i] += vy[i]
        if sx[i] < x0 or sx[i] > x1:
            vx[i] = -vx[i]
            sx[i] = min(max(sx[i], x0), x1)
        if sy[i] < y0 or sy[i] > y1:
            vy[i] = -vy[i]
            sy[i] = min(max(sy[i], y0), y1)

@njit(cache=True, fastmath=True)
def rotate_cubes(angles, speeds, dt):
    for i in range(angles.shape[0]):
        angles[i] += speeds[i] * dt


@njit(cache=True, fastmath=True)
def project_cubes(angles, axes, positions, width, height):
    n = angles.shape[0]
    out = np.zeros((n, 8, 2))
    visible = np.zeros(n, dtype=np.bool_)

    s = 0.25
    verts = np.array([
        [-s, -s, -s], [ s, -s, -s], [ s,  s, -s], [-s,  s, -s],
        [-s, -s,  s], [ s, -s,  s], [ s,  s,  s], [-s,  s,  s],
    ])

    fov = 1.0472
    cam_z = -8.0
    f = 1.0 / np.tan(fov / 2.0)
    aspect = width / height

    for i in range(n):
        a = angles[i]
        ax, ay, az = axes[i, 0], axes[i, 1], axes[i, 2]
        c = np.cos(a)
        sn = np.sin(a)
        r00 = c + ax * ax * (1 - c)
        r01 = ax * ay * (1 - c) - az * sn
        r02 = ax * az * (1 - c) + ay * sn
        r10 = ay * ax * (1 - c) + az * sn
        r11 = c + ay * ay * (1 - c)
        r12 = ay * az * (1 - c) - ax * sn
        r20 = az * ax * (1 - c) - ay * sn
        r21 = az * ay * (1 - c) + ax * sn
        r22 = c + az * az * (1 - c)

        ok = True
        for k in range(8):
            vx0 = verts[k, 0]
            vy0 = verts[k, 1]
            vz0 = verts[k, 2]
            wx = r00 * vx0 + r01 * vy0 + r02 * vz0
            wy = r10 * vx0 + r11 * vy0 + r12 * vz0
            wz = r20 * vx0 + r21 * vy0 + r22 * vz0
            wx += positions[i, 0]
            wy += positions[i, 1]
            wz += positions[i, 2]
            ez = wz - cam_z
            if ez <= 0.1:
                ok = False
                break
            ndc_x = (wx * f / aspect) / ez
            ndc_y = (wy * f) / ez
            out[i, k, 0] = (ndc_x * 0.5 + 0.5) * width
            out[i, k, 1] = (1.0 - (ndc_y * 0.5 + 0.5)) * height
        visible[i] = ok
    return out, visible


# ============================================================
# HUD
# ============================================================

class HUD:
    def __init__(self, font):
        self.font = font
        self.show = True
        self.frame_times = []
        self.max_fps = 0.0
        self.last_t = time.perf_counter()

    def tick(self):
        now = time.perf_counter()
        ft = (now - self.last_t) * 1000.0
        self.last_t = now
        self.frame_times.append(ft)
        if len(self.frame_times) > 240:
            self.frame_times.pop(0)
        if ft > 0:
            inst_fps = 1000.0 / ft
            if inst_fps > self.max_fps:
                self.max_fps = inst_fps

    def reset_max(self):
        self.max_fps = 0.0

    def stats(self):
        if not self.frame_times:
            return 0.0, 0.0, 0.0, 0.0
        arr = np.array(self.frame_times)
        avg = float(arr.mean())
        p99 = float(np.percentile(arr, 99))
        fps_avg = 1000.0 / avg if avg > 0 else 0.0
        inst = 1000.0 / np.maximum(arr, 1e-6)
        k = max(1, int(len(inst) * 0.01))
        low1 = float(np.sort(inst)[:k].mean())
        return fps_avg, p99, low1, self.max_fps

    def draw(self, surf, stack, test, objects, paused, measuring, measure_left, gval):
        if not self.show:
            return
        fps_avg, p99, low1, max_fps = self.stats()
        avg_ms = 1000.0 / fps_avg if fps_avg > 0 else 0.0

        lines = [
            f"Stack:    {stack}",
            f"Test:     {test}",
            f"FPS:      {fps_avg:6.1f}",
            f"Frame:    {avg_ms:5.2f} ms (avg) / {p99:5.2f} ms (p99)",
            f"Max FPS:  {max_fps:6.1f}",
            f"1% Low:   {low1:6.1f}",
            f"Objects:  {objects}",
            f"G:        {gval:.5f}" if gval is not None else "G:        -",
            f"Paused:   {paused}",
            f"Measuring:{' ' + str(int(measure_left)) + 's' if measuring else ' no'}",
            "[1] T1  [2] T2  [3] T3  [Space] Pause  [R] Reset",
            "[I] Info  [S] Measure  [Esc] Quit",
            "[Up/Down] G",
        ]
        y = 6
        for line in lines:
            s = self.font.render(line, True, (230, 230, 230))
            surf.blit(s, (8, y + 1))
            y += s.get_height() + 2


# ============================================================
# 场景
# ============================================================

class Scene:
    name = "BASE"
    objects = 0

    def reset(self): pass
    def update(self, dt): pass
    def draw(self, surf): pass
    def set_objects(self, n): pass


class T1NBody(Scene):
    name = "T1 - N-Body"

    def __init__(self, data, n=1000):
        self.all = data
        self.n = n
        self.reset()

    def reset(self):
        d = self.all[: self.n]
        self.x = d[:, 0].copy()
        self.y = d[:, 1].copy()
        self.vx = d[:, 2].copy()
        self.vy = d[:, 3].copy()
        self.mass = d[:, 4].copy()
        self.G = 0.0002
        self.dt = 0.016
        self.a_max = 3.0

    def set_objects(self, n):
        self.n = n
        self.reset()

    @property
    def objects(self):
        return self.n

    def update(self, dt):
        nbody_step(self.x, self.y, self.vx, self.vy, self.mass,
                   self.dt, self.G, self.a_max)

    def draw(self, surf):
        scale = min(WIDTH, HEIGHT) * 0.4
        sx = (self.x * scale + WIDTH / 2).astype(np.int32)
        sy = (self.y * scale + HEIGHT / 2).astype(np.int32)
        r = 2 if self.n <= 1000 else 1
        color = (180, 220, 255)
        for i in range(self.n):
            px = int(sx[i]); py = int(sy[i])
            if 0 <= px < WIDTH and 0 <= py < HEIGHT:
                pygame.draw.rect(surf, color, (px - r, py - r, r * 2, r * 2))


class T2Squares(Scene):
    name = "T2 - 2D Squares"

    def __init__(self, data, n=1000):
        self.all = data
        self.n = n
        self.reset()

    def reset(self):
        d = self.all[: self.n]
        self.x = d[:, 0].copy()
        self.y = d[:, 1].copy()
        self.vx = d[:, 2].copy()
        self.vy = d[:, 3].copy()
        self.size = d[:, 4].copy()
        self.color = d[:, 5:8].astype(np.uint8).copy()
        self.x0 = WIDTH / 2 - 200
        self.x1 = WIDTH / 2 + 200
        self.y0 = HEIGHT / 2 - 200
        self.y1 = HEIGHT / 2 + 200
        self.sx = self.x + self.x0
        self.sy = self.y + self.y0

    def set_objects(self, n):
        self.n = n
        self.reset()

    @property
    def objects(self):
        return self.n

    def update(self, dt):
        update_squares_numba(self.sx, self.sy, self.vx, self.vy, self.x0, self.x1, self.y0, self.y1)

    def draw(self, surf):
        for i in range(self.n):
            c = (int(self.color[i, 0]), int(self.color[i, 1]), int(self.color[i, 2]))
            s = int(self.size[i])
            pygame.draw.rect(surf, c,
                pygame.Rect(int(self.sx[i]), int(self.sy[i]), s, s))


class T3Cubes(Scene):
    name = "T3 - 3D Cubes"

    def __init__(self, data, n=200):
        self.all = data
        self.n = n
        self.reset()

    def reset(self):
        d = self.all[: self.n]
        self.pos = d[:, 0:3].copy()
        self.axes = d[:, 3:6].copy()
        self.speeds = d[:, 6].copy()
        self.color = d[:, 7:10].astype(np.uint8).copy()
        self.angles = np.zeros(self.n, dtype=np.float64)

    def set_objects(self, n):
        self.n = n
        self.reset()

    @property
    def objects(self):
        return self.n

    def update(self, dt):
        rotate_cubes(self.angles, self.speeds, dt)

    def draw(self, surf):
        pts, visible = project_cubes(self.angles, self.axes, self.pos, float(WIDTH), float(HEIGHT))
        edges = [(0,1),(1,2),(2,3),(3,0),(4,5),(5,6),(6,7),(7,4),(0,4),(1,5),(2,6),(3,7)]
        for i in range(self.n):
            if not visible[i]:
                continue
            c = (int(self.color[i, 0]), int(self.color[i, 1]), int(self.color[i, 2]))
            for a, b in edges:
                pygame.gfxdraw.line(surf,
                    int(pts[i, a, 0]), int(pts[i, a, 1]),
                    int(pts[i, b, 0]), int(pts[i, b, 1]), c)


# ============================================================
# 测量器
# ============================================================

class Measurer:
    def __init__(self, duration):
        self.duration = duration
        self.active = False
        self.start_t = 0.0
        self.frames = []
        self.last_t = 0.0
        self.just_finished = False

    def start(self):
        self.active = True
        self.start_t = time.perf_counter()
        self.last_t = self.start_t
        self.frames = []
        self.just_finished = False

    def tick(self):
        if not self.active:
            return
        now = time.perf_counter()
        self.frames.append(now - self.last_t)
        self.last_t = now
        if now - self.start_t >= self.duration:
            self.active = False
            self.just_finished = True

    @property
    def elapsed(self):
        if not self.active:
            return self.duration if self.just_finished else 0.0
        return time.perf_counter() - self.start_t

    @property
    def left(self):
        return max(0.0, self.duration - self.elapsed)


# ============================================================
# 工具
# ============================================================

def load_csv(path):
    return np.loadtxt(path, delimiter=",", skiprows=1)


def write_result(measurer, scene, startup_ms):
    arr = np.array(measurer.frames) * 1000.0
    if len(arr) == 0:
        arr = np.array([16.67])
    inst = 1000.0 / np.maximum(arr, 1e-6)
    k = max(1, int(len(inst) * 0.01))
    low1 = float(np.sort(inst)[:k].mean())
    result = {
        "stack": STACK_NAME,
        "test": scene.name.split(" ")[0],
        "objects": scene.objects,
        "resolution": [WIDTH, HEIGHT],
        "avg_fps": float(1000.0 / arr.mean()),
        "min_fps": float(1000.0 / arr.max()),
        "max_fps": float(1000.0 / arr.min()),
        "avg_frame_ms": float(arr.mean()),
        "p99_frame_ms": float(np.percentile(arr, 99)),
        "one_percent_low_fps": low1,
        "startup_ms": float(startup_ms),
        "duration_s": float(measurer.duration),
        "runtime": f"python {sys.version.split()[0]} + pygame {pygame.version.ver} + numba",
        "os": platform.platform(),
    }
    os.makedirs(RESULTS_DIR, exist_ok=True)
    fn = os.path.join(RESULTS_DIR,
                      f"{STACK_NAME}-{result['test']}-{scene.objects}.json")
    with open(fn, "w", encoding="utf-8") as f:
        json.dump(result, f, indent=2, ensure_ascii=False)
    print("Wrote", fn)
    return result


# ============================================================
# 主循环
# ============================================================

def main():
    pygame.init()

    try:
        icon = pygame.image.load("data/icon.png")
        pygame.display.set_icon(icon)
    except Exception as e:
        print("icon not found:", e)

    screen = pygame.display.set_mode((WIDTH, HEIGHT))
    pygame.display.set_caption(f"Perf Compare - {STACK_NAME}")
    clock = pygame.time.Clock()
    font = pygame.font.SysFont("consolas", 14)
    hud = HUD(font)

    particles = load_csv(os.path.join(DATA_DIR, "particles.csv"))
    squares = load_csv(os.path.join(DATA_DIR, "squares.csv"))
    cubes = load_csv(os.path.join(DATA_DIR, "cubes.csv"))

    scenes = [
        T1NBody(particles, 1000),
        T2Squares(squares, 1000),
        T3Cubes(cubes, 200),
    ]
    scene_idx = 0
    paused = False
    measurer = Measurer(DURATION)

    startup_t0 = time.perf_counter()
    startup_ms = 0.0
    first_frame = True

    running = True
    while running:
        for e in pygame.event.get():
            if e.type == pygame.QUIT:
                if measurer.frames:
                    write_result(measurer, scenes[scene_idx], startup_ms)
                running = False
            elif e.type == pygame.KEYDOWN:
                if e.key == pygame.K_ESCAPE:
                    if measurer.frames:
                        write_result(measurer, scenes[scene_idx], startup_ms)
                    running = False
                elif e.key == pygame.K_1:
                    scene_idx = 0
                    hud.reset_max()
                elif e.key == pygame.K_2:
                    scene_idx = 1
                    hud.reset_max()
                elif e.key == pygame.K_3:
                    scene_idx = 2
                    hud.reset_max()
                elif e.key == pygame.K_SPACE:
                    paused = not paused
                elif e.key == pygame.K_r:
                    scenes[scene_idx].reset()
                    hud.reset_max()
                elif e.key == pygame.K_i:
                    hud.show = not hud.show
                elif e.key == pygame.K_s:
                    if not measurer.active:
                        measurer.start()
                        hud.reset_max()
                        print("Measuring started...")
                elif e.key == pygame.K_UP:
                    if scene_idx == 0:
                        scenes[0].G *= 1.5
                        print("G =", scenes[0].G)
                elif e.key == pygame.K_DOWN:
                    if scene_idx == 0:
                        scenes[0].G /= 1.5
                        print("G =", scenes[0].G)

        raw_dt = clock.tick(FPS_CAP)
        dt = min(raw_dt / 1000.0, 0.05)

        scene = scenes[scene_idx]
        if not paused:
            scene.update(dt)

        screen.fill((20, 20, 28))
        scene.draw(screen)

        hud.tick()
        measurer.tick()

        if measurer.just_finished:
            measurer.just_finished = False
            write_result(measurer, scene, startup_ms)

        gval = scenes[0].G if scene_idx == 0 else None
        hud.draw(
            screen, STACK_NAME, scene.name, scene.objects,
            paused, measurer.active, measurer.left, gval,
        )
        pygame.display.flip()

        if first_frame:
            startup_ms = (time.perf_counter() - startup_t0) * 1000.0
            first_frame = False

    pygame.quit()


if __name__ == "__main__":
    main()