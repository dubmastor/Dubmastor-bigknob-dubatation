#pragma once
// Eco de fita, EQ, sirene e maximizer da estação Dub Knob. Independente do JUCE.

#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <complex>

namespace dub
{

static constexpr float kTwoPi = 6.28318530718f;

struct OnePole
{
    float z = 0.0f;
    inline float lp (float x, float a) { z += a * (x - z); return z; }
    static float coef (float hz, float fs) { return 1.0f - std::exp (-kTwoPi * hz / fs); }
};

//==============================================================================
// Eco de fita: mudança de tempo com "glide" de fita, saturação e filtro no loop,
// wow/flutter, ping-pong e envio suavizado (para o THROW não estalar).
struct EchoSettings
{
    float delaySec  = 0.375f;
    float feedback  = 0.55f;  // 0..1.2 (acima de 1 = autooscilação controlada)
    float toneHz    = 2500.0f;
    float wow       = 0.25f;  // 0..1
    float send      = 0.0f;   // 0..1
    bool  pingPong  = false;
    int   heads     = 1;      // bits: 1 = cabeçote 1 (1x), 2 = cabeçote 2 (2x), 4 = cabeçote 3 (3x)
};

class TapeEcho
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        size = (int) (6.0f * fs) + 8;
        for (auto& b : buf) b.assign ((size_t) size, 0.0f);
        glideCoef = std::exp (-1.0f / (0.25f * fs));  // glide de ~250 ms ao mudar o tempo
        sendCoef  = std::exp (-1.0f / (0.01f * fs));  // 10 ms
        hpA = OnePole::coef (110.0f, fs);
        reset();
    }

    void reset()
    {
        for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f);
        for (auto& f : toneLP) f.z = 0;
        for (auto& f : hpLP)   f.z = 0;
        curDelay = std::clamp (s.delaySec * fs, 4.0f, (float) size - 8.0f);
        sendSm = s.send; wowPh = flutPh = 0.0f; w = 0;
    }

    void setSettings (const EchoSettings& e) { s = e; }

    // in: entrada (L,R); out: retorno molhado (L,R)
    inline void processSample (float inL, float inR, float& outL, float& outR)
    {
        const float target = std::clamp (s.delaySec * fs, 4.0f, ((float) size - 8.0f) / 3.0f);
        curDelay = target + glideCoef * (curDelay - target);
        sendSm   = s.send + sendCoef * (sendSm - s.send);

        wowPh  += kTwoPi * 0.55f / fs; if (wowPh  > kTwoPi) wowPh  -= kTwoPi;
        flutPh += kTwoPi * 6.3f  / fs; if (flutPh > kTwoPi) flutPh -= kTwoPi;
        const float mod = s.wow * (0.0018f * fs * std::sin (wowPh) + 0.00015f * fs * std::sin (flutPh));

        // cabeçotes estilo Space Echo: 1x, 2x e 3x o tempo base
        float y[2] = { 0.0f, 0.0f };
        int active = 0;
        for (int h = 0; h < 3; ++h)
        {
            if (! (s.heads & (1 << h))) continue;
            ++active;
            const float d = std::min (curDelay * (float) (h + 1), (float) size - 8.0f);
            for (int ch = 0; ch < 2; ++ch)
                y[ch] += read (ch, d + mod + (ch ? 0.0007f * fs * s.wow : 0.0f));
        }
        if (active == 0) { active = 1; for (int ch = 0; ch < 2; ++ch) y[ch] = read (ch, curDelay + mod); }
        const float fbNorm = 1.0f / (float) active;
        const float outNorm = 1.0f / std::sqrt ((float) active);

        const float toneA = OnePole::coef (s.toneHz, fs);
        const float g = s.feedback;
        float fb[2];
        for (int ch = 0; ch < 2; ++ch)
        {
            const float src = (s.pingPong ? y[1 - ch] : y[ch]) * fbNorm;
            float f = toneLP[ch].lp (src, toneA);
            f -= hpLP[ch].lp (f, hpA);                  // tira o grave acumulado
            fb[ch] = 0.9f * std::tanh (f * g / 0.9f);   // saturação de fita
        }

        float in0 = inL * sendSm, in1 = inR * sendSm;
        if (s.pingPong) { in0 = 0.5f * (in0 + in1); in1 = 0.0f; }

        buf[0][(size_t) w] = in0 + fb[0];
        buf[1][(size_t) w] = in1 + fb[1];
        if (++w >= size) w = 0;

        outL = y[0] * outNorm; outR = y[1] * outNorm;
    }

