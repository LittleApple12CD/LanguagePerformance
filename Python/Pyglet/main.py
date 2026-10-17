import os
import sys
import json
import time
import math
import platform
import ctypes

import numpy as np
import pyglet
import pyglet.gl as pgl
from pyglet.window import key
from numba import njit

# ---------- 配置 ----------
WIDTH, HEIGHT = 1280, 720
DURATION = 30.0
STACK_NAME = "python-pyglet-gl-numba"
RESULTS_DIR = "results"
DATA_DIR = "data"


# ============================================================
# 手动加载 OpenGL 扩展函数（Windows）
# 所有指针参数统一用 c_void_p，避免 ctypes 严格类型检查报错
# ============================================================

def _load_gl_extensions():
    if platform.system() != "Windows":
        return
    try:
        opengl32 = ctypes.WinDLL("opengl32")
    except OSError:
        return

    wglGetProcAddress = opengl32.wglGetProcAddress
    wglGetProcAddress.restype = ctypes.c_void_p
    wglGetProcAddress.argtypes = [ctypes.c_char_p]

    def _get(name):
        addr = wglGetProcAddress(name.encode("ascii"))
        if not addr:
            try:
                addr = ctypes.cast(getattr(opengl32, name), ctypes.c_void_p).value
            except AttributeError:
                addr = 0
        return addr

    g = globals()

    def _bind(name, restype, argtypes):
        addr = _get(name)
        if not addr:
            print(f"[GL] WARNING: cannot load {name}")
            return
        func = ctypes.CFUNCTYPE(restype, *argtypes)(addr)
        setattr(pgl, name, func)
        g[name] = func

    V = ctypes.c_void_p
    U = ctypes.c_uint
    I = ctypes.c_int
    F = ctypes.c_float
    B = ctypes.c_ubyte
    LL = ctypes.c_longlong
    C = ctypes.c_char_p

    # 基础
    _bind("glGetString", C, [U])
    _bind("glGetError", U, [])
    _bind("glClearColor", None, [F, F, F, F])
    _bind("glClear", None, [U])
    _bind("glEnable", None, [U])
    _bind("glDisable", None, [U])
    _bind("glViewport", None, [I, I, I, I])

    # 缓冲
    _bind("glGenBuffers", None, [I, V])
    _bind("glBindBuffer", None, [U, U])
    _bind("glBufferData", None, [U, LL, V, U])
    _bind("glBufferSubData", None, [U, LL, LL, V])

    # VAO
    _bind("glGenVertexArrays", None, [I, V])
    _bind("glBindVertexArray", None, [U])
    _bind("glDeleteVertexArrays", None, [I, V])

    # 顶点属性
    _bind("glEnableVertexAttribArray", None, [U])
    _bind("glDisableVertexAttribArray", None, [U])
    _bind("glVertexAttribPointer", None, [U, I, U, B, I, V])
    _bind("glVertexAttribDivisor", None, [U, U])

    # 绘制
    _bind("glDrawArrays", None, [U, I, I])
    _bind("glDrawElements", None, [U, I, U, V])
    _bind("glDrawArraysInstanced", None, [U, I, I, I])
    _bind("glDrawElementsInstanced", None, [U, I, U, V, I])

    # 着色器
    _bind("glCreateShader", U, [U])
    _bind("glShaderSource", None, [U, I, V, V])
    _bind("glCompileShader", None, [U])
    _bind("glGetShaderiv", None, [U, U, V])
    _bind("glGetShaderInfoLog", None, [U, I, V, V])
    _bind("glCreateProgram", U, [])
    _bind("glAttachShader", None, [U, U])
    _bind("glLinkProgram", None, [U])
    _bind("glGetProgramiv", None, [U, U, V])
    _bind("glGetProgramInfoLog", None, [U, I, V, V])
    _bind("glDeleteShader", None, [U])
    _bind("glUseProgram", None, [U])
    _bind("glGetUniformLocation", I, [U, C])
    _bind("glUniform1f", None, [I, F])
    _bind("glUniform2f", None, [I, F, F])
    _bind("glUniformMatrix4fv", None, [I, I, B, V])

    # 深度/混合
    _bind("glDepthFunc", None, [U])
    _bind("glBlendFunc", None, [U, U])


