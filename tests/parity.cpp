// Paridad JS ↔ C++: lee tests/parity_in.f32 (mono float32) y escribe la salida de TapeDSP.h para varios ajustes.
// g++ -O2 -std=c++17 -I vst/Source tests/parity.cpp -o /tmp/parity && node tests/parity.js
#include "dsp/TapeDSP.h"
#include <cstdio>
#include <vector>
int main (int argc, char** argv)
{
    if (argc < 3) return 1;
    FILE* f = std::fopen (argv[1], "rb"); if (! f) return 1;
    std::vector<float> x; float v; while (std::fread (&v, 4, 1, f) == 1) x.push_back (v); std::fclose (f);
    FILE* o = std::fopen (argv[2], "wb");
    const double sr = 48000, drives[] = { 0.0, 0.35, 0.7, 1.0 }, bass[] = { 0.0, -6, 5, 8 }, treble[] = { 0.0, 4, -7, 9 };
    for (int s = 0; s < 4; ++s)
    {
        tape::Channel ch; ch.prepare (sr); ch.setParams (tape::map (drives[s], bass[s], treble[s], sr));
        const double ag = tape::autoGain (drives[s]);
        for (size_t i = 0; i < x.size() + tape::kLatency; ++i)
        {
            const double in = i < x.size() ? x[i] : 0.0, wet = ch.tick (in) * ag;
            ch.delayDry (in);
            if (i >= (size_t) tape::kLatency) { const float y = (float) wet; std::fwrite (&y, 4, 1, o); }
        }
    }
    std::fclose (o);
    return 0;
}