private:
    inline float read (int ch, float d) const
    {
        float pos = (float) w - d;
        while (pos < 0) pos += (float) size;
        const int i1 = (int) pos; const float t = pos - (float) i1;
        auto at = [&](int i) { i %= size; if (i < 0) i += size; return buf[ch][(size_t) i]; };
        const float y0 = at (i1 - 1), y1 = at (i1), y2 = at (i1 + 1), y3 = at (i1 + 2);
        // Hermite cúbica
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }

    float fs = 44100.0f, curDelay = 1000.0f, glideCoef = 0, sendCoef = 0, sendSm = 0, hpA = 0;
    float wowPh = 0, flutPh = 0;
    int size = 1, w = 0;
    EchoSettings s;
    std::array<std::vector<float>, 2> buf;
    std::array<OnePole, 2> toneLP, hpLP;
};

//==============================================================================
// Sirene de dub: oscilador com pitch modulado por LFO (forma selecionável),
// LFO reinicia ao disparar, envelope curto e delay próprio com realimentação.
struct SirenSettings
{
    float pitchHz  = 440.0f;
    float rateHz   = 2.0f;
    float depth    = 0.5f;   // 0..1 -> até ±2 oitavas
    int   wave     = 4;      // 0 seno, 1 triângulo, 2 quadrada, 3 dente subindo, 4 dente descendo
    float delaySec = 0.35f;
    float feedback = 0.5f;   // 0..0.95
    float level    = 0.5f;
    bool  gate     = false;
};

class DubSiren
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        line.assign ((size_t) (2.1f * fs) + 4, 0.0f);
        atk = 1.0f - std::exp (-1.0f / (0.004f * fs));
        rel = 1.0f - std::exp (-1.0f / (0.09f * fs));
        dSmooth = std::exp (-1.0f / (0.15f * fs));
        toneA = OnePole::coef (3200.0f, fs);
        reset();
    }

    void reset()
    {
        std::fill (line.begin(), line.end(), 0.0f);
        w = 0; env = 0; oscPh = 0; lfoPh = 0; lastGate = false; tone.z = 0;
        curDelay = s.delaySec * fs;
    }

    void setSettings (const SirenSettings& n)
    {
        if (n.gate && ! lastGate) lfoPh = 0.0f; // reinicia o LFO a cada disparo
        lastGate = n.gate;
        s = n;
    }

    inline float processSample()
    {
        env += (s.gate ? atk : rel) * ((s.gate ? 1.0f : 0.0f) - env);

        float dry = 0.0f;
        if (env > 1e-5f)
        {
            lfoPh += s.rateHz / fs; if (lfoPh >= 1.0f) lfoPh -= 1.0f;
            float lfo;
            switch (s.wave)
            {
                case 0:  lfo = std::sin (kTwoPi * lfoPh); break;
                case 1:  lfo = 1.0f - 4.0f * std::fabs (lfoPh - 0.5f); break;
                case 2:  lfo = lfoPh < 0.5f ? 1.0f : -1.0f; break;
                case 3:  lfo = 2.0f * lfoPh - 1.0f; break;
                default: lfo = 1.0f - 2.0f * lfoPh; break;
            }
            const float f = std::min (s.pitchHz * std::exp2 (2.0f * s.depth * lfo), 0.45f * fs);
            oscPh += f / fs; if (oscPh >= 1.0f) oscPh -= 1.0f;
            // quadrada arredondada: timbre de sirene analógica
            dry = std::tanh (3.0f * std::sin (kTwoPi * oscPh)) * 0.6f * env;
        }

        // delay próprio
        const float target = std::clamp (s.delaySec * fs, 8.0f, (float) line.size() - 4.0f);
        curDelay = target + dSmooth * (curDelay - target);
        float pos = (float) w - curDelay;
        if (pos < 0) pos += (float) line.size();
        const int i0 = (int) pos; const float t = pos - (float) i0;
        const int i1 = (i0 + 1) % (int) line.size();
        const float wet = line[(size_t) i0] + t * (line[(size_t) i1] - line[(size_t) i0]);
        const float fb = std::tanh (tone.lp (wet, toneA) * std::min (s.feedback, 0.95f));
        line[(size_t) w] = dry + fb;
        if (++w >= (int) line.size()) w = 0;

        return (dry + wet) * s.level;
    }

