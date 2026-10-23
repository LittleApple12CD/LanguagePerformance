import os
import numpy as np

os.makedirs("data", exist_ok=True)
rng = np.random.default_rng(42)

# ---------- T1: N-Body ----------
n = 2000
r = rng.uniform(0.1, 1.0, n)
theta = rng.uniform(0, 2 * np.pi, n)
x = r * np.cos(theta)
y = r * np.sin(theta)

v = 0.2
vx = -np.sin(theta) * v
vy =  np.cos(theta) * v

mass = np.ones(n)
p = np.column_stack([x, y, vx, vy, mass])
np.savetxt("data/particles.csv", p, delimiter=",",
           header="x,y,vx,vy,mass", comments="")
print("particles.csv:", p.shape)

# ---------- T2: 2D ----------
n = 2000
s = np.zeros((n, 8))
s[:, 0:2] = rng.uniform(0, 400, (n, 2))
s[:, 2:4] = rng.uniform(-2, 2, (n, 2))
s[:, 4] = rng.uniform(4, 20, n)
s[:, 5:8] = rng.uniform(50, 255, (n, 3))
np.savetxt("data/squares.csv", s, delimiter=",",
           header="x,y,vx,vy,size,r,g,b", comments="")
print("squares.csv:", s.shape)

# ---------- T3: 3D ----------
n = 200
c = np.zeros((n, 10))
cols = 20
rows = (n + cols - 1) // cols
for i in range(n):
    gx = i % cols
    gy = i // cols
    c[i, 0] = (gx - (cols - 1) / 2) * 0.8
    c[i, 1] = (gy - (rows - 1) / 2) * 0.8
    c[i, 2] = 0.0
    ax, ay, az = rng.normal(), rng.normal(), rng.normal()
    norm = (ax*ax + ay*ay + az*az) ** 0.5
    c[i, 3] = ax / norm
    c[i, 4] = ay / norm
    c[i, 5] = az / norm
    c[i, 6] = rng.uniform(0.5, 2.0)
    c[i, 7:10] = rng.uniform(50, 255, 3)
np.savetxt("data/cubes.csv", c, delimiter=",",
           header="px,py,pz,ax,ay,az,speed,r,g,b", comments="")
print("cubes.csv:", c.shape)