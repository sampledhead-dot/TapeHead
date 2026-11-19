# TapeHead — plugin VST3 / AU / Standalone (JUCE 9)

## Compilar
GitHub Actions (`build-vst`) compila Windows y Mac en cada cambio de `vst/`, `dsp/` o `tests/` y publica la release `vst-vX.Y.Z`.

En local: CMake ≥ 3.22, Git y un compilador (Visual Studio 2022 · Xcode).
```bash
cmake -S vst -B build -DCMAKE_BUILD_TYPE=Release      # descarga JUCE 9.0.5 (o -DJUCE_DIR=/ruta/JUCE)
cmake --build build --config Release
```

## Pruebas
```bash
node tests/parity.js            # el DSP en C++ da lo mismo que dsp/tapehead.js
node tests/gen_presets.js --check && node tests/presets.js
cmake -S vst -B build -DTAPEHEAD_BUILD_TESTS=ON && cmake --build build --target TapeHeadTests
./build/TapeHeadTests_artefacts/Release/TapeHeadTests   # latencia, Auto-Gain, estabilidad, CPU, estado, editor
```
