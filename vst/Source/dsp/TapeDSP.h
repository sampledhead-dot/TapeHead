// TapeHead · motor de sonido (C++). Mismo diseño y mismos números que dsp/tapehead.js (la referencia).
// Previo de consola de multipista de casete de los 90: op-amps en cascada (TRIM → fader → bus), realce de medios,
// sobremuestreo ×4 con FIR de fase lineal (latencia fija de 32 muestras) y compensación de nivel (Auto-Gain).
// Sin dependencias de JUCE: lo usan el plugin y la prueba de paridad (tests/parity.cpp).
#pragma once
#include <cmath>
#include <array>
#include <algorithm>

namespace tape
{
constexpr int kOS = 4, kTaps = 129, kLatency = 32;
constexpr double kPi = 3.14159265358979323846;

inline double db2lin (double d) { return std::pow (10.0, d / 20.0); }
inline double clampd (double x, double a, double b) { return x < a ? a : (x > b ? b : x); }

struct Map { double g1, g2, bias2, lsPre, pkPre, lsPost, lpf, bass, treble; };

inline Map map (double d, double bass, double treble, double sr)
{
    d = clampd (d, 0, 1); bass = clampd (bass, -10, 10); treble = clampd (treble, -10, 10);
    const double push = d > 0.4 ? (d - 0.4) / 0.6 : 0.0;
    Map m;
    m.g1 = db2lin (40.0 * std::pow (d, 1.3));
    m.g2 = db2lin (16.0 * std::pow (push, 1.5));
    m.bias2 = 0.08 * d;
    m.lsPre = -(1.0 + 5.0 * d);
    m.pkPre = 1.5 + 4.5 * d;
    m.lsPost = 0.5 + 2.5 * d;
    m.lpf = std::min (16000.0 - 8000.0 * d, 0.45 * sr);
    m.bass = bass;       // EQ de canal: estante de graves a 120 Hz…
    m.treble = treble;   // …y de agudos a 5 kHz (después del previo, antes del fader)
    return m;
}

// Auto-Gain: dB que se restan para drive = 0, 0.05 … 1 (tests/calibrate.js). Igual que T.AUTO_DB.
inline double autoGain (double d)
{
    static constexpr double t[21] = { 1.95, 2.75, 3.91, 5.25, 6.70, 8.20, 9.71, 11.15, 12.49, 13.98, 15.46, 16.79, 17.92,
                                      18.77, 19.33, 19.67, 19.89, 20.02, 20.09, 20.11, 20.11 };
    d = clampd (d, 0, 1) * 20.0;
    const int i = std::min (19, (int) std::floor (d));
    const double f = d - i;
    return db2lin (-(t[i] * (1 - f) + t[i + 1] * f));
}

inline const std::array<double, kTaps>& fir()
{
    static const std::array<double, kTaps> h = []
    {
        auto bessel0 = [] (double x) { double s = 1, t = 1; for (int k = 1; k < 40; ++k) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; } return s; };
        std::array<double, kTaps> a {};
        const double M = kTaps - 1, fc = 0.5 / kOS * 0.96, beta = 8;
        double s = 0;
        for (int k = 0; k < kTaps; ++k)
        {
            const double n = k - M / 2, sinc = n == 0 ? 2 * fc : std::sin (2 * kPi * fc * n) / (kPi * n);
            const double r = 2 * k / M - 1, w = bessel0 (beta * std::sqrt (std::max (0.0, 1 - r * r))) / bessel0 (beta);
            a[(size_t) k] = sinc * w; s += a[(size_t) k];
        }
        for (auto& v : a) v /= s;
        return a;
    }();
    return h;
}