_load_gl_extensions()


# ============================================================
# Numba 加速
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
            s = a_max / np.sqrt(a2)
            ax[i] *= s
            ay[i] *= s
        vx[i] += ax[i] * dt
        vy[i] += ay[i] * dt
        x[i] += vx[i] * dt
        y[i] += vy[i] * dt


@njit(cache=True, fastmath=True)
def rotate_cubes(angles, speeds, dt):
    for i in range(angles.shape[0]):
        angles[i] += speeds[i] * dt


@njit(cache=True, fastmath=True)
def build_cube_triangles(angles, axes, positions, colors, s, out):
    """
    每个立方体：8 顶点旋转+平移+透视投影到 NDC，6 面背面剔除，
    可见面拆 2 三角形，输出 (n*36, 5) float32：(ndc_x, ndc_y, r, g, b)
    远 → 近 排序（画家算法）
    """
    n = angles.shape[0]

    base = np.empty((8, 3), dtype=np.float64)
    base[0, 0] = -s; base[0, 1] = -s; base[0, 2] = -s
    base[1, 0] =  s; base[1, 1] = -s; base[1, 2] = -s
    base[2, 0] =  s; base[2, 1] =  s; base[2, 2] = -s
    base[3, 0] = -s; base[3, 1] =  s; base[3, 2] = -s
    base[4, 0] = -s; base[4, 1] = -s; base[4, 2] =  s
    base[5, 0] =  s; base[5, 1] = -s; base[5, 2] =  s
    base[6, 0] =  s; base[6, 1] =  s; base[6, 2] =  s
    base[7, 0] = -s; base[7, 1] =  s; base[7, 2] =  s

    face_v = np.empty((6, 4), dtype=np.int32)
    face_v[0, 0] = 0; face_v[0, 1] = 1; face_v[0, 2] = 2; face_v[0, 3] = 3
    face_v[1, 0] = 4; face_v[1, 1] = 5; face_v[1, 2] = 6; face_v[1, 3] = 7
    face_v[2, 0] = 0; face_v[2, 1] = 1; face_v[2, 2] = 5; face_v[2, 3] = 4
    face_v[3, 0] = 3; face_v[3, 1] = 2; face_v[3, 2] = 6; face_v[3, 3] = 7
    face_v[4, 0] = 0; face_v[4, 1] = 3; face_v[4, 2] = 7; face_v[4, 3] = 4
    face_v[5, 0] = 1; face_v[5, 1] = 2; face_v[5, 2] = 6; face_v[5, 3] = 5

    fn = np.empty((6, 3), dtype=np.float64)
    fn[0, 0] =  0.0; fn[0, 1] =  0.0; fn[0, 2] = -1.0
    fn[1, 0] =  0.0; fn[1, 1] =  0.0; fn[1, 2] =  1.0
    fn[2, 0] =  0.0; fn[2, 1] = -1.0; fn[2, 2] =  0.0
    fn[3, 0] =  0.0; fn[3, 1] =  1.0; fn[3, 2] =  0.0
    fn[4, 0] = -1.0; fn[4, 1] =  0.0; fn[4, 2] =  0.0
    fn[5, 0] =  1.0; fn[5, 1] =  0.0; fn[5, 2] =  0.0

    cam_z = 8.0
    f = 1.0 / math.tan(math.radians(60.0) / 2.0)
    aspect = WIDTH / HEIGHT

    order = np.empty(n, dtype=np.int32)
    center_d = np.empty(n, dtype=np.float64)
    for i in range(n):
        order[i] = i
        center_d[i] = cam_z - positions[i, 2]

    for i in range(1, n):
        key_idx = order[i]
        kd = center_d[key_idx]
        j = i - 1
        while j >= 0 and center_d[order[j]] < kd:
            order[j + 1] = order[j]
            j -= 1
        order[j + 1] = key_idx

    world = np.empty((8, 3), dtype=np.float64)
    screen_ndc = np.empty((8, 2), dtype=np.float32)

    shade = np.empty(6, dtype=np.float32)
    shade[0] = 1.0
    shade[1] = 0.85
    shade[2] = 0.6
    shade[3] = 0.7
    shade[4] = 0.5
    shade[5] = 0.65

    for oi in range(n):
        i = order[oi]
        a = angles[i]
        ax = axes[i, 0]; ay = axes[i, 1]; az = axes[i, 2]
        c = math.cos(a); sn = math.sin(a)
        r00 = c + ax * ax * (1 - c)
        r01 = ax * ay * (1 - c) - az * sn
        r02 = ax * az * (1 - c) + ay * sn
        r10 = ay * ax * (1 - c) + az * sn
        r11 = c + ay * ay * (1 - c)
        r12 = ay * az * (1 - c) - ax * sn
        r20 = az * ax * (1 - c) - ay * sn
        r21 = az * ay * (1 - c) + ax * sn
        r22 = c + az * az * (1 - c)

        cx = positions[i, 0]; cy = positions[i, 1]; cz = positions[i, 2]

        for k in range(8):
            bx = base[k, 0]; by = base[k, 1]; bz = base[k, 2]
            wx = r00 * bx + r01 * by + r02 * bz + cx
            wy = r10 * bx + r11 * by + r12 * bz + cy
            wz = r20 * bx + r21 * by + r22 * bz + cz
            world[k, 0] = wx; world[k, 1] = wy; world[k, 2] = wz
            depth = cam_z - wz
            if depth < 0.05:
                depth = 0.05
            scale = f / depth
            screen_ndc[k, 0] = wx * scale / aspect
            screen_ndc[k, 1] = wy * scale

        r0 = colors[i, 0]; g0 = colors[i, 1]; b0 = colors[i, 2]

        write_idx = oi * 36
        for fi in range(6):
            lx = fn[fi, 0]; ly = fn[fi, 1]; lz = fn[fi, 2]
            wnx = r00 * lx + r01 * ly + r02 * lz
            wny = r10 * lx + r11 * ly + r12 * lz
            wnz = r20 * lx + r21 * ly + r22 * lz
            v0 = face_v[fi, 0]; v2 = face_v[fi, 2]
            fcx = (world[v0, 0] + world[v2, 0]) * 0.5
            fcy = (world[v0, 1] + world[v2, 1]) * 0.5
            fcz = (world[v0, 2] + world[v2, 2]) * 0.5
            vx_ = -fcx; vy_ = -fcy; vz_ = cam_z - fcz
            dot = wnx * vx_ + wny * vy_ + wnz * vz_
            if dot <= 0.0:
                for t in range(6):
                    out[write_idx + t, 0] = 0.0
                    out[write_idx + t, 1] = 0.0
                    out[write_idx + t, 2] = 0.0
                    out[write_idx + t, 3] = 0.0
                    out[write_idx + t, 4] = 0.0
                write_idx += 6
                continue

            sh = shade[fi]
            fr = r0 * sh; fg = g0 * sh; fb = b0 * sh

            x0 = screen_ndc[v0, 0]; y0 = screen_ndc[v0, 1]
            v1 = face_v[fi, 1]
            v3 = face_v[fi, 3]
            x1 = screen_ndc[v1, 0]; y1 = screen_ndc[v1, 1]
            x2 = screen_ndc[v2, 0]; y2 = screen_ndc[v2, 1]
            x3 = screen_ndc[v3, 0]; y3 = screen_ndc[v3, 1]

            out[write_idx + 0, 0] = x0; out[write_idx + 0, 1] = y0
            out[write_idx + 0, 2] = fr; out[write_idx + 0, 3] = fg; out[write_idx + 0, 4] = fb
            out[write_idx + 1, 0] = x1; out[write_idx + 1, 1] = y1
            out[write_idx + 1, 2] = fr; out[write_idx + 1, 3] = fg; out[write_idx + 1, 4] = fb
            out[write_idx + 2, 0] = x2; out[write_idx + 2, 1] = y2
            out[write_idx + 2, 2] = fr; out[write_idx + 2, 3] = fg; out[write_idx + 2, 4] = fb
            out[write_idx + 3, 0] = x0; out[write_idx + 3, 1] = y0
            out[write_idx + 3, 2] = fr; out[write_idx + 3, 3] = fg; out[write_idx + 3, 4] = fb
            out[write_idx + 4, 0] = x2; out[write_idx + 4, 1] = y2
            out[write_idx + 4, 2] = fr; out[write_idx + 4, 3] = fg; out[write_idx + 4, 4] = fb
            out[write_idx + 5, 0] = x3; out[write_idx + 5, 1] = y3
            out[write_idx + 5, 2] = fr; out[write_idx + 5, 3] = fg; out[write_idx + 5, 4] = fb

            write_idx += 6


