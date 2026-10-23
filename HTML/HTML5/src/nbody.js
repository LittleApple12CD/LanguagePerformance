class NBody {
  constructor(data, n) {
    this.n = n;
    this.all = data;
    this.G = 0.0002;
    this.dt = 0.016;
    this.aMax = 3.0;
    this.scale = 0.4;
    this.reset();
  }

  reset() {
    const n = this.n;
    this.x = new Float32Array(n);
    this.y = new Float32Array(n);
    this.vx = new Float32Array(n);
    this.vy = new Float32Array(n);
    this.mass = new Float32Array(n);
    for (let i = 0; i < n; i++) {
      this.x[i] = this.all[i][0];
      this.y[i] = this.all[i][1];
      this.vx[i] = this.all[i][2];
      this.vy[i] = this.all[i][3];
      this.mass[i] = this.all[i][4];
    }
    this.ax = new Float32Array(n);
    this.ay = new Float32Array(n);
    this.vertexData = new Float32Array(n * 2);
  }

  update() {
    const n = this.n, dt = this.dt, G = this.G, aMax = this.aMax;
    const x = this.x, y = this.y, vx = this.vx, vy = this.vy, mass = this.mass;
    const ax = this.ax, ay = this.ay;

    for (let i = 0; i < n; i++) {
      let axi = 0, ayi = 0;
      const xi = x[i], yi = y[i];
      for (let j = 0; j < n; j++) {
        const dx = x[j] - xi;
        const dy = y[j] - yi;
        const d2 = dx * dx + dy * dy + 1e-12; 
        const inv = 1.0 / d2;
        const f = G * inv * Math.sqrt(inv) * mass[j];
        axi += f * dx;
        ayi += f * dy;
      }
      ax[i] = axi;
      ay[i] = ayi;
    }
    for (let i = 0; i < n; i++) {
      let a2 = ax[i] * ax[i] + ay[i] * ay[i];
      if (a2 > aMax * aMax) {
        const s = aMax / Math.sqrt(a2);
        ax[i] *= s;
        ay[i] *= s;
      }
      vx[i] += ax[i] * dt;
      vy[i] += ay[i] * dt;
      x[i] += vx[i] * dt;
      y[i] += vy[i] * dt;
    }
  }

  uploadVertices() {
    const n = this.n;
    const vd = this.vertexData;
    const cx = 0, cy = 0;
    const s = this.scale;
    for (let i = 0; i < n; i++) {
      // NDC 坐标 [-1, 1]
      vd[i * 2]     = this.x[i] * s;
      vd[i * 2 + 1] = this.y[i] * s;
    }
  }
}