#pragma once
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
        { "General", "Limpio cálido", { {"drive", 0.12f} } },
        { "General", "Grosor", { {"drive", 0.32f}, {"bass", 1.5f}, {"treble", -1.0f} } },
        { "General", "Voz lo-fi", { {"drive", 0.5f}, {"bass", -4.0f}, {"treble", -3.0f} } },
        { "Guitarra", "Punk 77", { {"drive", 0.78f}, {"bass", 2.0f}, {"treble", 3.0f} } },
        { "Guitarra", "Hardcore 80s", { {"drive", 0.88f}, {"bass", 3.0f}, {"treble", 4.0f} } },
        { "Guitarra", "Post-punk cortante", { {"drive", 0.38f}, {"bass", -5.0f}, {"treble", 5.0f} } },
        { "Guitarra", "Post-punk oscuro", { {"drive", 0.3f}, {"bass", 1.0f}, {"treble", -2.0f} } },
        { "Guitarra", "Indie jangle 80s", { {"drive", 0.18f}, {"bass", -2.0f}, {"treble", 4.0f} } },
        { "Guitarra", "Indie lo-fi 4 pistas", { {"drive", 0.62f}, {"bass", -1.0f}, {"treble", -4.0f} } },
        { "Guitarra", "DI a la consola", { {"drive", 0.58f}, {"bass", 2.0f}, {"treble", -3.0f} } },
        { "Guitarra", "Grunge crudo", { {"drive", 0.92f}, {"bass", 6.0f}, {"treble", 4.0f} } },
        { "Guitarra", "Grunge vaciado", { {"drive", 0.74f}, {"bass", 5.0f}, {"treble", 5.0f} } },
        { "Guitarra", "Noise muro", { {"drive", 1.0f}, {"bass", 4.0f}, {"treble", 7.0f} } },
        { "Guitarra", "Noise metálico", { {"drive", 0.95f}, {"bass", -4.0f}, {"treble", 9.0f} } },
        { "Bajo", "Bajo punk", { {"drive", 0.55f}, {"bass", 3.0f}, {"treble", 2.0f} } },
        { "Bajo", "Bajo post-punk agudo", { {"drive", 0.42f}, {"bass", 3.0f}, {"treble", 7.0f} } },
        { "Bajo", "Bajo grunge", { {"drive", 0.64f}, {"bass", 5.0f}, {"treble", 3.0f}, {"mix", 0.85f} } },
        { "Bajo", "Bajo noise", { {"drive", 0.9f}, {"bass", 2.0f}, {"treble", 5.0f}, {"mix", 0.7f} } },
        { "Bajo", "Bajo indie redondo", { {"drive", 0.35f}, {"bass", 3.0f}, {"treble", -3.0f} } },
        { "Batería", "Batería punk de garaje", { {"drive", 0.7f}, {"bass", 2.0f}, {"treble", 3.0f}, {"mix", 0.8f} } },
        { "Batería", "Batería 4 pistas", { {"drive", 0.6f}, {"bass", 1.0f}, {"treble", -5.0f} } },
        { "Batería", "Batería industrial aplastada", { {"drive", 0.96f}, {"bass", 4.0f}, {"treble", 6.0f}, {"mix", 0.6f} } },
        { "Batería", "Caja post-punk", { {"drive", 0.5f}, {"bass", -3.0f}, {"treble", 5.0f}, {"mix", 0.7f} } },
        { "Batería", "Bus en paralelo", { {"drive", 0.55f}, {"mix", 0.35f} } },
    };
    return list;
}