# ============================================================
# OpenGL 工具
# ============================================================

def compile_shader(src, shader_type):
    sid = pgl.glCreateShader(shader_type)
    src_bytes = src.encode("utf-8")
    src_array = (ctypes.c_char_p * 1)(src_bytes)
    c_text = ctypes.cast(src_array, ctypes.c_void_p)
    pgl.glShaderSource(sid, 1, c_text, None)
    pgl.glCompileShader(sid)
    ok = ctypes.c_int()
    pgl.glGetShaderiv(sid, pgl.GL_COMPILE_STATUS, ctypes.byref(ok))
    if not ok.value:
        log = ctypes.create_string_buffer(4096)
        pgl.glGetShaderInfoLog(sid, 4096, None, log)
        raise RuntimeError("Shader compile failed:\n" + log.value.decode("utf-8", "replace"))
    return sid


def link_program(vs_src, fs_src):
    vs = compile_shader(vs_src, pgl.GL_VERTEX_SHADER)
    fs = compile_shader(fs_src, pgl.GL_FRAGMENT_SHADER)
    prog = pgl.glCreateProgram()
    pgl.glAttachShader(prog, vs)
    pgl.glAttachShader(prog, fs)
    pgl.glLinkProgram(prog)
    ok = ctypes.c_int()
    pgl.glGetProgramiv(prog, pgl.GL_LINK_STATUS, ctypes.byref(ok))
    if not ok.value:
        log = ctypes.create_string_buffer(4096)
        pgl.glGetProgramInfoLog(prog, 4096, None, log)
        raise RuntimeError("Program link failed:\n" + log.value.decode("utf-8", "replace"))
    pgl.glDeleteShader(vs)
    pgl.glDeleteShader(fs)
    return prog


