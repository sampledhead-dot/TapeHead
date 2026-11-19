/* TapeHead · interfaz web (panel de multipista de casete de los 80, familia Sampled Head).
   Se usa en Tono-Pad y en design/preview.html. Sin librerías; todo el CSS va encerrado en .th-ui.

   var ui = TapeHeadUI.mount(contenedor, {
     state: { drive, bass, treble, mix, auto, on, ... },          // se modifica en sitio
     presets: TapeHead.PRESETS,
     extraKeys: [{ id, label, title, latch, get(), set(v) }],   // teclas de transporte adicionales (opcional)
     onChange(clave, valor, enDirecto)                    // enDirecto = arrastrando un mando
   });
   ui.update()          vuelve a pintar desde el estado
   ui.meter(dB)         mueve la aguja del vúmetro (−20…+3 VU) y hace girar las bobinas
   ui.setLabel(texto)   texto escrito a mano en la etiqueta del casete */
(function (root) {
  'use strict';
  var CSS = [
    '.th-ui{--th-panel:#1f1e1c;--th-panel2:#2a2926;--th-metal1:#d9d5cc;--th-metal2:#a9a59d;--th-silk:#ece7dd;--th-ink:#111;',
    '--th-or:#d9600b;--th-orl:#ef7a22;--th-ord:#a94a07;--th-cream:#f3efe9;--th-red:#c8321f;--th-blue:#2f5d9e;--th-yel:#e3b320;--th-grey:#8d8a84;',
    '--th-amber:#f6d58a;--th-mono:ui-monospace,Menlo,Consolas,monospace;--th-heavy:"Arial Black","Helvetica Neue",Arial,sans-serif;',
    'font-family:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;color:var(--th-silk);border-radius:12px;overflow:hidden;position:relative;',
    'background:repeating-linear-gradient(0deg,rgba(255,255,255,.018) 0 1px,transparent 1px 3px),linear-gradient(180deg,#262522,#1a1918);',
    'box-shadow:0 0 0 2px #0c0c0b,0 10px 0 rgba(0,0,0,.4),inset 0 1px 0 rgba(255,255,255,.08)}',
    '.th-ui *{box-sizing:border-box}',
    /* franja superior de metal cepillado */
    '.th-ui .th-top{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap;padding:12px 18px 14px;',
    'background:repeating-linear-gradient(90deg,rgba(255,255,255,.12) 0 1px,rgba(0,0,0,.04) 1px 2px,transparent 2px 4px),linear-gradient(180deg,var(--th-metal1),var(--th-metal2));',
    'border-bottom:3px solid #0c0c0b;color:var(--th-ink);position:relative;overflow:hidden}',
    '.th-ui .th-sun{position:absolute;width:300px;height:300px;border-radius:50%;background:var(--th-or);left:-70px;top:-190px;opacity:.95}',
    '.th-ui .th-logo{display:flex;align-items:center;gap:12px;position:relative}',
    '.th-ui .th-ico{width:62px;height:62px;flex:none;filter:drop-shadow(0 3px 0 rgba(0,0,0,.35))}',
    '.th-ui .th-word{position:relative;display:inline-block;transform:rotate(-3deg);line-height:1}',
    '.th-ui .th-word b{font-family:var(--th-heavy);font-weight:900;font-size:44px;letter-spacing:-.02em;color:var(--th-ink);-webkit-text-stroke:3px var(--th-cream);paint-order:stroke fill;text-shadow:4px 4px 0 #2b2622;display:inline-block}',
    '.th-ui .th-word b:nth-child(2){transform:translateY(5px) rotate(4deg);margin-left:3px}',
    '.th-ui .th-drips{position:absolute;left:6%;top:94%;width:88%;height:22px;overflow:visible}',
    '.th-ui .th-model{text-align:right;position:relative}',
    '.th-ui .th-model .m1{font-family:var(--th-heavy);font-size:12px;letter-spacing:.32em}',
    '.th-ui .th-model .m2{font-size:10px;letter-spacing:.22em;font-weight:700;opacity:.75;margin-top:3px}',
    '.th-ui .th-model .m3{display:inline-flex;gap:6px;margin-top:6px}',
    '.th-ui .th-model .m3 i{width:22px;height:5px;border-radius:2px;display:inline-block}',
    /* cuerpo */
    '.th-ui .th-body{padding:16px 18px 18px;display:grid;gap:16px;grid-template-columns:minmax(260px,1.35fr) minmax(200px,1fr)}',
    '@media (max-width:640px){.th-ui .th-body{grid-template-columns:1fr}}',
    '.th-ui .th-silk{font-size:10px;letter-spacing:.24em;font-weight:800;text-transform:uppercase;color:var(--th-silk);opacity:.85}',
    /* ventana del casete */
    '.th-ui .th-deck{position:relative;border-radius:10px;padding:10px;background:linear-gradient(180deg,#141413,#0d0d0c);box-shadow:inset 0 0 0 2px #3a3833,inset 0 8px 18px rgba(0,0,0,.6)}',
    '.th-ui .th-deck svg{display:block;width:100%;height:auto}',
    '.th-ui .th-deck .glass{position:absolute;inset:10px;border-radius:6px;background:linear-gradient(160deg,rgba(255,255,255,.10),transparent 38%,transparent 70%,rgba(255,255,255,.05));pointer-events:none}',
    '.th-ui .th-counter{display:flex;align-items:center;gap:8px;margin-top:10px}',
    '.th-ui .th-counter .dig{display:inline-flex;gap:2px;background:#0b0b0a;padding:3px;border-radius:4px;box-shadow:inset 0 0 0 1px #3a3833}',
    '.th-ui .th-counter .dig span{width:15px;height:21px;background:linear-gradient(180deg,#fafafa,#d8d4cc 50%,#fafafa);color:#111;font:700 15px/21px var(--th-mono);text-align:center;border-radius:2px}',
    /* vúmetro */
    '.th-ui .th-vu{position:relative;border-radius:10px;padding:10px;background:#0d0d0c;box-shadow:inset 0 0 0 2px #3a3833}',
    '.th-ui .th-vu svg{display:block;width:100%;height:auto;border-radius:5px}',
    '.th-ui .th-led{display:inline-flex;align-items:center;gap:6px;margin-top:10px}',
    '.th-ui .th-led i{width:10px;height:10px;border-radius:50%;background:#4a1810;box-shadow:inset 0 0 0 1px #000}',
    '.th-ui .th-led i.on{background:#ff3b22;box-shadow:0 0 8px #ff3b22}',
    '.th-ui .th-led i.g.on{background:#62e24a;box-shadow:0 0 8px #62e24a}',
    /* mandos */
    '.th-ui .th-knobs{grid-column:1/-1;display:flex;gap:6px;justify-content:space-around;align-items:flex-end;flex-wrap:wrap;padding:14px 6px 6px;border-top:1px solid #3a3833;border-bottom:1px solid #3a3833}',
    '.th-ui .th-knob{display:flex;flex-direction:column;align-items:center;gap:3px;width:90px;user-select:none;-webkit-user-select:none}',
    '.th-ui .th-knob.big{width:150px}',
    '.th-ui .th-knob svg{width:84px;height:84px;touch-action:none;cursor:ns-resize;overflow:visible}',
    '.th-ui .th-knob.big svg{width:132px;height:132px}',
    '.th-ui .th-knob .lbl{font-size:11px;letter-spacing:.22em;font-weight:900;color:var(--th-silk)}',
    '.th-ui .th-knob .val{font:12px var(--th-mono);color:var(--th-amber);opacity:.9;min-height:15px}',
    /* teclas de transporte */
    '.th-ui .th-keys{grid-column:1/-1;display:flex;gap:6px;flex-wrap:wrap;align-items:stretch}',
    '.th-ui .th-key{flex:1 1 74px;min-width:64px;min-height:54px;border:0;border-radius:4px 4px 7px 7px;cursor:pointer;padding:6px 6px 10px;',
    'background:linear-gradient(180deg,#f2efe8,#c9c5bc 70%,#a9a59d);color:#1a1a1a;font:900 13px var(--th-heavy);letter-spacing:.06em;',
    'box-shadow:0 6px 0 #5f5c56,0 8px 8px rgba(0,0,0,.5),inset 0 1px 0 #fff;transition:transform .06s,box-shadow .06s;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:4px}',
    '.th-ui .th-key small{font:800 9px system-ui,sans-serif;letter-spacing:.16em;opacity:.7}',
    '.th-ui .th-key:active,.th-ui .th-key.on{transform:translateY(4px);box-shadow:0 2px 0 #5f5c56,0 3px 4px rgba(0,0,0,.5),inset 0 1px 0 #fff}',
    '.th-ui .th-key.on{background:linear-gradient(180deg,#ffb36b,var(--th-or) 70%,var(--th-ord))}',
    '.th-ui .th-key.rec .dot{width:11px;height:11px;border-radius:50%;background:var(--th-red);display:inline-block}',
    '.th-ui .th-key:disabled{opacity:.45;cursor:default}',
    '.th-ui .th-psel{flex:1;min-width:0;background:#0b0b0a;color:var(--th-amber);border:1px solid #3a3833;border-radius:6px;padding:7px 8px;font:700 12px var(--th-mono);letter-spacing:.04em;height:auto}',
    '.th-ui .th-deck{cursor:pointer}',
    '.th-ui .th-foot{grid-column:1/-1;display:flex;justify-content:space-between;gap:10px;flex-wrap:wrap;font-size:11px;color:var(--th-silk);opacity:.65;font-family:var(--th-mono)}',
    '.th-ui.off .th-knobs,.th-ui.off .th-deck,.th-ui.off .th-vu{opacity:.45}',
    '@media (max-width:520px){.th-ui .th-word b{font-size:34px}.th-ui .th-ico{width:48px;height:48px}.th-ui .th-knob{width:80px}.th-ui .th-knob svg{width:70px;height:70px}.th-ui .th-knob.big{width:120px}.th-ui .th-knob.big svg{width:112px;height:112px}}'
  ].join('');

  // Icono: el círculo naranja de Sampled Head con un casete en lugar de la placa del sampler
  var ICON = '<svg class="th-ico" viewBox="0 0 64 64" aria-hidden="true">' +
    '<circle cx="32" cy="33.5" r="31" fill="#a94a07"/><circle cx="32" cy="32" r="31" fill="#d9600b"/>' +
    '<g transform="rotate(-6 32 32)"><rect x="5" y="15" width="54" height="35" rx="3" fill="#111"/>' +
    '<rect x="8" y="18" width="48" height="20" rx="1.5" fill="#f3efe9"/><rect x="8" y="18" width="48" height="4" fill="#d9600b"/>' +
    '<rect x="17" y="25" width="30" height="10" rx="5" fill="#111"/>' +
    '<circle cx="23" cy="30" r="3.6" fill="#f3efe9"/><circle cx="41" cy="30" r="3.6" fill="#f3efe9"/>' +
    '<circle cx="23" cy="30" r="1.3" fill="#111"/><circle cx="41" cy="30" r="1.3" fill="#111"/>' +
    '<path d="M16 50 L19 42 H45 L48 50 Z" fill="#2b2622"/><circle cx="24" cy="46" r="1.4" fill="#f3efe9"/><circle cx="40" cy="46" r="1.4" fill="#f3efe9"/></g></svg>';
  var DRIPS = '<svg class="th-drips" viewBox="0 0 200 22" preserveAspectRatio="none" aria-hidden="true"><g fill="#111" stroke="#f3efe9" stroke-width="1.5">' +
    '<path d="M20 0 h5 v13 a2.5 2.5 0 0 1 -5 0z"/><path d="M78 0 h4 v18 a2 2 0 0 1 -4 0z"/><path d="M126 0 h5 v9 a2.5 2.5 0 0 1 -5 0z"/><path d="M172 0 h4 v15 a2 2 0 0 1 -4 0z"/></g></svg>';

  // Bobina: rueda dentada blanca con 6 radios, cinta enrollada alrededor (radio = cantidad)
  function reel(cx, cy, pack) {
    var s = '<circle cx="' + cx + '" cy="' + cy + '" r="' + pack + '" fill="#3b2a20"/>' +
      '<circle cx="' + cx + '" cy="' + cy + '" r="' + pack + '" fill="none" stroke="#5a4232" stroke-width="1" stroke-dasharray="1 3" opacity=".7"/>' +
      '<g class="th-reel"><circle cx="' + cx + '" cy="' + cy + '" r="15" fill="#eae6de"/>';
    for (var i = 0; i < 6; i++) {
      var a = i * Math.PI / 3, x = cx + Math.cos(a) * 9.5, y = cy + Math.sin(a) * 9.5;
      s += '<circle cx="' + x.toFixed(2) + '" cy="' + y.toFixed(2) + '" r="3.6" fill="#1a1918"/>';
    }
    for (i = 0; i < 6; i++) { var b = i * Math.PI / 3 + Math.PI / 6; s += '<rect x="' + (cx - 1.4) + '" y="' + (cy - 5.6) + '" width="2.8" height="3" fill="#1a1918" transform="rotate(' + (b * 180 / Math.PI) + ' ' + cx + ' ' + cy + ')"/>'; }
    return s + '<circle cx="' + cx + '" cy="' + cy + '" r="3" fill="#1a1918"/></g>';
  }
  var DECK = '<svg viewBox="0 0 340 190" aria-hidden="true">' +
    '<rect x="6" y="6" width="328" height="178" rx="10" fill="#2b2a27"/><rect x="6" y="6" width="328" height="178" rx="10" fill="none" stroke="#45423c" stroke-width="2"/>' +
    '<g fill="#1a1918">' + [0, 1, 2, 3].map(function (i) { return '<circle cx="' + (18 + (i % 2) * 304) + '" cy="' + (18 + (i > 1 ? 154 : 0)) + '" r="4"/>'; }).join('') + '</g>' +
    '<rect x="22" y="16" width="296" height="118" rx="5" fill="#f3efe9"/>' +
    '<rect x="22" y="16" width="296" height="16" rx="5" fill="#d9600b"/><rect x="22" y="26" width="296" height="6" fill="#d9600b"/>' +
    '<text x="32" y="28.5" font-family="Arial Black,Arial" font-size="10" letter-spacing="2.4" fill="#111">SAMPLED HEAD · C-90</text>' +
    '<text x="308" y="28.5" text-anchor="end" font-family="Arial Black,Arial" font-size="10" fill="#111">A</text>' +
    '<line x1="32" y1="50" x2="308" y2="50" stroke="#c9c2b6"/><line x1="32" y1="122" x2="308" y2="122" stroke="#c9c2b6"/>' +
    '<text class="th-label" x="40" y="46" font-family="Marker Felt,Comic Sans MS,Segoe Print,cursive" font-size="17" fill="#1b3f8a" transform="rotate(-1.5 40 46)"></text>' +
    '<rect x="78" y="60" width="184" height="56" rx="28" fill="#141312"/>' +
    '<rect x="146" y="70" width="48" height="36" fill="#0a0a09"/><rect x="146" y="70" width="48" height="36" fill="url(#thWin)" opacity=".5"/>' +
    reel(118, 88, 26) + reel(222, 88, 19) +
    '<path d="M70 184 L86 146 H254 L270 184 Z" fill="#232220"/><path d="M86 146 H254" stroke="#45423c"/>' +
    '<circle cx="118" cy="166" r="6" fill="#0a0a09"/><circle cx="222" cy="166" r="6" fill="#0a0a09"/>' +
    '<rect x="158" y="158" width="24" height="12" rx="2" fill="#0a0a09"/>' +
    '<defs><linearGradient id="thWin" x1="0" x2="1"><stop offset="0" stop-color="#3b2a20"/><stop offset="1" stop-color="#5a4232"/></linearGradient></defs></svg>';

  // Vúmetro: escala −20…+3 VU, zona roja desde 0
  function vuAngle(db) { var t = (Math.max(-20, Math.min(3, db)) + 20) / 23; t = Math.pow(t, 1.7); return -48 + 96 * t; }
  function vuSvg() {
    var cx = 120, cy = 150, r = 104, s = '<svg viewBox="0 0 240 150" aria-hidden="true"><defs><radialGradient id="thAmb" cx=".5" cy=".9" r=".9"><stop offset="0" stop-color="#ffe7a8"/><stop offset="1" stop-color="#e9b85a"/></radialGradient></defs>' +
      '<rect width="240" height="150" fill="url(#thAmb)"/>';
    function pt(db, rr) { var a = vuAngle(db) * Math.PI / 180; return [cx + rr * Math.sin(a), cy - rr * Math.cos(a)]; }
    var a0 = pt(-20, r), a1 = pt(0, r), a2 = pt(3, r);
    s += '<path d="M' + a0[0].toFixed(1) + ' ' + a0[1].toFixed(1) + ' A' + r + ' ' + r + ' 0 0 1 ' + a1[0].toFixed(1) + ' ' + a1[1].toFixed(1) + '" fill="none" stroke="#1a1a1a" stroke-width="2"/>';
    s += '<path d="M' + a1[0].toFixed(1) + ' ' + a1[1].toFixed(1) + ' A' + r + ' ' + r + ' 0 0 1 ' + a2[0].toFixed(1) + ' ' + a2[1].toFixed(1) + '" fill="none" stroke="#c8321f" stroke-width="6"/>';
    [-20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3].forEach(function (d) {
      var p1 = pt(d, r), p2 = pt(d, r + (d % 5 === 0 || d > -4 ? 9 : 5)), lp = pt(d, r + 19);
      s += '<line x1="' + p1[0].toFixed(1) + '" y1="' + p1[1].toFixed(1) + '" x2="' + p2[0].toFixed(1) + '" y2="' + p2[1].toFixed(1) + '" stroke="' + (d > 0 ? '#c8321f' : '#1a1a1a') + '" stroke-width="1.6"/>';
      if ([-20, -10, -5, -3, 0, 3].indexOf(d) >= 0) s += '<text x="' + lp[0].toFixed(1) + '" y="' + (lp[1] + 3).toFixed(1) + '" text-anchor="middle" font-family="Arial" font-weight="700" font-size="9" fill="' + (d > 0 ? '#c8321f' : '#1a1a1a') + '">' + (d > 0 ? '+' + d : d) + '</text>';
    });
    s += '<text x="120" y="96" text-anchor="middle" font-family="Arial Black,Arial" font-size="17" fill="#1a1a1a">VU</text>' +
      '<text x="120" y="112" text-anchor="middle" font-family="Arial" font-weight="700" font-size="7" letter-spacing="2" fill="#1a1a1a">TAPEHEAD · OUTPUT</text>' +
      '<g class="th-needle" style="transform-origin:120px 150px;transition:transform .12s"><line x1="120" y1="150" x2="120" y2="38" stroke="#111" stroke-width="1.8"/></g>' +
      '<rect x="0" y="128" width="240" height="22" fill="#2b2a27"/><circle cx="120" cy="150" r="14" fill="#111"/></svg>';
    return s;
  }

  function knobSvg(big, cap) {
    var ticks = '', n = big ? 11 : 11, R = 46, i;
    for (i = 0; i < n; i++) {
      var a = (-135 + 270 * i / (n - 1)) * Math.PI / 180, x1 = 50 + R * Math.sin(a), y1 = 50 - R * Math.cos(a), x2 = 50 + (R - 5) * Math.sin(a), y2 = 50 - (R - 5) * Math.cos(a);
      ticks += '<line x1="' + x1.toFixed(1) + '" y1="' + y1.toFixed(1) + '" x2="' + x2.toFixed(1) + '" y2="' + y2.toFixed(1) + '" stroke="#ece7dd" stroke-width="' + (i % 5 === 0 ? 2 : 1.2) + '"/>';
      if (big && i % 2 === 0) { var tx = 50 + (R + 7) * Math.sin(a), ty = 50 - (R + 7) * Math.cos(a); ticks += '<text x="' + tx.toFixed(1) + '" y="' + (ty + 3).toFixed(1) + '" text-anchor="middle" font-family="Arial" font-weight="700" font-size="7" fill="#ece7dd">' + i + '</text>'; }
    }
    var skirt = '';
    for (i = 0; i < 24; i++) skirt += '<line x1="50" y1="15" x2="50" y2="19" stroke="#000" stroke-width="1.6" transform="rotate(' + i * 15 + ' 50 50)"/>';
    return '<svg viewBox="0 0 100 100" role="slider" tabindex="0">' + ticks +
      '<circle cx="50" cy="52.5" r="35" fill="#000" opacity=".5"/><circle cx="50" cy="50" r="35" fill="#2a2926"/><circle cx="50" cy="50" r="35" fill="none" stroke="#45423c" stroke-width="1.5"/>' +
      '<g class="rot">' + skirt + '<circle cx="50" cy="50" r="24" fill="' + cap + '"/><circle cx="50" cy="50" r="24" fill="url(#thCap)"/>' +
      '<rect x="48.4" y="14" width="3.2" height="21" rx="1.4" fill="#f3efe9"/></g>' +
      '<defs><radialGradient id="thCap" cx=".35" cy=".3" r=".8"><stop offset="0" stop-color="#fff" stop-opacity=".45"/><stop offset=".55" stop-color="#fff" stop-opacity="0"/><stop offset="1" stop-color="#000" stop-opacity=".3"/></radialGradient></defs></svg>';
  }

  function fmtDb(v) { return Math.abs(v) < 0.05 ? '0 dB' : (v > 0 ? '+' : '') + v.toFixed(1) + ' dB'; }
  var KNOBS = {
    drive: { lbl: 'DRIVE', min: 0, max: 1, def: 0.35, big: true, cap: '#c8321f', fmt: function (v) { return (v * 10).toFixed(1); } },
    bass: { lbl: 'GRAVES', min: -10, max: 10, def: 0, cap: '#2f5d9e', fmt: fmtDb },
    treble: { lbl: 'AGUDOS', min: -10, max: 10, def: 0, cap: '#d9600b', fmt: fmtDb },
    mix: { lbl: 'MIX', min: 0, max: 1, def: 1, cap: '#e3b320', fmt: function (v) { return Math.round(v * 100) + ' %'; } },
    inDb: { lbl: 'INPUT', min: -18, max: 18, def: 0, cap: '#8d8a84', fmt: function (v) { return (v > 0 ? '+' : '') + v.toFixed(1) + ' dB'; } },
    outDb: { lbl: 'OUTPUT', min: -18, max: 18, def: 0, cap: '#8d8a84', fmt: function (v) { return (v > 0 ? '+' : '') + v.toFixed(1) + ' dB'; } }
  };

  function mount(host, opt) {
    if (!document.getElementById('th-ui-css')) { var st = document.createElement('style'); st.id = 'th-ui-css'; st.textContent = CSS; document.head.appendChild(st); }
    var S = opt.state, knobs = opt.knobs || ['drive', 'bass', 'treble', 'mix', 'inDb', 'outDb'], presets = opt.presets || [];
    var el = document.createElement('div'); el.className = 'th-ui';
    el.innerHTML = '<div class="th-top"><div class="th-sun"></div><div class="th-logo">' + ICON + '<div class="th-word"><b>TAPE</b><b>HEAD</b>' + DRIPS + '</div></div>' +
      '<div class="th-model"><div class="m1">MULTITRACK PREAMP</div><div class="m2">CASSETTE CONSOLE · MODEL TH-1</div><div class="m3"><i style="background:#c8321f"></i><i style="background:#e3b320"></i><i style="background:#2f5d9e"></i><i style="background:#d9600b"></i></div></div></div>' +
      '<div class="th-body"><div><div class="th-deck">' + DECK + '<div class="glass"></div></div>' +
      '<div class="th-counter"><span class="th-silk">Preset</span><span class="dig"><span>0</span><span>0</span><span>0</span></span><select class="th-psel" aria-label="Preset"></select></div></div>' +
      '<div><div class="th-vu">' + vuSvg() + '</div><div class="th-led"><i class="pk"></i><span class="th-silk">Peak</span><i class="g ag" style="margin-left:14px"></i><span class="th-silk">Auto-gain</span></div></div>' +
      '<div class="th-knobs"></div><div class="th-keys"></div><div class="th-foot"><span class="th-chain">Entrada → TRIM → fader → bus → salida</span><span>' + (opt.footer || '') + '</span></div></div>';
    host.appendChild(el);
    var kwrap = el.querySelector('.th-knobs'), K = {};
    knobs.forEach(function (key) {
      var d = KNOBS[key], w = document.createElement('div'); w.className = 'th-knob' + (d.big ? ' big' : '');
      w.innerHTML = knobSvg(d.big, d.cap) + '<div class="lbl">' + d.lbl + '</div><div class="val"></div>';
      kwrap.appendChild(w);
      var svg = w.querySelector('svg'), rot = w.querySelector('.rot'), val = w.querySelector('.val');
      function norm(v) { return (v - d.min) / (d.max - d.min); }
      function set(t, live) { t = Math.min(1, Math.max(0, t)); var v = d.min + t * (d.max - d.min); if (key !== 'drive' && key !== 'mix') v = Math.round(v * 10) / 10; S[key] = v; K[key].update(); opt.onChange && opt.onChange(key, v, live); }
      K[key] = { update: function () { var t = norm(S[key]); rot.setAttribute('transform', 'rotate(' + (-135 + 270 * t) + ' 50 50)'); val.textContent = d.fmt(S[key]); svg.setAttribute('aria-label', d.lbl); svg.setAttribute('aria-valuetext', d.fmt(S[key])); } };
      var y0 = 0, t0 = 0, drag = false;
      svg.addEventListener('pointerdown', function (e) { drag = true; y0 = e.clientY; t0 = norm(S[key]); try { svg.setPointerCapture(e.pointerId); } catch (_) {} e.preventDefault(); });
      svg.addEventListener('pointermove', function (e) { if (drag) set(t0 + (y0 - e.clientY) * (e.shiftKey ? 0.0012 : 0.005), true); });
      function end() { if (drag) { drag = false; opt.onChange && opt.onChange(key, S[key], false); } }
      svg.addEventListener('pointerup', end); svg.addEventListener('pointercancel', end);
      svg.addEventListener('dblclick', function () { set(norm(d.def), false); });
      svg.addEventListener('wheel', function (e) { e.preventDefault(); set(norm(S[key]) - Math.sign(e.deltaY) * 0.02, false); }, { passive: false });
      svg.addEventListener('keydown', function (e) { var s = e.key === 'ArrowUp' || e.key === 'ArrowRight' ? 1 : e.key === 'ArrowDown' || e.key === 'ArrowLeft' ? -1 : 0; if (s) { e.preventDefault(); set(norm(S[key]) + s * 0.02, false); } });
    });
    // teclas de transporte: ◀◀ ▶▶ = preset anterior/siguiente, AUTO = Auto-Gain, + las que añada la app
    var keys = el.querySelector('.th-keys'), presetIdx = -1;
    function key(label, small, cls, fn) { var b = document.createElement('button'); b.type = 'button'; b.className = 'th-key ' + (cls || ''); b.innerHTML = label + (small ? '<small>' + small + '</small>' : ''); b.onclick = fn; keys.appendChild(b); return b; }
    function applyPreset(i) {
      var n = presets.length; if (!n) return; i = ((i % n) + n) % n; presetIdx = i;
      var p = presets[i].p; ['drive', 'bass', 'treble', 'mix'].forEach(function (k) { S[k] = p[k] != null ? p[k] : KNOBS[k].def; });
      ui.update(); opt.onChange && opt.onChange('preset', presets[i].id, false);
    }
    if (presets.length) { key('◀◀', 'PRESET', '', function () { applyPreset(presetIdx < 0 ? presets.length - 1 : presetIdx - 1); }); key('▶▶', 'PRESET', '', function () { applyPreset(presetIdx + 1); }); }
    var CATN = { general: 'General', guitarra: 'Guitarra', bajo: 'Bajo', bateria: 'Batería' }, sel = el.querySelector('.th-psel'), html = '<option value="">— personalizado —</option>', lastCat = null;
    presets.forEach(function (p, i) {
      if (p.cat !== lastCat) { if (lastCat !== null) html += '</optgroup>'; html += '<optgroup label="' + (CATN[p.cat] || p.cat) + '">'; lastCat = p.cat; }
      html += '<option value="' + i + '">' + p.name + '</option>';
    });
    sel.innerHTML = html + (lastCat !== null ? '</optgroup>' : '');
    sel.onchange = function () { if (sel.value !== '') applyPreset(+sel.value); };
    el.querySelector('.th-deck').addEventListener('click', function () { try { sel.showPicker ? sel.showPicker() : sel.focus(); } catch (_) { sel.focus(); } });
    var bAuto = key('AUTO', 'GAIN', 'latch', function () { S.auto = !S.auto; ui.update(); opt.onChange && opt.onChange('auto', S.auto, false); });
    var extra = (opt.extraKeys || []).map(function (k) { var b = key(k.label, k.small, (k.cls || '') + (k.latch ? ' latch' : ''), function () { k.click(); ui.update(); }); b.title = k.title || ''; return { k: k, b: b }; });

    var reels = el.querySelectorAll('.th-reel'), needle = el.querySelector('.th-needle'), pk = el.querySelector('.pk'), ang = 0, speed = opt.idleSpin || 0, lastT = 0, level = -40, pkT = 0;
    function frame(t) {
      var dt = lastT ? Math.min(0.1, (t - lastT) / 1000) : 0; lastT = t;
      ang = (ang + dt * speed * 140) % 360;
      for (var i = 0; i < reels.length; i++) reels[i].setAttribute('transform', 'rotate(' + (i ? ang * 1.35 : ang).toFixed(1) + ' ' + (i ? '222 88' : '118 88') + ')');
      level = Math.max(-40, level - dt * 22); speed = Math.max(opt.idleSpin || 0, speed - dt * 0.8);
      needle.style.transform = 'rotate(' + vuAngle(level).toFixed(1) + 'deg)';
      pk.classList.toggle('on', t < pkT);
      if (el.isConnected) requestAnimationFrame(frame);
    }
    requestAnimationFrame(frame);

    var ui = {
      el: el,
      update: function () {
        for (var k in K) K[k].update();
        bAuto.classList.toggle('on', !!S.auto); el.querySelector('.ag').classList.toggle('on', !!S.auto);
        extra.forEach(function (x) { if (x.k.get) x.b.classList.toggle('on', !!x.k.get()); if (x.k.disabled) x.b.disabled = !!x.k.disabled(); });
        el.classList.toggle('off', S.on === false);
        // ¿coincide con un preset?
        var hit = -1; presets.forEach(function (p, i) { var q = p.p; if (Math.abs(q.drive - S.drive) < 1e-6 && Math.abs((q.bass || 0) - S.bass) < 1e-6 && Math.abs((q.treble || 0) - S.treble) < 1e-6 && Math.abs((q.mix != null ? q.mix : 1) - S.mix) < 1e-6) hit = i; });
        if (hit >= 0) presetIdx = hit;
        var num = hit >= 0 ? String(hit + 1) : '0', digs = el.querySelectorAll('.dig span'); num = ('000' + num).slice(-3);
        for (var i = 0; i < 3; i++) digs[i].textContent = num[i];
        var name = hit >= 0 ? presets[hit].name : 'Personalizado · drive ' + (S.drive * 10).toFixed(1);
        sel.value = hit >= 0 ? String(hit) : '';
        ui.setLabel(opt.label ? opt.label(name) : name);
      },
      setLabel: function (t) { el.querySelector('.th-label').textContent = t; },
      setChain: function (t) { el.querySelector('.th-chain').textContent = t; },
      meter: function (db, peak) { level = Math.max(level, db); speed = Math.max(speed, 1); if (peak) pkT = performance.now() + 250; },
      spin: function (on) { speed = on ? 1 : 0; }
    };
    ui.update();
    return ui;
  }
  root.TapeHeadUI = { mount: mount, KNOBS: KNOBS };
})(typeof window !== 'undefined' ? window : globalThis);