private:
    float fs = 44100.0f, atk = 0, rel = 0, env = 0, oscPh = 0, lfoPh = 0;
    float dSmooth = 0, curDelay = 0, toneA = 0;
    bool lastGate = false;
    int w = 0;
    std::vector<float> line;
    OnePole tone;
    SirenSettings s;
};

//==============================================================================
// EQ de retorno do eco: HP 12 dB/oit, sino de médio e LP 12 dB/oit.
// Filtros SVF (Simper) — podem ser girados ao vivo sem estalos.
struct EQSettings
{
    float hpHz   = 20.0f;
    float lpHz   = 20000.0f;
    float midHz  = 1000.0f;
    float midDb  = 0.0f;
    static constexpr float kMidQ = 1.0f;
};

class ReturnEQ
{
public:
    void prepare (double sampleRate) { fs = (float) sampleRate; reset(); }
    void reset() { for (auto& c : st) for (auto& b : c) b = {}; }

    void setSettings (const EQSettings& e)
    {
        s = e;
        hp = make (std::min (e.hpHz, 0.45f * fs), 0.7071f, 1, 0);
        lp = make (std::min (e.lpHz, 0.45f * fs), 0.7071f, 2, 0);
        mid = make (std::min (e.midHz, 0.45f * fs), EQSettings::kMidQ, 3, e.midDb);
        hpOn = e.hpHz > 21.0f;
        lpOn = e.lpHz < 19500.0f;
        midOn = std::fabs (e.midDb) > 0.05f;
    }

    inline void processSample (float& l, float& r)
    {
        float* x[2] = { &l, &r };
        for (int ch = 0; ch < 2; ++ch)
        {
            float v = *x[ch];
            if (hpOn)  v = tick (st[ch][0], hp,  v);
            if (midOn) v = tick (st[ch][1], mid, v);
            if (lpOn)  v = tick (st[ch][2], lp,  v);
            *x[ch] = v;
        }
    }

    // resposta em dB (para o visor), exata para o projeto bilinear
    static float magnitudeDb (float f, const EQSettings& e, float fs)
    {
        using C = std::complex<double>;
        auto w = [&] (float fc) { return std::tan (3.14159265 * std::min (fc, 0.45f * fs) / fs); };
        const double wf = std::tan (3.14159265 * std::min (f, 0.49f * fs) / fs);
        double mag = 1.0;
        if (e.hpHz > 21.0f)
        {
            const C s (0.0, wf / w (e.hpHz));
            mag *= std::abs (s * s / (s * s + s / 0.7071 + 1.0));
        }
        if (e.lpHz < 19500.0f)
        {
            const C s (0.0, wf / w (e.lpHz));
            mag *= std::abs (1.0 / (s * s + s / 0.7071 + 1.0));
        }
        if (std::fabs (e.midDb) > 0.05f)
        {
            const double A = std::pow (10.0, e.midDb / 40.0), Q = EQSettings::kMidQ;
            const C s (0.0, wf / w (e.midHz));
            mag *= std::abs ((s * s + s * (A / Q) + 1.0) / (s * s + s / (A * Q) + 1.0));
        }
        return (float) (20.0 * std::log10 (std::max (mag, 1e-6)));
    }

private:
    struct Coef { float a1 = 0, a2 = 0, a3 = 0, m0 = 0, m1 = 0, m2 = 0; };
    struct State { float ic1 = 0, ic2 = 0; };

    Coef make (float fc, float q, int type, float db) const
    {
        Coef c;
        const float g = std::tan (3.14159265f * fc / fs);
        float k = 1.0f / q;
        if (type == 3) // sino
        {
            const float A = std::pow (10.0f, db / 40.0f);
            k = 1.0f / (q * A);
            c.m0 = 1.0f; c.m1 = k * (A * A - 1.0f); c.m2 = 0.0f;
        }
        else if (type == 1) { c.m0 = 1.0f; c.m1 = -k; c.m2 = -1.0f; } // HP
        else                { c.m0 = 0.0f; c.m1 = 0.0f; c.m2 = 1.0f; } // LP
        c.a1 = 1.0f / (1.0f + g * (g + k));
        c.a2 = g * c.a1;
        c.a3 = g * c.a2;
        return c;
    }