def make_buffer(data, target=None, usage=None):
    if target is None:
        target = pgl.GL_ARRAY_BUFFER
    if usage is None:
        usage = pgl.GL_DYNAMIC_DRAW
    buf = ctypes.c_uint()
    pgl.glGenBuffers(1, ctypes.byref(buf))
    pgl.glBindBuffer(target, buf)
    arr = np.ascontiguousarray(data)
    pgl.glBufferData(target,
                     ctypes.c_longlong(arr.nbytes),
                     arr.ctypes.data_as(ctypes.c_void_p),
                     usage)
    return buf


def make_vao():
    vao = ctypes.c_uint()
    pgl.glGenVertexArrays(1, ctypes.byref(vao))
    return vao


# ============================================================
# T1: N-Body
# ============================================================

T1_VS = """
#version 330 core
layout(location=0) in vec2 aPos;
uniform float uScale;
uniform float uAspect;
void main() {
    vec2 p = aPos * uScale;
    p.x /= uAspect;
    gl_Position = vec4(p, 0.0, 1.0);
    gl_PointSize = 3.0;
}
"""

T1_FS = """
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(0.7, 0.85, 1.0, 1.0);
}
"""


class T1NBody:
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
        self.scale = 0.4
        self.vertex_data = np.zeros((self.n, 2), dtype=np.float32)
        self._sync_vertices()

        self.prog = link_program(T1_VS, T1_FS)
        self.u_scale = pgl.glGetUniformLocation(self.prog, b"uScale")
        self.u_aspect = pgl.glGetUniformLocation(self.prog, b"uAspect")

        self.vao = make_vao()
        pgl.glBindVertexArray(self.vao)
        self.vbo = make_buffer(self.vertex_data)
        pgl.glBindBuffer(pgl.GL_ARRAY_BUFFER, self.vbo)
        pgl.glEnableVertexAttribArray(0)
        pgl.glVertexAttribPointer(0, 2, pgl.GL_FLOAT, pgl.GL_FALSE, 0, None)
        pgl.glBindVertexArray(0)

    def _sync_vertices(self):
        self.vertex_data[:, 0] = self.x
        self.vertex_data[:, 1] = self.y

    def set_objects(self, n):
        self.n = n
        self.reset()

    @property
    def objects(self):
        return self.n

    def update(self, dt):
        nbody_step(self.x, self.y, self.vx, self.vy, self.mass,
                   self.dt, self.G, self.a_max)

    def draw(self):
        pgl.glBindVertexArray(self.vao)
        pgl.glUseProgram(self.prog)
        pgl.glUniform1f(self.u_scale, self.scale)
        pgl.glUniform1f(self.u_aspect, float(WIDTH) / float(HEIGHT))
        self._sync_vertices()
        pgl.glBindBuffer(pgl.GL_ARRAY_BUFFER, self.vbo)
        pgl.glBufferSubData(pgl.GL_ARRAY_BUFFER, 0,
                            ctypes.c_longlong(self.vertex_data.nbytes),
                            self.vertex_data.ctypes.data_as(ctypes.c_void_p))
        pgl.glDrawArrays(pgl.GL_POINTS, 0, self.n)
        pgl.glBindVertexArray(0)


