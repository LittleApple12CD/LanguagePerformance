function createShader(gl, type, source) {
  const shader = gl.createShader(type);
  gl.shaderSource(shader, source);
  gl.compileShader(shader);
  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
    console.error(gl.getShaderInfoLog(shader));
    return null;
  }
  return shader;
}

function createProgram(gl, vsSource, fsSource, attribs) {
  const vs = createShader(gl, gl.VERTEX_SHADER, vsSource);
  const fs = createShader(gl, gl.FRAGMENT_SHADER, fsSource);
  const prog = gl.createProgram();
  gl.attachShader(prog, vs);
  gl.attachShader(prog, fs);
  if (attribs) {
    for (const [name, loc] of Object.entries(attribs)) {
      gl.bindAttribLocation(prog, loc, name);
    }
  }
  gl.linkProgram(prog);
  if (!gl.getProgramParameter(prog, gl.LINK_STATUS)) {
    console.error(gl.getProgramInfoLog(prog));
    return null;
  }
  return prog;
}

function createBuffer(gl, data, target = gl.ARRAY_BUFFER, usage = gl.DYNAMIC_DRAW) {
  const buf = gl.createBuffer();
  gl.bindBuffer(target, buf);
  gl.bufferData(target, data, usage);
  return buf;
}