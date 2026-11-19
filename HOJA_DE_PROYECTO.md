# TapeHead — Hoja de proyecto

Familia de apps de **Sampled Head**. **Versión actual:** 1.0.0 (19-11-2026) ·
**Repositorio:** https://github.com/sampledhead-dot/TapeHead

## Descripción
Efecto inspirado en un plugin comercial que modela solo la **etapa de entrada** de una grabadora multipista de casete
de finales de los 80 (op-amps de consola económica japonesa), usada como saturador y caja DI. Nombre, diseño visual y
código propios: no se usan marcas ni nombres del producto de referencia ni del fabricante de la grabadora.
Sin artefactos mecánicos de cinta (ni wow & flutter, ni ruido, ni pérdida de cabezales): solo la electrónica.

Dónde está:
1. **Plugin** (`vst/`): VST3 (Win/Mac), AU (Mac) y Standalone, JUCE 9.0.5, tiempo real.
2. **Tono-Pad**: efecto de los pads (pestaña «TapeHead»), mismo motor.
3. Sin app web propia (decisión del usuario, 19-11-2026). `design/preview.html` es solo la vista previa del diseño.

## Estética (decisión del usuario: grabadora de casete de los 80 + logo de Sampled Head)
- Franja superior de **metal cepillado** con el gran **círculo naranja** (#d9600b) del logo de Sampled Head.
- **Icono**: un casete negro con etiqueta crema y franja naranja sobre el círculo naranja (en lugar de la placa de sampler).
- Palabra **TAPEHEAD** en negro con contorno crema, sombra y goteos (grafiti, como FUZZHEAD).
- Panel **negro con serigrafía blanca**, «MULTITRACK PREAMP · CASSETTE CONSOLE · MODEL TH-1» y 4 franjas de color.
- **Ventana de casete**: etiqueta «SAMPLED HEAD · C-90» con el **nombre del preset escrito a mano**; las bobinas
  **giran cuando pasa audio**. Tocar el casete abre los presets.
- **Vúmetro de aguja** con luz ámbar (0 VU = −18 dBFS RMS, balística ~300 ms), pilotos **PEAK** (> −1 dBFS) y **AUTO-GAIN**.
- **Contador mecánico** de 3 cifras = número del preset.
- Mandos de consola con **faldón estriado y capuchón de color**: DRIVE grande rojo (escala 0–10), GRAVES azul,
  AGUDOS naranja, MIX amarillo, INPUT/OUTPUT gris.
- **Teclas de transporte** de plástico crema que se hunden: ◀◀ / ▶▶ preset, AUTO GAIN (se queda hundida y naranja),
  ● GUARDAR, ⏏ BORRAR (en Tono-Pad: teclas propias de la app).

## Motor de sonido (igual en `dsp/tapehead.js` y `vst/Source/dsp/TapeDSP.h`)
```
Input → ×0,5 (margen) → [×4] → HPF 20 Hz → estante graves 180 Hz (−1…−6 dB) → campana 1 kHz (+1,5…+6 dB)
      → op-amp TRIM (ganancia 0…+40 dB, asimetría fija) → LP 20 kHz → EQ de canal (graves 120 Hz / agudos 5 kHz, ±10 dB)
      → op-amp FADER (+0…+16 dB a partir de Drive 4, asimetría creciente) → estante graves +0,5…+3 dB → LP 16→8 kHz
      → op-amp BUS (suave) → HPF 10 Hz → [÷4] → Auto-Gain → Mix con el seco (retrasado) → Output
```
- **Op-amp**: recorte con rodilla cuadrática (raíl = 1) + pizca de curva cúbica para el grosor a nivel bajo.
- **Sobremuestreo ×4** con FIR de fase lineal de 129 coeficientes (Kaiser β 8) → **latencia fija de 32 muestras**
  (el plugin la declara; el seco va retrasado lo mismo para que Mix no tenga filtro peine).
- **Auto-Gain**: tabla de 21 puntos medida con `tests/calibrate.js` (ponderación K, material a −14 dBFS RMS),
  relativa a la señal original: con Auto-Gain, A/B contra bypass queda al mismo volumen (±0,5 dB medido en el plugin).
- Medidas (seno 220 Hz): Drive 0 → 0,06 % THD a −18 dBFS; Drive 5 → 22 % a −6 dBFS; Drive 10 → ~38 % (fuzz). Medios
  +6 dB a Drive 5. CPU del plugin: ~2,6 % de un núcleo en estéreo a 48 kHz (prueba en Linux).
- **Paridad**: diferencias JS↔C++ < 2·10⁻⁸ (`node tests/parity.js`).

## Presets (24 + Default) · `TapeHead.PRESETS` → `vst/Source/Presets.h` (generado)
Investigación (19-11-2026): la consola real tiene TRIM + EQ de graves/agudos (±10 dB, 100 Hz / 10 kHz) tras el previo;
el punk/grunge se graba con mucha ganancia y medios recortados (graves y agudos arriba); el bajo post-punk (Joy Division)
es agudo y al frente (graves y agudos al máximo, medios no); el bajo grunge (Nirvana) es un SVT con humbucker, gruñón y
algo vaciado; el lo-fi de 4 pistas (Guided By Voices, Sebadoh, Pavement) suena apagado por arriba; en batería la
distorsión se usa en paralelo.

| Categoría | Preset | Drive | Graves | Agudos | Mix | Idea |
|---|---|---|---|---|---|---|
| General | Limpio cálido | 1,2 | 0 | 0 | 100 | grosor sin distorsión audible |
| | Grosor | 3,2 | +1,5 | −1 | 100 | |
| | Voz lo-fi | 5 | −4 | −3 | 100 | voz de maqueta |
| Guitarra | Punk 77 | 7,8 | +2 | +3 | 100 | muro de acordes crujiente |
| | Hardcore 80s | 8,8 | +3 | +4 | 100 | más ganancia y ataque |
| | Post-punk cortante | 3,8 | −5 | +5 | 100 | fina y aguda (Gang of Four, Sumner) |
| | Post-punk oscuro | 3 | +1 | −2 | 100 | limpia con grano, para chorus/reverb |
| | Indie jangle 80s | 1,8 | −2 | +4 | 100 | arpegios brillantes (R.E.M., The Smiths) |
| | Indie lo-fi 4 pistas | 6,2 | −1 | −4 | 100 | maqueta de cuarto (GBV, Sebadoh) |
| | DI a la consola | 5,8 | +2 | −3 | 100 | guitarra enchufada directa a la mesa |
| | Grunge crudo | 9,2 | +6 | +4 | 100 | sucio y pesado (época Bleach) |
| | Grunge vaciado | 7,4 | +5 | +5 | 100 | medios recortados (época Nevermind) |
| | Noise muro | 10 | +4 | +7 | 100 | pared de ruido (Sonic Youth, JAMC) |
| | Noise metálico | 9,5 | −4 | +9 | 100 | abrasivo y agudo (Big Black) |
| Bajo | Bajo punk | 5,5 | +3 | +2 | 100 | con púa, empujado |
| | Bajo post-punk agudo | 4,2 | +3 | +7 | 100 | bajo melódico al frente (Peter Hook) |
| | Bajo grunge | 6,4 | +5 | +3 | 85 | gruñido con fondo limpio (Novoselic) |
| | Bajo noise | 9 | +2 | +5 | 70 | fuzz en paralelo para no perder graves |
| | Bajo indie redondo | 3,5 | +3 | −3 | 100 | cálido (Kim Deal) |
| Batería | Batería punk de garaje | 7 | +2 | +3 | 80 | sala reventada |
| | Batería 4 pistas | 6 | +1 | −5 | 100 | maqueta apagada |
| | Batería industrial aplastada | 9,6 | +4 | +6 | 60 | caja de ritmos destrozada en paralelo |
| | Caja post-punk | 5 | −3 | +5 | 70 | chasquido seco |
| | Bus en paralelo | 5,5 | 0 | 0 | 35 | pegamento para todo el kit |

Los presets no tocan Input, Output ni Auto-Gain. Nivel de cada uno medido con `node tests/presets.js`
(todos entre −1,7 y +3,4 dB respecto a la entrada, con material sintético de su categoría). **No oídos aún con
instrumentos reales**: ajustarlos al oído.

## Archivos
| Ruta | Contenido |
|---|---|
| `dsp/tapehead.js` | Motor de referencia (JS) + `PRESETS` + `processBuffers` para procesado offline (Tono-Pad) |
| `web/th-ui.js` | Interfaz web (casete, vúmetro, mandos, teclas) para Tono-Pad |
| `design/preview.html` | Vista previa del diseño |
| `vst/Source/dsp/TapeDSP.h` | Motor en C++ (sin JUCE) |
| `vst/Source/PluginProcessor.*`, `PluginEditor.*` | Plugin |
| `vst/Source/Presets.h` | Generado por `tests/gen_presets.js` |
| `vst/Source/UpdateBanner.h` | Aviso de versión nueva (común a los plugins de Sampled Head) |
| `tests/` | `calibrate.js` (medidas y tabla Auto-Gain), `parity.js/.cpp`, `presets.js`, `gen_presets.js` |
| `vst/tests/TestMain.cpp` | Pruebas del plugin (latencia, Auto-Gain, estabilidad, CPU, estado, editor, aviso) |
| `.github/workflows/build-vst.yml` | Pruebas en Linux + Windows y Mac universal; release `vst-vX.Y.Z` |

## Reglas permanentes
- Versionado semántico `MAYOR.MENOR.PARCHE`; versión en `vst/CMakeLists.txt` **y** `kVersion` del procesador.
- **Paridad**: cualquier cambio de sonido en `dsp/tapehead.js` **y** `TapeDSP.h` (y copiar `tapehead.js` a Tono-Pad).
  Presets: editar solo `TapeHead.PRESETS` y ejecutar `node tests/gen_presets.js`.
- Si cambia la cadena, recalcular `AUTO_DB` con `node tests/calibrate.js --table` (en los dos archivos).
- Probar antes de entregar: `tests/*.js` + `TapeHeadTests`.

## Historial
- **1.0.0** (19-11-2026): primera versión. Plugin VST3/AU/Standalone, 24 presets por géneros (punk, hardcore,
  post-punk, indie 80s/90s, lo-fi, grunge, noise) para guitarra, bajo y batería, estética de multipista de casete,
  integrado en Tono-Pad.

## Límites conocidos
- Es un modelo propio aproximado de la topología (op-amps en cascada + EQ), no una medición de un aparato real.
- Plugin compilado y probado solo en Linux sin DAW; Windows y Mac se compilan en GitHub Actions. Sin firma de Apple.
- La letra «a mano» del casete usa Marker Felt (Mac) o Segoe Print (Windows); en otros sistemas, cursiva normal.
