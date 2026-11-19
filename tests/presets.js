// Nivel percibido (ponderación K) y pico de cada preset de fábrica con material de su categoría.
// node tests/presets.js   → falla si algún preset se sale de ±4 dB respecto a la entrada.
'use strict';
const T = require('../dsp/tapehead.js'); const SR = 48000;
function rng(seed) { let s = seed >>> 0; return () => ((s = (s * 1664525 + 1013904223) >>> 0) / 4294967296); }
function norm(x, db) { let s = 0; for (const v of x) s += v * v; const g = Math.pow(10, db / 20) / Math.sqrt(s / x.length); return x.map(v => v * g); }
function guitar() { const N = SR * 4, x = new Float32Array(N), f = [196, 147, 220, 165]; for (let b = 0; b < 16; b++) for (let i = 0; i < SR / 4; i++) { const t = i / SR, e = Math.exp(-t * 3), k = b * SR / 4 + i; for (const m of [1, 1.5, 2]) x[k] += 0.3 * (2 * ((f[b % 4] * m * t) % 1) - 1) * e; } return norm(x, -16); }
function bass() { const N = SR * 4, x = new Float32Array(N), f = [55, 73.4, 41.2, 61.7]; for (let b = 0; b < 16; b++) for (let i = 0; i < SR / 4; i++) { const t = i / SR, e = Math.exp(-t * 4), k = b * SR / 4 + i, ph = (f[b % 4] * t) % 1; x[k] = (Math.sin(2 * Math.PI * ph) + 0.3 * (2 * ph - 1)) * e; } return norm(x, -16); }
function drums() { const N = SR * 4, x = new Float32Array(N), r = rng(9); for (let b = 0; b < 32; b++) for (let i = 0; i < SR / 8; i++) { const t = i / SR, k = b * SR / 8 + i; if (b % 4 === 0) x[k] += Math.sin(2 * Math.PI * (50 + 90 * Math.exp(-t * 35)) * t) * Math.exp(-t * 10); if (b % 4 === 2) x[k] += 0.6 * (r() * 2 - 1) * Math.exp(-t * 18) + 0.4 * Math.sin(2 * Math.PI * 190 * t) * Math.exp(-t * 25); x[k] += 0.15 * (r() * 2 - 1) * Math.exp(-t * 70); } return norm(x, -16); }
function kw(x) { const bq = (b, a) => { let x1 = 0, x2 = 0, y1 = 0, y2 = 0; return v => { const y = b[0] * v + b[1] * x1 + b[2] * x2 - a[1] * y1 - a[2] * y2; x2 = x1; x1 = v; y2 = y1; y1 = y; return y; }; };
  const f1 = bq([1.53512485958697, -2.69169618940638, 1.19839281085285], [1, -1.69065929318241, 0.73248077421585]), f2 = bq([1, -2, 1], [1, -1.99004745483398, 0.99007225036621]);
  let s = 0; for (const v of x) { const y = f2(f1(v)); s += y * y; } return 10 * Math.log10(s / x.length + 1e-20); }
const src = { general: guitar(), guitarra: guitar(), bajo: bass(), bateria: drums() };
let bad = 0;
for (const p of T.PRESETS) {
  const x = src[p.cat], [y] = T.processBuffers([x], SR, Object.assign({}, p.p, { auto: true }));
  let pk = 0; for (const v of y) pk = Math.max(pk, Math.abs(v));
  const d = kw(y) - kw(x), ok = Math.abs(d) <= 4 && pk < 1; if (!ok) bad++;
  console.log(`${p.cat.padEnd(9)} ${p.name.padEnd(30)} ${d >= 0 ? '+' : ''}${d.toFixed(1)} dB  pico ${(20 * Math.log10(pk)).toFixed(1)} dBFS ${ok ? '' : '  ← revisar'}`);
}
process.exit(bad ? 1 : 0);