# ============================================================
# T2: 2D 方块
# ============================================================

T2_VS = """
#version 330 core
layout(location=0) in vec2 aCorner;
layout(location=1) in vec2 iPos;
layout(location=2) in vec2 iSize;
layout(location=3) in vec3 iColor;
out vec3 vColor;
uniform vec2 uRes;
void main() {
    vec2 world = iPos + aCorner * iSize;
    vec2 ndc = (world / uRes) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
    vColor = iColor;
}
"""

T2_FS = """
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
"""


class T2Squares:
    name = "T2 - 2D Squares"

    def __init__(self, data, n=1000):
        self.all = data
        self.n = n
        self.reset()

    def reset(self):
        d = self.all[: self.n]
        self.x = d[:, 0].copy() + WIDTH / 2 - 200
        self.y = d[:, 1].copy() + HEIGHT / 2 - 200
        self.vx = d[:, 2].copy()
        self.vy = d[:, 3].copy()
        self.size = d[:, 4].copy()
        self.color = d[:, 5:8].astype(np.float32) / 255.0
        self.x0 = WIDTH / 2 - 200
        self.x1 = WIDTH / 2 + 200
        self.y0 = HEIGHT / 2 - 200
        self.y1 = HEIGHT / 2 + 200

        quad = np.array([
            [0, 0], [1, 0], [0, 1],
            [0, 1], [1, 0], [1, 1],
        ], dtype=np.float32)

        self.instance_data = np.zeros((self.n, 7), dtype=np.float32)
        self._sync_instances()

        self.prog = link_program(T2_VS, T2_FS)
        self.u_res = pgl.glGetUniformLocation(self.prog, b"uRes")

        self.vao = make_vao()
        pgl.glBindVertexArray(self.vao)

        self.quad_vbo = make_buffer(quad)
        pgl.glBindBuffer(pgl.GL_ARRAY_BUFFER, self.quad_vbo)
        pgl.glEnableVertexAttribArray(0)
        pgl.glVertexAttribPointer(0, 2, pgl.GL_FLOAT, pgl.GL_FALSE, 0, None)
        pgl.glVertexAttribDivisor(0, 0)

        self.inst_vbo = make_buffer(self.instance_data)
        pgl.glBindBuffer(pgl.GL_ARRAY_BUFFER, self.inst_vbo)
        stride = 7 * 4
        pgl.glEnableVertexAttribArray(1)
        pgl.glVertexAttribPointer(1, 2, pgl.GL_FLOAT, pgl.GL_FALSE, stride, ctypes.c_void_p(0))
        pgl.glVertexAttribDivisor(1, 1)
        pgl.glEnableVertexAttribArray(2)
        pgl.glVertexAttribPointer(2, 2, pgl.GL_FLOAT, pgl.GL_FALSE, stride, ctypes.c_void_p(8))
        pgl.glVertexAttribDivisor(2, 1)
        pgl.glEnableVertexAttribArray(3)
        pgl.glVertexAttribPointer(3, 3, pgl.GL_FLOAT, pgl.GL_FALSE, stride, ctypes.c_void_p(16))
        pgl.glVertexAttribDivisor(3, 1)

        pgl.glBindVertexArray(0)

    def _sync_instances(self):
        self.instance_data[:, 0] = self.x
        self.instance_data[:, 1] = self.y
        self.instance_data[:, 2] = self.size
        self.instance_data[:, 3] = self.size
        self.instance_data[:, 4:7] = self.color

    def set_objects(self, n):
        self.n = n
        self.reset()

    @property
    def objects(self):
        return self.n

    def update(self, dt):
        self.x += self.vx
        self.y += self.vy
        for i in range(self.n):
            if self.x[i] < self.x0 or self.x[i] > self.x1:
                self.vx[i] = -self.vx[i]
                self.x[i] = min(max(self.x[i], self.x0), self.x1)
            if self.y[i] < self.y0 or self.y[i] > self.y1:
                self.vy[i] = -self.vy[i]
                self.y[i] = min(max(self.y[i], self.y0), self.y1)

    def draw(self):
        pgl.glBindVertexArray(self.vao)
        self._sync_instances()
        pgl.glBindBuffer(pgl.GL_ARRAY_BUFFER, self.inst_vbo)
        pgl.glBufferSubData(pgl.GL_ARRAY_BUFFER, 0,
                            ctypes.c_longlong(self.instance_data.nbytes),
                            self.instance_data.ctypes.data_as(ctypes.c_void_p))
        pgl.glUseProgram(self.prog)
        pgl.glUniform2f(self.u_res, float(WIDTH), float(HEIGHT))
        pgl.glDrawArraysInstanced(pgl.GL_TRIANGLES, 0, 6, self.n)
        pgl.glBindVertexArray(0)


