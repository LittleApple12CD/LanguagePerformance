class Squares2D {
  constructor(data, n) {
    this.n = n;
    this.all = data;
    this.reset();
  }

  reset() {
    const n = this.n;
    const x0 = 640 - 200, y0 = 360 - 200;

    this.x = new Float32Array(n);
    this.y = new Float32Array(n);
    this.vx = new Float32Array(n);
    this.vy = new Float32Array(n);
    this.size = new Float32Array(n);
    this.colors = new Float32Array(n * 3);

    for (let i = 0; i < n; i++) {
      this.x[i] = this.all[i][0] + x0;
      this.y[i] = this.all[i][1] + y0;
      this.vx[i] = this.all[i][2];
      this.vy[i] = this.all[i][3];
      this.size[i] = this.all[i][4];
      this.colors[i * 3]     = this.all[i][5] / 255;
      this.colors[i * 3 + 1] = this.all[i][6] / 255;
      this.colors[i * 3 + 2] = this.all[i][7] / 255;
    }

    this.x0 = x0; this.x1 = 640 + 200;
    this.y0 = y0; this.y1 = 360 + 200;

    this.vertexData = new Float32Array(n * 6 * 5);
    this.vertexCount = 0;
  }

  update() {
    const n = this.n;
    for (let i = 0; i < n; i++) {
      this.x[i] += this.vx[i];
      this.y[i] += this.vy[i];

      if (this.x[i] < this.x0) {
        this.x[i] = this.x0;
        this.vx[i] = -this.vx[i];
      } else if (this.x[i] > this.x1) {
        this.x[i] = this.x1;
        this.vx[i] = -this.vx[i];
      }

      if (this.y[i] < this.y0) {
        this.y[i] = this.y0;
        this.vy[i] = -this.vy[i];
      } else if (this.y[i] > this.y1) {
        this.y[i] = this.y1;
        this.vy[i] = -this.vy[i];
      }
    }
  }

  uploadVertices() {
    const n = this.n;
    const d = this.vertexData;
    let idx = 0;

    for (let i = 0; i < n; i++) {
      const x = this.x[i];
      const y = this.y[i];
      const s = this.size[i];
      const r = this.colors[i * 3];
      const g = this.colors[i * 3 + 1];
      const b = this.colors[i * 3 + 2];

      const corners = [
        [x,     y    ],
        [x + s, y    ],
        [x,     y + s],
        [x,     y + s],
        [x + s, y    ],
        [x + s, y + s],
      ];

      for (let k = 0; k < 6; k++) {
        d[idx++] = corners[k][0];
        d[idx++] = corners[k][1];
        d[idx++] = r;
        d[idx++] = g;
        d[idx++] = b;
      }
    }

    this.vertexCount = idx / 5;
  }
}