    static inline float tick (State& st, const Coef& c, float v0)
    {
        const float v3 = v0 - st.ic2;
        const float v1 = c.a1 * st.ic1 + c.a2 * v3;
        const float v2 = st.ic2 + c.a2 * st.ic1 + c.a3 * v3;
        st.ic1 = 2.0f * v1 - st.ic1;
        st.ic2 = 2.0f * v2 - st.ic2;
        return c.m0 * v0 + c.m1 * v1 + c.m2 * v2;
    }

    float fs = 44100.0f;
    EQSettings s;
    Coef hp, lp, mid;
    bool hpOn = false, lpOn = false, midOn = false;
    std::array<std::array<State, 3>, 2> st {};
};

//==============================================================================
// MAXIMIZER (v2.0): compressor de "cola" suave + limiter brickwall com lookahead.
// Um único controle (PUSH) empurra o volume; o teto (CEILING) nunca é ultrapassado.
struct MaximizerSettings
{
    float pushDb    = 0.0f;   // 0..20 dB de ganho
    float ceilingDb = -0.3f;  // -3..0 dBFS
    bool  on        = true;
};

class Maximizer
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        N = std::max (1, (int) std::lround (0.0015 * sampleRate));     // 1.5 ms de lookahead
        for (auto& d : delay) d.assign ((size_t) N, 0.0f);
        reqRing.assign ((size_t) N + 1, 1.0f);
        boxRing.assign ((size_t) N, 1.0f);
        relA     = 1.0f - std::exp (-1.0f / (0.080f * fs));             // release do limiter
        cAtk     = 1.0f - std::exp (-1.0f / (0.005f * fs));             // compressor
        cRel     = 1.0f - std::exp (-1.0f / (0.150f * fs));
        smoothA  = 1.0f - std::exp (-1.0f / (0.020f * fs));
        reset();
    }

    void reset()
    {
        for (auto& d : delay) std::fill (d.begin(), d.end(), 0.0f);
        std::fill (reqRing.begin(), reqRing.end(), 1.0f);
        std::fill (boxRing.begin(), boxRing.end(), 1.0f);
        boxSum = (double) N; env = 1.0f; compEnv = 0.0f; di = ri = bi = 0;
        pushLin = std::pow (10.0f, s.pushDb / 20.0f);
        lastGrDb = 0.0f;
    }

    void setSettings (const MaximizerSettings& m) { s = m; }
    int  getLatency() const { return N; }
    float getLastGainReductionDb() const { return lastGrDb; } // <= 0

    inline void processSample (float& l, float& r)
    {
        const float ceil = std::pow (10.0f, s.ceilingDb / 20.0f);
        pushLin += smoothA * ((s.on ? std::pow (10.0f, s.pushDb / 20.0f) : 1.0f) - pushLin);

        float xl = l, xr = r, compGrDb = 0.0f;
        if (s.on)
        {
            xl *= pushLin; xr *= pushLin;

            // compressor de cola: 1.6:1, joelho de 6 dB, limiar 6 dB abaixo do teto
            const float pk = std::max (std::fabs (xl), std::fabs (xr));
            compEnv += (pk > compEnv ? cAtk : cRel) * (pk - compEnv);
            const float lvl = 20.0f * std::log10 (std::max (compEnv, 1e-9f));
            const float over = lvl - (s.ceilingDb - 6.0f), W = 6.0f, slope = 1.0f / 1.6f - 1.0f;
            if (2.0f * over > W)                    compGrDb = slope * over;
            else if (2.0f * std::fabs (over) <= W)  compGrDb = slope * (over + W * 0.5f) * (over + W * 0.5f) / (2.0f * W);
            const float cg = std::pow (10.0f, compGrDb / 20.0f);
            xl *= cg; xr *= cg;
        }

        // limiter: ganho necessário, mínimo na janela de lookahead, release, média móvel
        const float peak = std::max (std::fabs (xl), std::fabs (xr));
        const float req  = (s.on && peak > ceil) ? ceil / peak : 1.0f;
        reqRing[(size_t) ri] = req;
        if (++ri > N) ri = 0;
        float wmin = 1.0f;
        for (float v : reqRing) wmin = std::min (wmin, v);
        env = wmin < env ? wmin : env + (wmin - env) * relA;
        boxSum += (double) env - (double) boxRing[(size_t) bi];
        boxRing[(size_t) bi] = env;
        if (++bi >= N) bi = 0;
        const float g = (float) (boxSum / (double) N);

        // atraso de N amostras (latência informada ao Cubase)
        const float dl = delay[0][(size_t) di], dr = delay[1][(size_t) di];
        delay[0][(size_t) di] = xl; delay[1][(size_t) di] = xr;
        if (++di >= N) di = 0;
        // sai a amostra de N passos atrás; cada valor da média móvel veio de uma janela
        // de mínimo que contém essa amostra, então o ganho final nunca deixa passar do teto.

        if (s.on)
        {
            l = std::clamp (dl * g, -ceil, ceil);   // trava de segurança
            r = std::clamp (dr * g, -ceil, ceil);
            lastGrDb = compGrDb + 20.0f * std::log10 (std::max (g, 1e-6f));
        }
        else { l = dl; r = dr; lastGrDb = 0.0f; }
    }

