// Comprueba que el DSP en C++ (vst/Source/dsp/TapeDSP.h) da lo mismo que el de JavaScript (dsp/tapehead.js).
// node tests/parity.js   (compila tests/parity.cpp con g++)
'use strict';
const fs = require('fs'), os = require('os'), path = require('path'), { execFileSync } = require('child_process');
const T = require('../dsp/tapehead.js');
const SR = 48000, N = SR, x = new Float32Array(N);
let s = 3; const r = () => ((s = (s * 1664525 + 1013904223) >>> 0) / 4294967296);
for (let i = 0; i < N; i++) x[i] = 0.5 * Math.sin(2 * Math.PI * 110 * i / SR) * Math.exp(-(i % 12000) / 6000) + 0.2 * (r() * 2 - 1) * (i % 24000 < 2000 ? 1 : 0.05);
const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'tape-')), fin = path.join(dir, 'in.f32'), fout = path.join(dir, 'out.f32'), bin = path.join(dir, 'parity');
fs.writeFileSync(fin, Buffer.from(x.buffer));
execFileSync('g++', ['-O2', '-std=c++17', '-I', path.join(__dirname, '../vst/Source'), path.join(__dirname, 'parity.cpp'), '-o', bin]);
execFileSync(bin, [fin, fout]);
const c = new Float32Array(fs.readFileSync(fout).buffer.slice(0));
const sets = [[0, 0, 0], [0.35, -6, 4], [0.7, 5, -7], [1, 8, 9]];
let fail = 0;
sets.forEach(([d, b, t], k) => {
  const [y] = T.processBuffers([x], SR, { drive: d, bass: b, treble: t, mix: 1, auto: true });
  let mx = 0, pk = 0;
  for (let i = 0; i < N; i++) { mx = Math.max(mx, Math.abs(y[i] - c[k * N + i])); pk = Math.max(pk, Math.abs(y[i])); }
  const ok = mx < 1e-5; if (!ok) fail++;
  console.log(`drive ${d} graves ${b} agudos ${t}: diferencia máx ${mx.toExponential(2)} (pico ${pk.toFixed(3)}) ${ok ? 'OK' : 'FALLO'}`);
});
process.exit(fail ? 1 : 0);
