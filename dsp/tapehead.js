/* TapeHead · motor de sonido (JavaScript)
   Previo de consola de multipista de casete de los 90: amplificadores operacionales en cascada
   (recorte de entrada → amplificador del fader → bus de mezcla), realce de medios y compensación de nivel.
   Sin artefactos de cinta (ni wow & flutter, ni ruido, ni pérdida de cabezales): solo la electrónica.

   Es la REFERENCIA del DSP: el plugin (vst/Source/dsp/TapeDSP.h) hace exactamente lo mismo con los mismos números.
   Si cambias algo aquí, cámbialo allí igual (y en la copia de Tono-Pad). tests/parity.js lo comprueba.

   Uso offline (Tono-Pad):  var out = TapeHead.processBuffers([L, R], sr, params)  → [L2, R2] (misma longitud, sin latencia)
   Parámetros: { drive 0–1, bass ±10 dB, treble ±10 dB, mix 0–1, inDb, outDb, auto } */
function tpTapeHead(root) {
  'use strict';
  var T = root.TapeHead = root.TapeHead || {};
  T.VERSION = '1.0.0';
  T.OS = 4;                       // sobremuestreo ×4 (recorte duro sin aliasing grueso)
  T.TAPS = 129;                   // FIR de fase lineal del sobremuestreo (32 + 1 por fase)
  T.LATENCY = 32;                 // muestras (64 + 64 a ×4 = 128 / 4)
  T.DEFAULTS = { drive: 0.35, bass: 0, treble: 0, mix: 1, inDb: 0, outDb: 0, auto: true };

  function db2lin(d) { return Math.pow(10, d / 20); }
  function clamp(x, a, b) { return x < a ? a : (x > b ? b : x); }
  T.db2lin = db2lin;

  // ------------------------------------------------------------ mapa del mando DRIVE (0–1, se muestra 0–10)
  T.map = function (d, bass, treble, sr) {
    d = clamp(d, 0, 1); bass = clamp(bass || 0, -10, 10); treble = clamp(treble || 0, -10, 10);
    var push = d > 0.4 ? (d - 0.4) / 0.6 : 0;
    return {
      g1: db2lin(40 * Math.pow(d, 1.3)),          // TRIM: hasta +40 dB contra los raíles del primer op-amp
      g2: db2lin(16 * Math.pow(push, 1.5)),       // el fader empuja la 2.ª etapa a partir de ~4
      bias2: 0.08 * d,                            // asimetría creciente (armónicos pares)
      lsPre: -(1 + 5 * d),                        // graves recortados antes de saturar (fuzz más apretado)
      pkPre: 1.5 + 4.5 * d,                       // medios hacia delante (1 kHz)
      lsPost: 0.5 + 2.5 * d,                      // parte de los graves se devuelve después
      lpf: Math.min(16000 - 8000 * d, 0.45 * sr), // ancho de banda de una consola barata; se cierra al saturar
      bass: bass,                                 // EQ de canal (como la consola: después del previo, antes del fader)
      treble: treble                              //   graves: estante a 120 Hz · agudos: estante a 5 kHz (ancho, tipo Baxandall)
    };
  };

  // Compensación de nivel (Auto-Gain), medida con tests/calibrate.js sobre material musical a −14 dBFS RMS.
  // 21 puntos para drive = 0, 0.05 … 1 (dB que se restan; relativos a la señal original, así A/B con el bypass
  // queda al mismo nivel). Mismos valores en TapeDSP.h.
  T.AUTO_DB = [1.95, 2.75, 3.91, 5.25, 6.70, 8.20, 9.71, 11.15, 12.49, 13.98, 15.46, 16.79, 17.92, 18.77, 19.33, 19.67, 19.89, 20.02, 20.09, 20.11, 20.11];
  T.autoGain = function (d) {
    d = clamp(d, 0, 1) * 20; var i = Math.min(19, Math.floor(d)), f = d - i;
    return db2lin(-(T.AUTO_DB[i] * (1 - f) + T.AUTO_DB[i + 1] * f));
  };

  // ------------------------------------------------------------ FIR del sobremuestreo (sinc con ventana Kaiser)
  function bessel0(x) { var s = 1, t = 1, k; for (k = 1; k < 40; k++) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; } return s; }
  T.makeFir = function () {
    var N = T.TAPS, M = N - 1, fc = 0.5 / T.OS * 0.96, beta = 8, h = new Float64Array(N), s = 0, k;
    for (k = 0; k < N; k++) {
      var n = k - M / 2, sinc = n === 0 ? 2 * fc : Math.sin(2 * Math.PI * fc * n) / (Math.PI * n);
      var r = 2 * k / M - 1, w = bessel0(beta * Math.sqrt(Math.max(0, 1 - r * r))) / bessel0(beta);
      h[k] = sinc * w; s += h[k];
    }
    for (k = 0; k < N; k++) h[k] /= s;
    return h;
  };
  var FIR = T.makeFir(), TAPS = T.TAPS, XL = 40, RL = TAPS + 4, PH = [];
  (function () { for (var p = 0; p < T.OS; p++) { var a = []; for (var j = 0; p + T.OS * j < TAPS; j++) a.push(FIR[p + T.OS * j]); PH.push(new Float64Array(a)); } })();

  // ------------------------------------------------------------ filtros (RBJ, forma directa I en doble precisión)
  function Biquad() { this.b0 = 1; this.b1 = 0; this.b2 = 0; this.a1 = 0; this.a2 = 0; this.x1 = 0; this.x2 = 0; this.y1 = 0; this.y2 = 0; }
  Biquad.prototype.set = function (type, f, q, gainDb, fs) {
    var A = Math.pow(10, gainDb / 40), w = 2 * Math.PI * f / fs, cw = Math.cos(w), sw = Math.sin(w), al = sw / (2 * q), b0, b1, b2, a0, a1, a2, sA;
    if (type === 'peak') { b0 = 1 + al * A; b1 = -2 * cw; b2 = 1 - al * A; a0 = 1 + al / A; a1 = -2 * cw; a2 = 1 - al / A; }
    else if (type === 'lowshelf') { sA = 2 * Math.sqrt(A) * al;
      b0 = A * ((A + 1) - (A - 1) * cw + sA); b1 = 2 * A * ((A - 1) - (A + 1) * cw); b2 = A * ((A + 1) - (A - 1) * cw - sA);
      a0 = (A + 1) + (A - 1) * cw + sA; a1 = -2 * ((A - 1) + (A + 1) * cw); a2 = (A + 1) + (A - 1) * cw - sA; }
    else if (type === 'highshelf') { sA = 2 * Math.sqrt(A) * al;
      b0 = A * ((A + 1) + (A - 1) * cw + sA); b1 = -2 * A * ((A - 1) + (A + 1) * cw); b2 = A * ((A + 1) + (A - 1) * cw - sA);
      a0 = (A + 1) - (A - 1) * cw + sA; a1 = 2 * ((A - 1) - (A + 1) * cw); a2 = (A + 1) - (A - 1) * cw - sA; }
    else { b0 = (1 - cw) / 2; b1 = 1 - cw; b2 = (1 - cw) / 2; a0 = 1 + al; a1 = -2 * cw; a2 = 1 - al; }   // lowpass
    this.b0 = b0 / a0; this.b1 = b1 / a0; this.b2 = b2 / a0; this.a1 = a1 / a0; this.a2 = a2 / a0;
  };
  Biquad.prototype.run = function (x) {
    var y = this.b0 * x + this.b1 * this.x1 + this.b2 * this.x2 - this.a1 * this.y1 - this.a2 * this.y2;
    this.x2 = this.x1; this.x1 = x; this.y2 = this.y1; this.y1 = y; return y;
  };
  function OnePoleHP(f, fs) { this.a = Math.exp(-2 * Math.PI * f / fs); this.x1 = 0; this.y1 = 0; }
  OnePoleHP.prototype.run = function (x) { var y = this.a * (this.y1 + x - this.x1); this.x1 = x; this.y1 = y; return y; };
  function OnePoleLP(f, fs) { this.a = Math.exp(-2 * Math.PI * f / fs); this.y1 = 0; }
  OnePoleLP.prototype.run = function (x) { this.y1 = x + this.a * (this.y1 - x); return this.y1; };

  // ------------------------------------------------------------ recorte del op-amp
  // Rodilla cuadrática (recorte duro redondeado, raíl = 1) y después una pizca de curva cúbica (grosor a nivel bajo).
  function knee(v, w) {
    var a = v < 0 ? -v : v, y;
    if (a <= 1 - w) y = a; else if (a >= 1 + w) y = 1; else { var t = a - (1 - w); y = a - t * t / (4 * w); }
    return v < 0 ? -y : y;
  }
  function opamp(v, w, c) { var y = knee(v, w); return (y - c * y * y * y) / (1 - c); }
  T.opamp = opamp;
  var W1 = 0.35, C1 = 0.12, B1 = 0.05;     // TRIM (entrada)
  var W2 = 0.15, C2 = 0.05;                // fader (más duro: el «fuzz aplastado»)
  var W3 = 0.40, C3 = 0.08, R3 = 1.2;      // bus de mezcla
  var HEAD = 0.5;                          // margen interno: 0 dBFS queda a −6 dB de los raíles

  // ------------------------------------------------------------ un canal
  function Channel(sr) {
    var fs = sr * T.OS;
    this.sr = sr; this.fs = fs;
    // historias en espejo (cada muestra se escribe dos veces) para leerlas seguidas sin índices circulares
    this.xh = new Float64Array(2 * XL); this.xw = 0;       // entrada (base) para el FIR polifásico
    this.ring = new Float64Array(2 * RL); this.rw = 0;     // muestras a ×4 para el diezmado
    this.dly = new Float64Array(T.LATENCY + 1); this.di = 0;
    this.hpIn = new OnePoleHP(20, fs); this.hpOut = new OnePoleHP(10, fs);
    this.lsPre = new Biquad(); this.pkPre = new Biquad(); this.lp1 = new OnePoleLP(20000, fs);
    this.eqLo = new Biquad(); this.eqHi = new Biquad(); this.lsPost = new Biquad(); this.lpf = new Biquad();
    this.s1off = opamp(B1, W1, C1);
  }
  Channel.prototype.setParams = function (m) {
    var fs = this.fs;
    this.m = m;
    this.lsPre.set('lowshelf', 180, 0.707, m.lsPre, fs);
    this.pkPre.set('peak', 1000, 0.8, m.pkPre, fs);
    this.lsPost.set('lowshelf', 180, 0.707, m.lsPost, fs);
    this.lpf.set('lowpass', m.lpf, 0.707, 0, fs);
    this.eqLo.set('lowshelf', 120, 0.5, m.bass, fs);
    this.eqHi.set('highshelf', 5000, 0.5, m.treble, fs);
    this.s2off = opamp(m.bias2, W2, C2);
  };
  // una muestra a ×4 por toda la cadena analógica
  Channel.prototype.chain = function (x) {
    var m = this.m;
    x = this.hpIn.run(x);
    x = this.pkPre.run(this.lsPre.run(x));
    x = opamp(m.g1 * x + B1, W1, C1) - this.s1off;          // 1) TRIM → op-amp de entrada
    x = this.lp1.run(x);
    x = this.eqHi.run(this.eqLo.run(x));                   // EQ de canal
    x = opamp(m.g2 * x + m.bias2, W2, C2) - this.s2off;     // 2) fader → amplificador del canal
    x = this.lpf.run(this.lsPost.run(x));
    x = R3 * opamp(x / R3, W3, C3);                         // 3) bus de mezcla
    return this.hpOut.run(x);
  };
  // procesa una muestra base (ya con la ganancia de entrada); devuelve la salida húmeda retrasada LATENCY muestras.
  // Mismas sumas y en el mismo orden que TapeDSP.h (resultados idénticos), con lecturas seguidas para ir más rápido.
  Channel.prototype.tick = function (x) {
    var OS = T.OS, xh = this.xh, ring = this.ring, p, j, k, acc, w, b;
    w = this.xw = (this.xw + XL - 1) % XL;                // escribe hacia atrás: xh[w + j] = x[n − j]
    xh[w] = xh[w + XL] = x * HEAD;
    for (p = 0; p < OS; p++) {
      var ph = PH[p], n = ph.length;
      acc = 0;
      for (j = 0; j < n; j++) acc += ph[j] * xh[w + j];
      var y = this.chain(acc * OS), r = this.rw = (this.rw + RL - 1) % RL;
      ring[r] = ring[r + RL] = y;
    }
    // salida en el instante 4n (el primero de los cuatro): 3 posiciones antes de la última
    b = this.rw + 3; acc = 0;
    for (k = 0; k < TAPS; k++) acc += FIR[k] * ring[b + k];
    return acc / HEAD;
  };
  Channel.prototype.delayDry = function (x) {      // seco alineado con la latencia
    var d = this.dly, n = d.length, i = this.di; d[i] = x; this.di = (i + 1) % n; return d[(i + 1) % n];
  };
  T.Channel = Channel;

  /* Procesado offline de un sonido. bufs = [L, R] (R puede faltar o ser el mismo array que L).
     Devuelve [L, R] de la misma longitud, sin latencia. */
  T.processBuffers = function (bufs, sr, params) {
    var P = Object.assign({}, T.DEFAULTS, params || {}), L = bufs[0], R = bufs[1] || bufs[0], mono = R === L, N = L.length;
    var m = T.map(P.drive, P.bass, P.treble, sr), gin = db2lin(P.inDb || 0), gout = db2lin(P.outDb || 0), mix = clamp(P.mix, 0, 1);
    var ag = P.auto ? T.autoGain(P.drive) : 1, lat = T.LATENCY, outs = [], c, i;
    for (c = 0; c < (mono ? 1 : 2); c++) {
      var src = c ? R : L, ch = new Channel(sr), o = new Float32Array(N);
      ch.setParams(m);
      for (i = 0; i < N + lat; i++) {
        var x = i < N ? src[i] * gin : 0, wet = ch.tick(x) * ag, dry = ch.delayDry(x);
        if (i >= lat) o[i - lat] = (dry * (1 - mix) + wet * mix) * gout;
      }
      outs.push(o);
    }
    return [outs[0], mono ? outs[0] : outs[1]];
  };

  /* Presets de fábrica (iguales en vst/Source/Presets.h). cat: general · guitarra · bajo · bateria.
     Afinados a partir de cómo se grababan esos géneros (ver HOJA_DE_PROYECTO.md, «Presets»). */
  T.PRESETS = [
    { id: 'clean', cat: 'general', name: 'Limpio cálido', p: { drive: 0.12 } },
    { id: 'thick', cat: 'general', name: 'Grosor', p: { drive: 0.32, bass: 1.5, treble: -1 } },
    { id: 'vox', cat: 'general', name: 'Voz lo-fi', p: { drive: 0.5, bass: -4, treble: -3 } },
    { id: 'g_punk77', cat: 'guitarra', name: 'Punk 77', p: { drive: 0.78, bass: 2, treble: 3 } },
    { id: 'g_hc', cat: 'guitarra', name: 'Hardcore 80s', p: { drive: 0.88, bass: 3, treble: 4 } },
    { id: 'g_pp_thin', cat: 'guitarra', name: 'Post-punk cortante', p: { drive: 0.38, bass: -5, treble: 5 } },
    { id: 'g_pp_dark', cat: 'guitarra', name: 'Post-punk oscuro', p: { drive: 0.3, bass: 1, treble: -2 } },
    { id: 'g_jangle', cat: 'guitarra', name: 'Indie jangle 80s', p: { drive: 0.18, bass: -2, treble: 4 } },
    { id: 'g_lofi', cat: 'guitarra', name: 'Indie lo-fi 4 pistas', p: { drive: 0.62, bass: -1, treble: -4 } },
    { id: 'g_di', cat: 'guitarra', name: 'DI a la consola', p: { drive: 0.58, bass: 2, treble: -3 } },
    { id: 'g_grunge_raw', cat: 'guitarra', name: 'Grunge crudo', p: { drive: 0.92, bass: 6, treble: 4 } },
    { id: 'g_grunge', cat: 'guitarra', name: 'Grunge vaciado', p: { drive: 0.74, bass: 5, treble: 5 } },
    { id: 'g_noise', cat: 'guitarra', name: 'Noise muro', p: { drive: 1, bass: 4, treble: 7 } },
    { id: 'g_noise_metal', cat: 'guitarra', name: 'Noise metálico', p: { drive: 0.95, bass: -4, treble: 9 } },
    { id: 'b_punk', cat: 'bajo', name: 'Bajo punk', p: { drive: 0.55, bass: 3, treble: 2 } },
    { id: 'b_pp', cat: 'bajo', name: 'Bajo post-punk agudo', p: { drive: 0.42, bass: 3, treble: 7 } },
    { id: 'b_grunge', cat: 'bajo', name: 'Bajo grunge', p: { drive: 0.64, bass: 5, treble: 3, mix: 0.85 } },
    { id: 'b_noise', cat: 'bajo', name: 'Bajo noise', p: { drive: 0.9, bass: 2, treble: 5, mix: 0.7 } },
    { id: 'b_indie', cat: 'bajo', name: 'Bajo indie redondo', p: { drive: 0.35, bass: 3, treble: -3 } },
    { id: 'd_garage', cat: 'bateria', name: 'Batería punk de garaje', p: { drive: 0.7, bass: 2, treble: 3, mix: 0.8 } },
    { id: 'd_4track', cat: 'bateria', name: 'Batería 4 pistas', p: { drive: 0.6, bass: 1, treble: -5 } },
    { id: 'd_crush', cat: 'bateria', name: 'Batería industrial aplastada', p: { drive: 0.96, bass: 4, treble: 6, mix: 0.6 } },
    { id: 'd_snare', cat: 'bateria', name: 'Caja post-punk', p: { drive: 0.5, bass: -3, treble: 5, mix: 0.7 } },
    { id: 'd_bus', cat: 'bateria', name: 'Bus en paralelo', p: { drive: 0.55, mix: 0.35 } }
  ];

  return T;
}
if (typeof module !== 'undefined' && module.exports) module.exports = tpTapeHead({});
else tpTapeHead(typeof window !== 'undefined' ? window : globalThis);
