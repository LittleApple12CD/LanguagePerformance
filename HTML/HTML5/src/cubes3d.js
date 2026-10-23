class Cubes3D {
  constructor(data, n) {
    this.n = n;
    this.all = data;
    this.reset();
  }

  reset() {
    const n = this.n;
    this.px = new Float32Array(n);
    this.py = new Float32Array(n);
    this.pz = new Float32Array(n);
    this.ax = new Float32Array(n);
    this.ay = new Float32Array(n);
    this.az = new Float32Array(n);
    this.speed = new Float32Array(n);
    this.colors = new Float32Array(n * 3);
    this.angles = new Float32Array(n);

    for (let i = 0; i < n; i++) {
      this.px[i] = this.all[i][0];
      this.py[i] = this.all[i][1];
      this.pz[i] = this.all[i][2];
      this.ax[i] = this.all[i][3];
      this.ay[i] = this.all[i][4];
      this.az[i] = this.all[i][5];
      this.speed[i] = this.all[i][6];
      this.colors[i * 3]     = this.all[i][7] / 255;
      this.colors[i * 3 + 1] = this.all[i][8] / 255;
      this.colors[i * 3 + 2] = this.all[i][9] / 255;
    }

    const s = 0.25;
    this.localVerts = new Float32Array([
      -s, -s, -s,
       s, -s, -s,
       s,  s, -s,
      -s,  s, -s,
      -s, -s,  s,
       s, -s,  s,
       s,  s,  s,
      -s,  s,  s,
    ]);

    this.edges = new Uint16Array([
      0,1, 1,2, 2,3, 3,0,
      4,5, 5,6, 6,7, 7,4,
      0,4, 1,5, 2,6, 3,7,
    ]);

    this.vertexData = new Float32Array(8 * 3);
    this.colorData = new Float32Array(n * 3);
    this.mvpData = new Float32Array(n * 16);
  }

  update(dt) {
    for (let i = 0; i < this.n; i++) {
      this.angles[i] += this.speed[i] * dt;
    }
  }

  uploadVertices() {
    const n = this.n;
    const vd = this.vertexData;
    const cd = this.colorData;
    const md = this.mvpData;
    const lv = this.localVerts;

    for (let k = 0; k < 8; k++) {
      vd[k * 3]     = lv[k * 3];
      vd[k * 3 + 1] = lv[k * 3 + 1];
      vd[k * 3 + 2] = lv[k * 3 + 2];
    }

    const fov = 60 * Math.PI / 180;
    const f = 1.0 / Math.tan(fov / 2);
    const aspect = 1280 / 720;
    const near = 0.1, far = 100.0;
    const camZ = -8.0;

    for (let i = 0; i < n; i++) {
      const a = this.angles[i];
      const axx = this.ax[i], ayy = this.ay[i], azz = this.az[i];
      const c = Math.cos(a), sn = Math.sin(a);

      const r00 = c + axx*axx*(1-c);
      const r01 = axx*ayy*(1-c) - azz*sn;
      const r02 = axx*azz*(1-c) + ayy*sn;
      const r10 = ayy*axx*(1-c) + azz*sn;
      const r11 = c + ayy*ayy*(1-c);
      const r12 = ayy*azz*(1-c) - axx*sn;
      const r20 = azz*axx*(1-c) - ayy*sn;
      const r21 = azz*ayy*(1-c) + axx*sn;
      const r22 = c + azz*azz*(1-c);

      const tx = this.px[i], ty = this.py[i], tz = this.pz[i];

      const model = new Float32Array(16);
      model[0] = r00; model[1] = r10; model[2] = r20; model[3] = 0;
      model[4] = r01; model[5] = r11; model[6] = r21; model[7] = 0;
      model[8] = r02; model[9] = r12; model[10] = r22; model[11] = 0;
      model[12] = tx; model[13] = ty; model[14] = tz; model[15] = 1;

      const view = new Float32Array(16);
      view[0] = 1; view[1] = 0; view[2] = 0; view[3] = 0;
      view[4] = 0; view[5] = 1; view[6] = 0; view[7] = 0;
      view[8] = 0; view[9] = 0; view[10] = 1; view[11] = 0;
      view[12] = 0; view[13] = 0; view[14] = camZ; view[15] = 1;

      const proj = new Float32Array(16);
      proj[0] = f / aspect; proj[1] = 0; proj[2] = 0; proj[3] = 0;
      proj[4] = 0; proj[5] = f; proj[6] = 0; proj[7] = 0;
      proj[8] = 0; proj[9] = 0; proj[10] = (far + near) / (near - far); proj[11] = -1;
      proj[12] = 0; proj[13] = 0; proj[14] = (2 * far * near) / (near - far); proj[15] = 0;

      const vm = mulMat4(view, model);
      const mvp = mulMat4(proj, vm);

      for (let k = 0; k < 16; k++) {
        md[i * 16 + k] = mvp[k];
      }

      const cb = i * 3;
      cd[i * 3]     = this.colors[cb];
      cd[i * 3 + 1] = this.colors[cb + 1];
      cd[i * 3 + 2] = this.colors[cb + 2];
    }

    this.vertexCount = 8;
    this.indexCount = 24;
  }
}

function mulMat4(a, b) {
  const out = new Float32Array(16);
  for (let r = 0; r < 4; r++) {
    for (let c = 0; c < 4; c++) {
      let s = 0;
      for (let k = 0; k < 4; k++) {
        s += a[k * 4 + r] * b[c * 4 + k];
      }
      out[c * 4 + r] = s;
    }
  }
  return out;
}