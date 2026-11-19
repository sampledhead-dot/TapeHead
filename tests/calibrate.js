// Mide el comportamiento del DSP y calcula la tabla Auto-Gain (T.AUTO_DB).
// node tests/calibrate.js            → informe
// node tests/calibrate.js --table    → solo la tabla nueva
'use strict';
const T = require('../dsp/tapehead.js');
const SR = 48000;

function rng(seed) { let s = seed >>> 0; return () => ((s = (s * 1664525 + 1013904223) >>> 0) / 4294967296); }
// Material de prueba: guitarra/bajo (sierras con caída), bombo, caja de ruido y charles, 6 s
function music(sr, seconds, rmsDb) {
  const N = Math.round(sr * seconds), x = new Float32Array(N), r = rng(7);
  const notes = [110, 146.8, 164.8, 220, 196, 130.8, 82.4, 98];
  for (let b = 0; b < seconds * 4; b++) {
    const t0 = Math.round(b * sr / 4), f = notes[b % notes.length];
    for (let i = 0; i < sr / 4 && t0 + i < N; i++) {
      const t = i / sr, ph = (f * t) % 1, env = Math.exp(-t * 5);
      x[t0 + i] += 0.5 * (2 * ph - 1) * env + 0.25 * (2 * ((2 * f * t) % 1) - 1) * env;
      if (b % 2 === 0) x[t0 + i] += 0.9 * Math.sin(2 * Math.PI * (50 + 80 * Math.exp(-t * 30)) * t) * Math.exp(-t * 12);
      if (b % 4 === 2) x[t0 + i] += 0.5 * (r() * 2 - 1) * Math.exp(-t * 25);
      x[t0 + i] += 0.08 * (r() * 2 - 1) * Math.exp(-t * 60);
    }
  }
  let s = 0; for (const v of x) s += v * v;
  const g = Math.pow(10, rmsDb / 20) / Math.sqrt(s / N);
  for (let i = 0; i < N; i++) x[i] *= g;
  return x;
}
// Ponderación K (BS.1770) a 48 kHz
function kweight(x) {
  const bq = (b, a) => { let x1 = 0, x2 = 0, y1 = 0, y2 = 0; return v => { const y = b[0] * v + b[1] * x1 + b[2] * x2 - a[1] * y1 - a[2] * y2; x2 = x1; x1 = v; y2 = y1; y1 = y; return y; }; };
  const f1 = bq([1.53512485958697, -2.69169618940638, 1.19839281085285], [1, -1.69065929318241, 0.73248077421585]);
  const f2 = bq([1, -2, 1], [1, -1.99004745483398, 0.99007225036621]);
  let s = 0; for (const v of x) { const y = f2(f1(v)); s += y * y; }
  return 10 * Math.log10(s / x.length + 1e-20);
}
function thd(drive, ampDb, f = 220) {
  const N = SR, x = new Float32Array(N), a = Math.pow(10, ampDb / 20);
  for (let i = 0; i < N; i++) x[i] = a * Math.sin(2 * Math.PI * f * i / SR);
  const [y] = T.processBuffers([x], SR, { drive, auto: false });
  const seg = y.subarray(SR / 2, SR / 2 + 24000);          // 24000 = 110 periodos de 220 Hz
  const harm = k => { let re = 0, im = 0; for (let i = 0; i < seg.length; i++) { const w = 2 * Math.PI * f * k * i / SR; re += seg[i] * Math.cos(w); im += seg[i] * Math.sin(w); } return Math.hypot(re, im); };
  const h = [1, 2, 3, 4, 5, 6, 7, 8, 9].map(harm);
  const rest = Math.sqrt(h.slice(1).reduce((s, v) => s + v * v, 0));
  return { thd: 100 * rest / h[0], h2: 20 * Math.log10(h[1] / h[0] + 1e-12), h3: 20 * Math.log10(h[2] / h[0] + 1e-12) };
}
function response(drive, f) {
  const N = SR / 2, x = new Float32Array(N), a = Math.pow(10, -40 / 20);
  for (let i = 0; i < N; i++) x[i] = a * Math.sin(2 * Math.PI * f * i / SR);
  const [y] = T.processBuffers([x], SR, { drive, auto: false });
  let s = 0, n = 0; for (let i = N / 2; i < N; i++) { s += y[i] * y[i]; n++; }
  return 20 * Math.log10(Math.sqrt(2 * s / n) / a) - 20 * Math.log10(T.map(drive, 0, 0, SR).g1 * T.map(drive, 0, 0, SR).g2);
}

const x = music(SR, 6, -14), kin = kweight(x);
const table = [];
for (let i = 0; i <= 20; i++) {
  const d = i / 20, [y] = T.processBuffers([x], SR, { drive: d, auto: false });
  table.push(kweight(y) - kin);
}
const auto = table.map(v => +v.toFixed(2));   // relativo a la señal original: con Auto-Gain el nivel percibido = el de entrada
if (process.argv.includes('--table')) { console.log(JSON.stringify(auto.map(v => +v.toFixed(2)))); process.exit(0); }

console.log('Ganancia percibida sin Auto-Gain (dB, ponderación K) por drive 0…1:');
console.log(table.map((v, i) => (i / 2).toFixed(1) + ':' + v.toFixed(1)).join('  '));
console.log('\nTabla AUTO_DB propuesta:', JSON.stringify(auto));
let worst = 0;
for (let i = 0; i <= 20; i++) {
  const d = i / 20, [y] = T.processBuffers([x], SR, { drive: d, auto: true });
  const err = kweight(y) - kin; worst = Math.max(worst, Math.abs(err));
}
console.log('Error máximo de nivel con Auto-Gain (tabla actual):', worst.toFixed(2), 'dB');
console.log('\nTHD con seno de 220 Hz (sin Auto-Gain):');
for (const d of [0, 0.15, 0.3, 0.5, 0.7, 0.85, 1]) for (const amp of [-18, -6]) {
  const r = thd(d, amp); console.log(`  drive ${(d * 10).toFixed(1)}  ${amp} dBFS  THD ${r.thd.toFixed(2)} %  H2 ${r.h2.toFixed(1)} dB  H3 ${r.h3.toFixed(1)} dB`);
}
console.log('\nRespuesta en frecuencia a −40 dBFS (relativa a la ganancia de drive):');
for (const d of [0, 0.5, 1]) console.log('  drive', d * 10, [40, 100, 300, 1000, 3000, 8000, 15000].map(f => f + 'Hz ' + response(d, f).toFixed(1)).join('  '));
