const WIDTH = 1280;
const HEIGHT = 720;
const DURATION = 30.0;
const STACK_NAME = "html-webgl";

let gl, canvas;
let nbody, squares, cubes;
let vboCubesVerts, vboCubesColors, vboCubesMVP, eboCubes;
let sceneIdx = 0;
let paused = false;
let hudShow = true;

let frameTimes = new Float64Array(240);
let frameCount = 0;
let frameHead = 0;
let maxFps = 0;
let startupMs = 0;
let firstFrame = true;
let lastTime = 0;

let measuring = false;
let measureElapsed = 0;
let measureFrames = [];
let measureJustFinished = false;

let progNBody, progSquares, progCubes;
let vboNBody, vboSquares;

const VS_NBODY = `
attribute vec2 aPos;
uniform float uAspect;
void main() {
  vec2 p = aPos;
  p.x /= uAspect;
  gl_Position = vec4(p, 0.0, 1.0);
  gl_PointSize = 3.0;
}
`;
const FS_NBODY = `
precision mediump float;
void main() {
  gl_FragColor = vec4(0.7, 0.85, 1.0, 1.0);
}
`;

const VS_SQUARES = `
attribute vec2 aPos;
attribute vec3 aColor;
uniform vec2 uRes;
varying vec3 vColor;
void main() {
  vec2 ndc = (aPos / uRes) * 2.0 - 1.0;
  ndc.y = -ndc.y;
  gl_Position = vec4(ndc, 0.0, 1.0);
  vColor = aColor;
}
`;
const FS_SQUARES = `
precision mediump float;
varying vec3 vColor;
void main() {
  gl_FragColor = vec4(vColor, 1.0);
}
`;

const VS_CUBES = `
attribute vec3 aPos;
attribute vec3 aColor;
attribute mat4 aMVP;
varying vec3 vColor;
void main() {
  gl_Position = aMVP * vec4(aPos, 1.0);
  vColor = aColor;
}
`;
const FS_CUBES = `
precision mediump float;
varying vec3 vColor;
void main() {
  gl_FragColor = vec4(vColor, 1.0);
}
`;

async function main() {
  canvas = document.getElementById("glcanvas");

  const ctxOpts = {
    desynchronized: true,
    antialias: false,
    powerPreference: "high-performance",
  };
  gl = canvas.getContext("webgl2", ctxOpts) || canvas.getContext("webgl", ctxOpts);
  if (!gl) { alert("WebGL not supported"); return; }

  gl.clearColor(0.08, 0.08, 0.11, 1.0);

  const [particles, sq, cb] = await Promise.all([
    loadCsv("data/particles.csv"),
    loadCsv("data/squares.csv"),
    loadCsv("data/cubes.csv"),
  ]);

  nbody = new NBody(particles, 1000);
  squares = new Squares2D(sq, 1000);
  cubes = new Cubes3D(cb, 200);

  progNBody = createProgram(gl, VS_NBODY, FS_NBODY);
  progSquares = createProgram(gl, VS_SQUARES, FS_SQUARES);
  progCubes = createProgram(gl, VS_CUBES, FS_CUBES, {
    aPos: 0,
    aColor: 1,
    aMVP: 2,
  });

  vboNBody = gl.createBuffer();
  vboSquares = gl.createBuffer();

  vboCubesVerts = gl.createBuffer();
  vboCubesColors = gl.createBuffer();
  vboCubesMVP = gl.createBuffer();
  eboCubes = gl.createBuffer();

  window.addEventListener("keydown", handleKey);

  lastTime = performance.now();
  scheduleNext();
}

const channel = new MessageChannel();
channel.port1.onmessage = () => loop(performance.now());

function scheduleNext() {
  channel.port2.postMessage(null);
}

function handleKey(e) {
  switch (e.code) {
    case "Digit1": sceneIdx = 0; resetMax(); break;
    case "Digit2": sceneIdx = 1; resetMax(); break;
    case "Digit3": sceneIdx = 2; resetMax(); break;
    case "Space": paused = !paused; break;
    case "KeyR":
      if (sceneIdx === 0) nbody.reset();
      else if (sceneIdx === 1) squares.reset();
      else cubes.reset();
      resetMax();
      break;
    case "KeyI": hudShow = !hudShow; break;
    case "KeyS":
      if (!measuring) {
        measuring = true;
        measureElapsed = 0;
        measureFrames = [];
        measureJustFinished = false;
        resetMax();
      }
      break;
    case "Escape":
      if (measureFrames.length > 0) writeResult();
      break;
  }
}