struct Biquad
{
    enum Type { peak, lowshelf, highshelf, lowpass };
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    void set (Type type, double f, double q, double gainDb, double fs)
    {
        const double A = std::pow (10.0, gainDb / 40), w = 2 * kPi * f / fs, cw = std::cos (w), sw = std::sin (w), al = sw / (2 * q);
        double B0, B1, B2, A0, A1, A2;
        if (type == peak) { B0 = 1 + al * A; B1 = -2 * cw; B2 = 1 - al * A; A0 = 1 + al / A; A1 = -2 * cw; A2 = 1 - al / A; }
        else if (type == lowshelf)
        {
            const double sA = 2 * std::sqrt (A) * al;
            B0 = A * ((A + 1) - (A - 1) * cw + sA); B1 = 2 * A * ((A - 1) - (A + 1) * cw); B2 = A * ((A + 1) - (A - 1) * cw - sA);
            A0 = (A + 1) + (A - 1) * cw + sA; A1 = -2 * ((A - 1) + (A + 1) * cw); A2 = (A + 1) + (A - 1) * cw - sA;
        }
        else if (type == highshelf)
        {
            const double sA = 2 * std::sqrt (A) * al;
            B0 = A * ((A + 1) + (A - 1) * cw + sA); B1 = -2 * A * ((A - 1) + (A + 1) * cw); B2 = A * ((A + 1) + (A - 1) * cw - sA);
            A0 = (A + 1) - (A - 1) * cw + sA; A1 = 2 * ((A - 1) - (A + 1) * cw); A2 = (A + 1) - (A - 1) * cw - sA;
        }
        else { B0 = (1 - cw) / 2; B1 = 1 - cw; B2 = (1 - cw) / 2; A0 = 1 + al; A1 = -2 * cw; A2 = 1 - al; }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    inline double run (double x)
    {
        const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x; y2 = y1; y1 = y; return y;
    }
    void reset() { x1 = x2 = y1 = y2 = 0; }
};

struct OnePoleHP { double a = 0, x1 = 0, y1 = 0; void set (double f, double fs) { a = std::exp (-2 * kPi * f / fs); }
                   inline double run (double x) { const double y = a * (y1 + x - x1); x1 = x; y1 = y; return y; } void reset() { x1 = y1 = 0; } };
struct OnePoleLP { double a = 0, y1 = 0; void set (double f, double fs) { a = std::exp (-2 * kPi * f / fs); }
                   inline double run (double x) { y1 = x + a * (y1 - x); return y1; } void reset() { y1 = 0; } };

inline double knee (double v, double w)
{
    const double a = std::abs (v);
    double y;
    if (a <= 1 - w) y = a; else if (a >= 1 + w) y = 1; else { const double t = a - (1 - w); y = a - t * t / (4 * w); }
    return v < 0 ? -y : y;
}
inline double opamp (double v, double w, double c) { const double y = knee (v, w); return (y - c * y * y * y) / (1 - c); }

constexpr double W1 = 0.35, C1 = 0.12, B1 = 0.05;   // TRIM (entrada)
constexpr double W2 = 0.15, C2 = 0.05;              // fader
constexpr double W3 = 0.40, C3 = 0.08, R3 = 1.2;    // bus
constexpr double HEAD = 0.5;                        // 0 dBFS a −6 dB de los raíles

/** Un canal. prepare() una vez; setParams() cuando cambien drive/tono (sin reservar memoria). */
struct Channel
{
    double fs = 176400;
    std::array<double, 64> xh {}; int xi = 0;
    std::array<double, 256> ring {}; int ri = 0;
    std::array<double, kLatency + 1> dly {}; int di = 0;
    OnePoleHP hpIn, hpOut; OnePoleLP lp1;
    Biquad lsPre, pkPre, eqLo, eqHi, lsPost, lpf;
    Map m {};
    double s1off = 0, s2off = 0;

    void prepare (double sr)
    {
        fs = sr * kOS;
        hpIn.set (20, fs); hpOut.set (10, fs); lp1.set (20000, fs);
        s1off = opamp (B1, W1, C1);
        reset();
    }
    void reset()
    {
        xh.fill (0); ring.fill (0); dly.fill (0); xi = ri = di = 0;
        hpIn.reset(); hpOut.reset(); lp1.reset();
        for (auto* b : { &lsPre, &pkPre, &eqLo, &eqHi, &lsPost, &lpf }) b->reset();
    }
    void setParams (const Map& mm)
    {
        m = mm;
        lsPre.set (Biquad::lowshelf, 180, 0.707, m.lsPre, fs);
        pkPre.set (Biquad::peak, 1000, 0.8, m.pkPre, fs);
        lsPost.set (Biquad::lowshelf, 180, 0.707, m.lsPost, fs);
        lpf.set (Biquad::lowpass, m.lpf, 0.707, 0, fs);
        eqLo.set (Biquad::lowshelf, 120, 0.5, m.bass, fs);
        eqHi.set (Biquad::highshelf, 5000, 0.5, m.treble, fs);
        s2off = opamp (m.bias2, W2, C2);
    }
    inline double chain (double x)
    {
        x = hpIn.run (x);
        x = pkPre.run (lsPre.run (x));
        x = opamp (m.g1 * x + B1, W1, C1) - s1off;
        x = lp1.run (x);
        x = eqHi.run (eqLo.run (x));
        x = opamp (m.g2 * x + m.bias2, W2, C2) - s2off;
        x = lpf.run (lsPost.run (x));
        x = R3 * opamp (x / R3, W3, C3);
        return hpOut.run (x);
    }
    /** Una muestra base → salida húmeda retrasada kLatency muestras. */
    inline double tick (double x)
    {
        const auto& h = fir();
        xi = (xi + 1) & 63;
        xh[(size_t) xi] = x * HEAD;
        for (int p = 0; p < kOS; ++p)
        {
            double acc = 0;
            for (int j = 0; p + kOS * j < kTaps; ++j) acc += h[(size_t) (p + kOS * j)] * xh[(size_t) ((xi - j) & 63)];
            ri = (ri + 1) & 255;
            ring[(size_t) ri] = chain (acc * kOS);
        }
        const int base = ri - 3;
        double acc = 0;
        for (int k = 0; k < kTaps; ++k) acc += h[(size_t) k] * ring[(size_t) ((base - k) & 255)];
        return acc / HEAD;
    }
    inline double delayDry (double x)
    {
        constexpr int n = kLatency + 1;
        dly[(size_t) di] = x; di = (di + 1) % n; return dly[(size_t) di];
    }
};
} // namespace tape