# ============================================================
# T3: 3D 立方体（手绘顶点，Numba 计算）
# ============================================================

T3_VS = """
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec3 aColor;
out vec3 vColor;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}
"""

T3_FS = """
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
"""


class T3Cubes:
    name = "T3 - 3D Cubes"

    def __init__(self, data, n=200):
        self.all = data
        self.n = n
        self.reset()

    def reset(self):
        d = self.all[: self.n]
        self.pos = d[:, 0:3].astype(np.float64)
        self.axes = d[:, 3:6].copy()
        self.speeds = d[:, 6].copy()
        if d.shape[1] >= 10:
            self.color = d[:, 7:10].astype(np.float32) / 255.0
        else:
            self.color = np.tile(np.array([[0.6, 0.8, 1.0]], dtype=np.float32), (self.n, 1))
        self.angles = np.zeros(self.n, dtype=np.float64)
        self.s = 0.25

        self.vertex_data = np.zeros((self.n * 36, 5), dtype=np.float32)

        self.prog = link_program(T3_VS, T3_FS)

        self.vao = make_vao()
        pgl.glBindVertexArray(self.vao)

        self.vbo = make_buffer(self.vertex_data, pgl.GL_ARRAY_BUFFER, pgl.GL_DYNAMIC_DRAW)
        pgl.glBindBuffer(pgl.GL_ARRAY_BUFFER, self.vbo)
        stride = 5 * 4
        pgl.glEnableVertexAttribArray(0)
        pgl.glVertexAttribPointer(0, 2, pgl.GL_FLOAT, pgl.GL_FALSE, stride, ctypes.c_void_p(0))
        pgl.glEnableVertexAttribArray(1)
        pgl.glVertexAttribPointer(1, 3, pgl.GL_FLOAT, pgl.GL_FALSE, stride, ctypes.c_void_p(8))

        pgl.glBindVertexArray(0)

    def set_objects(self, n):
        self.n = n
        self.reset()

    @property
    def objects(self):
        return self.n

    def update(self, dt):
        rotate_cubes(self.angles, self.speeds, dt)

    def draw(self):
        build_cube_triangles(self.angles, self.axes, self.pos, self.color,
                             self.s, self.vertex_data)
        pgl.glBindVertexArray(self.vao)
        pgl.glBindBuffer(pgl.GL_ARRAY_BUFFER, self.vbo)
        pgl.glBufferSubData(pgl.GL_ARRAY_BUFFER, 0,
                            ctypes.c_longlong(self.vertex_data.nbytes),
                            self.vertex_data.ctypes.data_as(ctypes.c_void_p))
        pgl.glUseProgram(self.prog)
        pgl.glDrawArrays(pgl.GL_TRIANGLES, 0, self.n * 36)
        pgl.glBindVertexArray(0)