function resetMax() {
  maxFps = 0;
  frameHead = 0;
  frameCount = 0;
}

function loop(now) {
  const dt = (now - lastTime) / 1000;
  lastTime = now;
  const dtMs = dt * 1000;

  frameTimes[frameHead] = dtMs;
  frameHead = (frameHead + 1) % 240;
  if (frameCount < 240) frameCount++;
  const instFps = 1000 / dtMs;
  if (instFps > maxFps) maxFps = instFps;

  if (measuring) {
    measureFrames.push(dt);
    measureElapsed += dt;
    if (measureElapsed >= DURATION) {
      measuring = false;
      measureJustFinished = true;
    }
  }
  if (measureJustFinished) {
    measureJustFinished = false;
    writeResult();
  }

  if (firstFrame) { startupMs = now; firstFrame = false; }

  if (!paused) {
    if (sceneIdx === 0) nbody.update();
    else if (sceneIdx === 1) squares.update();
    else cubes.update(dt);
  }

  render();
  updateHud();

  scheduleNext();
}

function render() {
  gl.clearColor(0.08, 0.08, 0.11, 1.0);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  gl.enable(gl.DEPTH_TEST);
  
  for (let i = 0; i < 8; i++) {
    gl.vertexAttribDivisor(i, 0);
  }

  if (sceneIdx === 0) renderNBody();
  else if (sceneIdx === 1) renderSquares();
  else renderCubes();
}

function renderNBody() {
  nbody.uploadVertices();
  const data = nbody.vertexData;

  gl.useProgram(progNBody);
  gl.bindBuffer(gl.ARRAY_BUFFER, vboNBody);
  gl.bufferData(gl.ARRAY_BUFFER, data, gl.DYNAMIC_DRAW);

  const loc = gl.getAttribLocation(progNBody, "aPos");
  gl.enableVertexAttribArray(loc);
  gl.vertexAttribPointer(loc, 2, gl.FLOAT, false, 0, 0);

  const uAspect = gl.getUniformLocation(progNBody, "uAspect");
  gl.uniform1f(uAspect, WIDTH / HEIGHT);

  gl.drawArrays(gl.POINTS, 0, nbody.n);
}

function renderSquares() {
  squares.uploadVertices();
  const data = squares.vertexData;

  gl.useProgram(progSquares);
  gl.bindBuffer(gl.ARRAY_BUFFER, vboSquares);
  gl.bufferData(gl.ARRAY_BUFFER,
                data.subarray(0, squares.vertexCount * 5),
                gl.DYNAMIC_DRAW);

  const stride = 5 * 4;
  const locPos = gl.getAttribLocation(progSquares, "aPos");
  const locCol = gl.getAttribLocation(progSquares, "aColor");
  gl.enableVertexAttribArray(locPos);
  gl.vertexAttribPointer(locPos, 2, gl.FLOAT, false, stride, 0);
  gl.enableVertexAttribArray(locCol);
  gl.vertexAttribPointer(locCol, 3, gl.FLOAT, false, stride, 2 * 4);

  const uRes = gl.getUniformLocation(progSquares, "uRes");
  gl.uniform2f(uRes, WIDTH, HEIGHT);

  gl.drawArrays(gl.TRIANGLES, 0, squares.vertexCount);
}

