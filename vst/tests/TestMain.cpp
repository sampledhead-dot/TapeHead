// Pruebas offline del plugin: latencia, mezcla, Auto-Gain, estabilidad, rendimiento, estado, presets y captura del editor.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/Presets.h"
#include <iostream>
#include <chrono>

static constexpr double SR = 48000.0;
static int failures = 0;
#define CHECK(c, msg) do { if (!(c)) { std::cout << "  FALLO: " << msg << "\n"; ++failures; } } while (0)

static void setP (TapeHeadProcessor& p, const juce::String& id, float v)
{
    auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v));
}

// Procesa (L, R) en bloques; devuelve la salida izquierda y derecha
static std::pair<std::vector<float>, std::vector<float>> run (TapeHeadProcessor& p, const std::vector<float>& L, const std::vector<float>& R, int block = 256)
{
    p.prepareToPlay (SR, block);
    juce::AudioBuffer<float> buf (2, block); juce::MidiBuffer mb;
    std::vector<float> oL (L.size()), oR (L.size());
    for (size_t pos = 0; pos < L.size(); pos += (size_t) block)
    {
        const int len = (int) std::min<size_t> ((size_t) block, L.size() - pos);
        buf.setSize (2, len, false, false, true);
        for (int i = 0; i < len; ++i) { buf.setSample (0, i, L[pos + (size_t) i]); buf.setSample (1, i, R[pos + (size_t) i]); }
        p.processBlock (buf, mb);
        for (int i = 0; i < len; ++i) { oL[pos + (size_t) i] = buf.getSample (0, i); oR[pos + (size_t) i] = buf.getSample (1, i); }
    }
    return { oL, oR };
}
static double rmsDb (const std::vector<float>& x, size_t from = 0)
{
    double e = 0; for (size_t i = from; i < x.size(); ++i) e += (double) x[i] * x[i];
    return 10.0 * std::log10 (e / (double) (x.size() - from) + 1e-20);
}
// material: sierras con caída + bombo + ruido, a −14 dBFS RMS (como tests/calibrate.js)
static std::vector<float> music (double seconds)
{
    const size_t N = (size_t) (SR * seconds); std::vector<float> x (N, 0.0f); uint32_t s = 7;
    auto rnd = [&s] { s = s * 1664525u + 1013904223u; return (float) s / 4294967296.0f; };
    const float notes[] = { 110, 146.8f, 164.8f, 220, 196, 130.8f, 82.4f, 98 };
    for (int b = 0; b < (int) (seconds * 4); ++b)
    {
        const size_t t0 = (size_t) (b * SR / 4); const float f = notes[b % 8];
        for (size_t i = 0; i < (size_t) (SR / 4) && t0 + i < N; ++i)
        {
            const float t = (float) i / (float) SR, env = std::exp (-t * 5);
            float v = 0.5f * (2 * std::fmod (f * t, 1.0f) - 1) * env + 0.25f * (2 * std::fmod (2 * f * t, 1.0f) - 1) * env;
            if (b % 2 == 0) v += 0.9f * std::sin (6.2831853f * (50 + 80 * std::exp (-t * 30)) * t) * std::exp (-t * 12);
            if (b % 4 == 2) v += 0.5f * (rnd() * 2 - 1) * std::exp (-t * 25);
            x[t0 + i] += v + 0.08f * (rnd() * 2 - 1) * std::exp (-t * 60);
        }
    }
    double e = 0; for (auto v : x) e += v * v;
    const float g = (float) (std::pow (10.0, -14.0 / 20) / std::sqrt (e / (double) N));
    for (auto& v : x) v *= g;
    return x;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    std::cout << "== Latencia y mezcla ==\n";
    {
        TapeHeadProcessor p; setP (p, ids::mix, 0.0f);
        std::vector<float> imp (4096, 0.0f); imp[100] = 0.5f;
        auto [l, r] = run (p, imp, imp);
        int at = -1; for (int i = 0; i < 4096; ++i) if (std::abs (l[(size_t) i]) > 0.25f) { at = i; break; }
        std::cout << "  latencia declarada " << p.getLatencySamples() << ", impulso seco en " << at - 100 << "\n";
        CHECK (p.getLatencySamples() == 32 && at - 100 == 32 && std::abs (l[132] - 0.5f) < 1e-4f, "el seco va retrasado exactamente la latencia");
    }
    {   // con Mix 100 % y drive 0 la señal húmeda está alineada con la seca (correlación máxima en 0)
        TapeHeadProcessor p; setP (p, ids::drive, 0.0f);
        std::vector<float> x (SR); for (size_t i = 0; i < x.size(); ++i) x[i] = 0.3f * std::sin (6.2831853f * 440.0f * (float) i / (float) SR);
        auto [l, r] = run (p, x, x);
        int best = 0; double bc = -1;
        for (int lag = 20; lag <= 44; ++lag) { double c = 0; for (size_t i = 10000; i < 40000; ++i) c += (double) l[i] * x[i - (size_t) lag]; if (c > bc) { bc = c; best = lag; } }
        std::cout << "  alineación húmeda: " << best << " muestras (los filtros de la consola giran algo la fase)\n";
        CHECK (std::abs (best - 32) <= 3, "la señal procesada no está alineada con la latencia");
    }

    std::cout << "== Auto-Gain (material a -14 dBFS RMS) ==\n";
    const auto mus = music (6);
    const double inDb = rmsDb (mus);
    for (float d : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
    {
        TapeHeadProcessor on, off;
        setP (on, ids::drive, d); setP (off, ids::drive, d); setP (off, ids::autoGain, 0.0f);
        auto a = run (on, mus, mus).first, b = run (off, mus, mus).first;
        const double da = rmsDb (a, 4800) - inDb, db = rmsDb (b, 4800) - inDb;
        std::cout << "  drive " << d * 10 << ": con Auto-Gain " << da << " dB · sin " << db << " dB\n";
        CHECK (std::abs (da) < 3.0, "Auto-Gain fuera de ±3 dB (RMS)");
        CHECK (d < 0.5f || db > 6.0, "sin Auto-Gain el drive alto debería sonar más fuerte");
    }

    std::cout << "== Estabilidad ==\n";
    {
        TapeHeadProcessor p; setP (p, ids::drive, 1.0f); setP (p, ids::inputDb, 18.0f); setP (p, ids::autoGain, 0.0f); setP (p, ids::outputDb, 18.0f);
        std::vector<float> x (SR * 2); uint32_t s = 1;
        for (size_t i = 0; i < x.size(); ++i) { s = s * 1103515245u + 12345u; x[i] = i < SR ? ((float) s / 4294967296.0f * 2 - 1) : 1.0f; }   // ruido y luego continua
        auto [l, r] = run (p, x, x, 333);
        int nan = 0; float pk = 0; for (auto v : l) { if (! std::isfinite (v)) ++nan; pk = std::max (pk, std::abs (v)); }
        double tail = 0; for (size_t i = x.size() - 4800; i < x.size(); ++i) tail += std::abs (l[i]);
        std::cout << "  extremo: pico " << pk << ", NaN " << nan << ", media final con continua " << tail / 4800 << "\n";
        CHECK (nan == 0 && pk < 20.0f, "estabilidad con señal extrema"); CHECK (tail / 4800 < 0.05, "la continua no se bloquea");
    }
    {   // mover el drive en marcha: sin saltos raros
        TapeHeadProcessor p; p.prepareToPlay (SR, 128);
        juce::AudioBuffer<float> buf (2, 128); juce::MidiBuffer mb; float maxJump = 0, prev = 0; int nan = 0;
        for (int b = 0; b < 750; ++b)
        {
            setP (p, ids::drive, (float) (b % 150) / 149.0f); setP (p, ids::bass, 10.0f * std::sin ((float) b * 0.05f)); setP (p, ids::treble, -10.0f * std::cos ((float) b * 0.04f));
            for (int i = 0; i < 128; ++i) { const float v = 0.3f * std::sin (6.2831853f * 220.0f * (float) (b * 128 + i) / (float) SR); buf.setSample (0, i, v); buf.setSample (1, i, v); }
            p.processBlock (buf, mb);
            for (int i = 0; i < 128; ++i) { const float v = buf.getSample (0, i); if (! std::isfinite (v)) ++nan; maxJump = std::max (maxJump, std::abs (v - prev)); prev = v; }
        }
        std::cout << "  automatización: salto máximo entre muestras " << maxJump << "\n";
        CHECK (nan == 0 && maxJump < 0.5f, "clics al mover drive/tono");
    }
    {   // mono
        TapeHeadProcessor p;
        juce::AudioProcessor::BusesLayout lay; lay.inputBuses.add (juce::AudioChannelSet::mono()); lay.outputBuses.add (juce::AudioChannelSet::mono());
        CHECK (p.setBusesLayout (lay), "mono → mono");
        p.prepareToPlay (SR, 256); juce::AudioBuffer<float> buf (1, 256); juce::MidiBuffer mb;
        for (int i = 0; i < 256; ++i) buf.setSample (0, i, 0.2f);
        p.processBlock (buf, mb); CHECK (std::isfinite (buf.getSample (0, 255)), "mono procesa");
    }

    std::cout << "== Rendimiento ==\n";
    {
        TapeHeadProcessor p; setP (p, ids::drive, 0.8f);
        std::vector<float> x (SR * 10); for (size_t i = 0; i < x.size(); ++i) x[i] = mus[i % mus.size()];
        const auto t0 = std::chrono::steady_clock::now();
        run (p, x, x, 512);
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        std::cout << "  10 s estéreo a 48 kHz en " << secs << " s (" << 10.0 / secs << "× tiempo real, ~" << 100.0 * secs / 10.0 << " % de un núcleo)\n";
        CHECK (secs < 2.0, "demasiado lento");
    }

    std::cout << "== Estado y presets ==\n";
    {
        TapeHeadProcessor a; a.loadFactory (17);  // Bajo grunge
        juce::MemoryBlock mb; a.getStateInformation (mb);
        TapeHeadProcessor c; c.setStateInformation (mb.getData(), (int) mb.getSize());
        const float d = c.apvts.getRawParameterValue (ids::drive)->load(), m = c.apvts.getRawParameterValue (ids::mix)->load(), bs = c.apvts.getRawParameterValue (ids::bass)->load();
        std::cout << "  restaurado: " << c.currentPresetName << " drive " << d << " graves " << bs << " mix " << m << " · presets " << a.getNumPrograms() << "\n";
        CHECK (std::abs (d - 0.64f) < 1e-3f && std::abs (bs - 5.0f) < 1e-3f && std::abs (m - 0.85f) < 1e-3f && c.currentPresetName == u8 ("Bajo grunge"), "restaurar estado");
        CHECK (a.getNumPrograms() == 25, "número de presets");
        a.loadFactory (1); CHECK (std::abs (a.apvts.getRawParameterValue (ids::mix)->load() - 1.0f) < 1e-6f, "un preset sin mix vuelve a 100 %");
    }

    std::cout << "== Editor ==\n";
    {
        TapeHeadProcessor p; p.prepareToPlay (SR, 256); p.loadFactory (12);   // Grunge vaciado
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        auto& te = dynamic_cast<TapeHeadEditor&> (*ed);
        te.stepPreset (1); CHECK (p.currentPresetName == u8 ("Noise muro"), "▶▶ pasa al siguiente preset");
        te.stepPreset (-1); te.stepPreset (-1); CHECK (p.currentPresetName == u8 ("Grunge crudo"), "◀◀ vuelve");
        p.loadFactory (12);
        // un poco de audio para que se mueva la aguja y giren las bobinas
        auto x = music (1.0); run (p, x, x);
        for (int i = 0; i < 10; ++i) { juce::Thread::sleep (35); te.tickForTests(); }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
        juce::File out ("/tmp/tapehead_editor.png"); out.deleteFile();
        juce::FileOutputStream fos (out); juce::PNGImageFormat png; png.writeImageToStream (img, fos);
        std::cout << "  " << out.getFullPathName() << " " << img.getWidth() << "x" << img.getHeight() << " · VU " << te.vuLevel() << "\n";
    }

    std::cout << "== Aviso de versión nueva ==\n";
    {
        using namespace shupdate;
        CHECK (newer ("1.0.9", "1.0.10") && ! newer ("1.1.0", "1.1.0"), "comparar versiones");
        const auto rel = juce::JSON::parse (R"([{"tag_name":"vst-v9.9.9","html_url":"https://github.com/r/releases/tag/vst-v9.9.9"},{"tag_name":"vst-v1.0.0","html_url":"vieja"}])");
        auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("sh-update-test-th"); tmp.deleteRecursively(); tmp.createDirectory();
        auto fake = tmp.getChildFile ("releases.json"); fake.replaceWithText (juce::JSON::toString (rel));
        setenv ("SH_UPDATE_SETTINGS", tmp.getFullPathName().toRawUTF8(), 1); setenv ("SH_UPDATE_FAKE", fake.getFullPathName().toRawUTF8(), 1);
        auto open = [&] (bool snap) {
            TapeHeadProcessor p; p.prepareToPlay (SR, 256);
            std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
            auto& up = dynamic_cast<TapeHeadEditor&> (*ed).update; up.waitForCheck (5000);
            const bool shown = up.hasUpdate();
            if (snap) { auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f); juce::File o ("/tmp/tapehead_editor_update.png"); o.deleteFile(); juce::FileOutputStream fos (o); juce::PNGImageFormat png; png.writeImageToStream (img, fos); }
            if (shown && ! snap) up.ignoreThisVersion();
            return shown;
        };
        CHECK (open (true), "con versión nueva aparece el aviso");
        open (false); CHECK (! open (false), "tras ✕ no vuelve a avisar");
        unsetenv ("SH_UPDATE_FAKE"); unsetenv ("SH_UPDATE_SETTINGS"); tmp.deleteRecursively();
    }
    std::cout << (failures ? "FALLOS: " + std::to_string (failures) : std::string ("TODO OK")) << "\n";
    return failures ? 1 : 0;
}