# ============================================================
# 测量器 / HUD / 工具 / 主循环
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


class HUD:
    def __init__(self):
        self.show = True
        self.frame_times = []
        self.max_fps = 0.0
        self.last_t = time.perf_counter()
        self.batch = pyglet.graphics.Batch()
        self._label = pyglet.text.Label(
            "", font_name="Consolas", font_size=12,
            x=8, y=HEIGHT - 8, anchor_x="left", anchor_y="top",
            color=(230, 230, 230, 255), multiline=True, width=WIDTH - 16,
            batch=self.batch,
        )

    def tick(self):
        now = time.perf_counter()
        ft = (now - self.last_t) * 1000.0
        self.last_t = now
        self.frame_times.append(ft)
        if len(self.frame_times) > 240:
            self.frame_times.pop(0)
        if ft > 0:
            inst = 1000.0 / ft
            if inst > self.max_fps:
                self.max_fps = inst

    def reset_max(self):
        self.max_fps = 0.0

    def stats(self):
        if not self.frame_times:
            return 0.0, 0.0, 0.0, 0.0
        arr = np.array(self.frame_times)
        avg = float(arr.mean())
        p99 = float(np.percentile(arr, 99))
        fps = 1000.0 / avg if avg > 0 else 0.0
        inst = 1000.0 / np.maximum(arr, 1e-6)
        k = max(1, int(len(inst) * 0.01))
        low1 = float(np.sort(inst)[:k].mean())
        return fps, p99, low1, self.max_fps

    def draw(self, stack, test, objects, paused, measuring, left, gval):
        if not self.show:
            return
        fps, p99, low1, max_fps = self.stats()
        avg_ms = 1000.0 / fps if fps > 0 else 0.0
        lines = [
            f"Stack:    {stack}",
            f"Test:     {test}",
            f"FPS:      {fps:6.1f}",
            f"Frame:    {avg_ms:5.2f} ms (avg) / {p99:5.2f} ms (p99)",
            f"Max FPS:  {max_fps:6.1f}",
            f"1% Low:   {low1:6.1f}",
            f"Objects:  {objects}",
            f"G:        {gval:.5f}" if gval is not None else "G:        -",
            f"Paused:   {paused}",
            f"Measuring:{' ' + str(int(left)) + 's' if measuring else ' no'}",
            "[1] T1  [2] T2  [3] T3  [Space] Pause  [R] Reset",
            "[I] Info  [S] Measure  [Esc] Quit",
            "[Up/Down] G",
        ]
        self._label.text = "\n".join(lines)
        self.batch.draw()


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
        "runtime": f"python {sys.version.split()[0]} + pyglet {pyglet.version} + numba",
        "os": platform.platform(),
    }
    os.makedirs(RESULTS_DIR, exist_ok=True)
    fn = os.path.join(RESULTS_DIR,
                      f"{STACK_NAME}-{result['test']}-{scene.objects}.json")
    with open(fn, "w", encoding="utf-8") as f:
        json.dump(result, f, indent=2, ensure_ascii=False)
    print("Wrote", fn)