function renderCubes() {
  cubes.uploadVertices();

  gl.useProgram(progCubes);

  gl.bindBuffer(gl.ARRAY_BUFFER, vboCubesVerts);
  gl.bufferData(gl.ARRAY_BUFFER, cubes.vertexData, gl.DYNAMIC_DRAW);
  gl.enableVertexAttribArray(0);
  gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 0, 0);
  gl.vertexAttribDivisor(0, 0);

  gl.bindBuffer(gl.ARRAY_BUFFER, vboCubesColors);
  gl.bufferData(gl.ARRAY_BUFFER, cubes.colorData, gl.DYNAMIC_DRAW);
  gl.enableVertexAttribArray(1);
  gl.vertexAttribPointer(1, 3, gl.FLOAT, false, 0, 0);
  gl.vertexAttribDivisor(1, 1);

  gl.bindBuffer(gl.ARRAY_BUFFER, vboCubesMVP);
  gl.bufferData(gl.ARRAY_BUFFER, cubes.mvpData, gl.DYNAMIC_DRAW);
  const stride = 16 * 4;
  for (let k = 0; k < 4; k++) {
    const loc = 2 + k;
    gl.enableVertexAttribArray(loc);
    gl.vertexAttribPointer(loc, 4, gl.FLOAT, false, stride, k * 16);
    gl.vertexAttribDivisor(loc, 1);
  }

  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, eboCubes);
  gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, cubes.edges, gl.STATIC_DRAW);

  if (gl.drawElementsInstanced) {
    gl.drawElementsInstanced(gl.LINES, cubes.indexCount, gl.UNSIGNED_SHORT, 0, cubes.n);
  } else {
    const ext = gl.getExtension("ANGLE_instanced_arrays");
    ext.drawElementsInstancedANGLE(gl.LINES, cubes.indexCount, gl.UNSIGNED_SHORT, 0, cubes.n);
  }
}

function stats() {
  if (frameCount === 0) return [0, 0, 0, maxFps];
  const arr = Array.from(frameTimes.slice(0, frameCount)).sort((a, b) => a - b);
  const avg = arr.reduce((a, b) => a + b, 0) / arr.length;
  const p99 = arr[Math.min(arr.length - 1, Math.floor(arr.length * 0.99))];
  const k = Math.max(1, Math.floor(arr.length * 0.01));
  const slowSum = arr.slice(-k).reduce((a, b) => a + b, 0);
  const low1 = 1000 / (slowSum / k);
  return [1000 / avg, p99, low1, maxFps];
}

function updateHud() {
  const hud = document.getElementById("hud");
  if (!hudShow) { hud.textContent = ""; return; }
  const s = stats();
  const test = ["T1 - N-Body", "T2 - 2D Squares", "T3 - 3D Cubes"][sceneIdx];
  const objects = [nbody.n, squares.n, cubes.n][sceneIdx];
  const lines = [
    `Stack:    ${STACK_NAME}`,
    `Test:     ${test}`,
    `FPS:      ${s[0].toFixed(1)}`,
    `Frame:    ${(1000 / s[0]).toFixed(2)} ms (avg) / ${s[1].toFixed(2)} ms (p99)`,
    `Max FPS:  ${s[3].toFixed(1)}`,
    `1% Low:   ${s[2].toFixed(1)}`,
    `Objects:  ${objects}`,
    `Paused:   ${paused}`,
    measuring ? `Measuring: ${Math.ceil(DURATION - measureElapsed)}s` : "Measuring: no",
    "[1] T1  [2] T2  [3] T3  [Space] Pause  [R] Reset",
    "[I] Info  [S] Measure  [Esc] Quit",
  ];
  hud.textContent = lines.join("\n");
}

function writeResult() {
  const s = stats();
  const test = ["T1", "T2", "T3"][sceneIdx];
  const objects = [1000, 1000, 200][sceneIdx];
  const maxMs = Math.max(...Array.from(frameTimes.slice(0, frameCount)));
  const json = {
    stack: STACK_NAME,
    test: test,
    objects: objects,
    resolution: [WIDTH, HEIGHT],
    avg_fps: s[0],
    min_fps: 1000 / maxMs,
    max_fps: s[3],
    avg_frame_ms: 1000 / s[0],
    p99_frame_ms: s[1],
    one_percent_low_fps: s[2],
    startup_ms: startupMs,
    duration_s: DURATION,
    runtime: "HTML5 + WebGL",
    os: navigator.platform,
  };
  const blob = new Blob([JSON.stringify(json, null, 2)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = `${STACK_NAME}-${test}-${objects}.json`;
  a.click();
  URL.revokeObjectURL(url);
  console.log("Wrote", a.download);
}

main();