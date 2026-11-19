// Genera vst/Source/Presets.h a partir de TapeHead.PRESETS (dsp/tapehead.js), para que el plugin y la web tengan los mismos.
// node tests/gen_presets.js          → escribe el archivo
// node tests/gen_presets.js --check  → falla si el archivo no coincide
'use strict';
const fs = require('fs'), path = require('path'), T = require('../dsp/tapehead.js');
const CAT = { general: 'General', guitarra: 'Guitarra', bajo: 'Bajo', bateria: 'Batería' };
const f = v => (Number.isInteger(v) ? v.toFixed(1) : String(v)) + 'f';
let out = `#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <utility>

// Presets de fábrica. ARCHIVO GENERADO por tests/gen_presets.js desde TapeHead.PRESETS (dsp/tapehead.js): no editar a mano.
// Lo que no aparece en un preset vuelve a su valor por defecto (drive 0.35, graves 0, agudos 0, mix 100 %).
inline juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }

struct FactoryPreset { const char* category; const char* name; std::vector<std::pair<const char*, float>> values; };

inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> list {
        { "General", "Default", {} },
`;
for (const p of T.PRESETS) {
  const v = Object.entries(p.p).map(([k, x]) => `{"${k}", ${f(x)}}`).join(', ');
  out += `        { "${CAT[p.cat]}", "${p.name}", { ${v} } },\n`;
}
out += `    };
    return list;
}
`;
const file = path.join(__dirname, '../vst/Source/Presets.h');
if (process.argv.includes('--check')) { const ok = fs.readFileSync(file, 'utf8') === out; console.log(ok ? 'Presets.h coincide con dsp/tapehead.js' : 'Presets.h NO coincide: node tests/gen_presets.js'); process.exit(ok ? 0 : 1); }
fs.writeFileSync(file, out); console.log('escrito', file, T.PRESETS.length + 1, 'presets');