def main():
    window = pyglet.window.Window(WIDTH, HEIGHT, caption=f"Perf Compare - {STACK_NAME}")

    try:
        icon = pyglet.image.load("data/icon.png")
        window.set_icon(icon)
    except Exception as e:
        print("icon not found:", e)

    pgl.glClearColor(0.08, 0.08, 0.11, 1.0)

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
    hud = HUD()

    startup_t0 = time.perf_counter()
    startup_ms = 0.0
    first_frame = [True]

    @window.event
    def on_draw():
        pgl.glClear(pgl.GL_COLOR_BUFFER_BIT | pgl.GL_DEPTH_BUFFER_BIT)
        scene = scenes[scene_idx]
        pgl.glDisable(pgl.GL_DEPTH_TEST)
        scene.draw()
        pgl.glUseProgram(0)
        pgl.glBindVertexArray(0)
        hud.draw(STACK_NAME, scene.name, scene.objects,
                 paused, measurer.active, measurer.left,
                 scenes[0].G if scene_idx == 0 else None)
        if first_frame[0]:
            startup_ms = (time.perf_counter() - startup_t0) * 1000.0
            first_frame[0] = False

    @window.event
    def on_key_press(symbol, modifiers):
        nonlocal scene_idx, paused
        if symbol == key.ESCAPE:
            if measurer.frames:
                write_result(measurer, scenes[scene_idx], startup_ms)
            pyglet.app.exit()
        elif symbol == key._1:
            scene_idx = 0; hud.reset_max()
        elif symbol == key._2:
            scene_idx = 1; hud.reset_max()
        elif symbol == key._3:
            scene_idx = 2; hud.reset_max()
        elif symbol == key.SPACE:
            paused = not paused
        elif symbol == key.R:
            scenes[scene_idx].reset(); hud.reset_max()
        elif symbol == key.I:
            hud.show = not hud.show
        elif symbol == key.S:
            if not measurer.active:
                measurer.start(); hud.reset_max()
                print("Measuring started...")
        elif symbol == key.UP:
            if scene_idx == 0:
                scenes[0].G *= 1.5
                print("G =", scenes[0].G)
        elif symbol == key.DOWN:
            if scene_idx == 0:
                scenes[0].G /= 1.5
                print("G =", scenes[0].G)

    def update(dt):
        nonlocal startup_ms
        scene = scenes[scene_idx]
        if not paused:
            scene.update(dt)
        hud.tick()
        measurer.tick()
        if measurer.just_finished:
            measurer.just_finished = False
            write_result(measurer, scene, startup_ms)

    pyglet.clock.schedule_interval(update, 1.0 / 1000.0)
    pyglet.app.run()


if __name__ == "__main__":
    main()