private:
    float fs = 44100.0f, relA = 0, cAtk = 0, cRel = 0, smoothA = 0;
    float env = 1.0f, compEnv = 0.0f, pushLin = 1.0f, lastGrDb = 0.0f;
    double boxSum = 0.0;
    int N = 64, di = 0, ri = 0, bi = 0;
    std::array<std::vector<float>, 2> delay;
    std::vector<float> reqRing, boxRing;
    MaximizerSettings s;
};

//==============================================================================
// Loudness de curto prazo (ITU-R BS.1770, janela de 3 s): filtro K + média quadrática.
class LoudnessMeter
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        // estágio 1: shelf de agudos
        {
            const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
            const double K = std::tan (3.141592653589793 * f0 / fs), Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
            const double a0 = 1.0 + K / Q + K * K;
            s1 = { (Vh + Vb * K / Q + K * K) / a0, 2.0 * (K * K - Vh) / a0, (Vh - Vb * K / Q + K * K) / a0,
                   2.0 * (K * K - 1.0) / a0, (1.0 - K / Q + K * K) / a0 };
        }
        // estágio 2: passa-altas RLB
        {
            const double f0 = 38.13547087602444, Q = 0.5003270373238773;
            const double K = std::tan (3.141592653589793 * f0 / fs), a0 = 1.0 + K / Q + K * K;
            s2 = { 1.0, -2.0, 1.0, 2.0 * (K * K - 1.0) / a0, (1.0 - K / Q + K * K) / a0 };
        }
        blockLen = (int) (0.1 * fs);
        reset();
    }

    void reset()
    {
        for (auto& c : z) c = {};
        blocks.fill (0.0); acc = 0.0; count = 0; bIdx = 0; filled = 0;
    }

    inline void processSample (float l, float r)
    {
        const double yl = biquad (biquad (l, s1, z[0]), s2, z[1]);
        const double yr = biquad (biquad (r, s1, z[2]), s2, z[3]);
        acc += yl * yl + yr * yr;
        if (++count >= blockLen)
        {
            blocks[(size_t) bIdx] = acc / (double) count;
            bIdx = (bIdx + 1) % (int) blocks.size();
            filled = std::min (filled + 1, (int) blocks.size());
            acc = 0.0; count = 0;
        }
    }

    float getShortTermLufs() const
    {
        if (filled == 0) return -70.0f;
        double sum = 0.0;
        for (int i = 0; i < filled; ++i) sum += blocks[(size_t) i];
        const double ms = sum / (double) filled;
        return ms > 1e-12 ? (float) (-0.691 + 10.0 * std::log10 (ms)) : -70.0f;
    }

private:
    struct BQ { double b0, b1, b2, a1, a2; };
    struct Z  { double z1 = 0, z2 = 0; };
    static inline double biquad (double x, const BQ& c, Z& st)
    {
        const double y = c.b0 * x + st.z1;
        st.z1 = c.b1 * x - c.a1 * y + st.z2;
        st.z2 = c.b2 * x - c.a2 * y;
        return y;
    }
    double fs = 48000.0, acc = 0.0;
    BQ s1 {}, s2 {};
    std::array<Z, 4> z {};
    std::array<double, 30> blocks {};
    int blockLen = 4800, count = 0, bIdx = 0, filled = 0;
};

} // namespace dub
