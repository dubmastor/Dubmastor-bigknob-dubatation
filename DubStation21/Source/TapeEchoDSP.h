// =============================================================================
//  FitaEco TE-12 — Tape echo + spring reverb DSP core
//  Independent of JUCE (pure C++17) so it can be unit-tested offline.
//
//  Model (inspired by classic 3-head tape echo units):
//    input preamp (drive) -> record head saturation -> tape loop with 3
//    playback heads (spacing 1:2:3) -> head-loss filtering -> feedback
//    ("intensity") back to the record head; tape speed ("repeat rate") is
//    slewed so changing it bends the pitch like real tape. Wow & flutter
//    modulate the tape speed. Spring reverb is fed from input + echoes.
//    A 6-band graphic EQ acts on the wet (echo+reverb) signal.
// =============================================================================
#pragma once
#include <cmath>
#include <vector>
#include <cstdint>
#include <algorithm>

namespace fitaeco
{
constexpr float kPi = 3.14159265358979f;

// ---------------------------------------------------------------- utilities
struct Rng
{
    uint32_t s = 0x12345678u;
    float next() // white noise in [-1, 1)
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (float) ((int32_t) s) * (1.0f / 2147483648.0f);
    }
};

struct OnePole // simple 1-pole lowpass / smoother
{
    float a = 0.0f, z = 0.0f;
    void setCutoff (float hz, float sr) { a = std::exp (-2.0f * kPi * hz / sr); }
    void setTime (float seconds, float sr) { a = std::exp (-1.0f / (seconds * sr)); }
    float process (float x) { z = x + a * (z - x); return z; }
};

struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    float process (float x)
    {
        float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0; }
    void setNorm (float B0, float B1, float B2, float A0, float A1, float A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    void lowpass (float f, float q, float sr)
    {
        float w = 2 * kPi * f / sr, c = std::cos (w), al = std::sin (w) / (2 * q);
        setNorm ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highpass (float f, float q, float sr)
    {
        float w = 2 * kPi * f / sr, c = std::cos (w), al = std::sin (w) / (2 * q);
        setNorm ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void bandpass (float f, float q, float sr)
    {
        float w = 2 * kPi * f / sr, c = std::cos (w), al = std::sin (w) / (2 * q);
        setNorm (al, 0, -al, 1 + al, -2 * c, 1 - al);
    }
    void lowShelf (float f, float dB, float sr)
    {
        float A = std::pow (10.0f, dB / 40.0f), w = 2 * kPi * f / sr, c = std::cos (w), s = std::sin (w);
        float al = s / 2 * std::sqrt (2.0f), sA = 2 * std::sqrt (A) * al;
        setNorm (A * ((A + 1) - (A - 1) * c + sA), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sA),
                 (A + 1) + (A - 1) * c + sA, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sA);
    }
    void peaking (float f, float q, float dB, float sr)
    {
        float A = std::pow (10.0f, dB / 40.0f), w = 2 * kPi * f / sr, c = std::cos (w), al = std::sin (w) / (2 * q);
        setNorm (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void highShelf (float f, float dB, float sr)
    {
        float A = std::pow (10.0f, dB / 40.0f), w = 2 * kPi * f / sr, c = std::cos (w), s = std::sin (w);
        float al = s / 2 * std::sqrt (2.0f), sA = 2 * std::sqrt (A) * al;
        setNorm (A * ((A + 1) + (A - 1) * c + sA), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sA),
                 (A + 1) - (A - 1) * c + sA, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sA);
    }
};

struct DelayLine // circular buffer with 4-point Hermite interpolation
{
    std::vector<float> buf; int mask = 0, w = 0;
    void init (int minSize)
    {
        int n = 1; while (n < minSize) n <<= 1;
        buf.assign ((size_t) n, 0.0f); mask = n - 1; w = 0;
    }
    void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    float tap (int d) const { return buf[(size_t) ((w - 1 - d) & mask)]; }
    float read (float d) const // d = delay in samples (>= 1)
    {
        int i = (int) d; float f = d - (float) i;
        float xm1 = tap (i - 1), x0 = tap (i), x1 = tap (i + 1), x2 = tap (i + 2);
        float c1 = 0.5f * (x1 - xm1), c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
};

struct Allpass // Schroeder allpass with integer delay
{
    DelayLine d; int len = 1; float g = 0.5f;
    void init (int samples, float gain) { d.init (samples + 4); len = samples; g = gain; }
    float process (float x)
    {
        float v = d.tap (len - 1);
        float u = x + g * v;
        d.push (u);
        return v - g * u;
    }
};

// ------------------------------------------------------------ spring reverb
// Dispersive allpass cascades (the "drip"/chirp) feeding two cross-coupled
// spring delay lines with damping and slight modulation.
class SpringReverb
{
public:
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        const float k = sr / 44100.0f;
        for (int i = 0; i < kDisp; ++i) { dispA[i].init (std::max (1, (int) (3 * k)), 0.62f); dispB[i].init (std::max (1, (int) (4 * k)), 0.60f); }
        springA.init ((int) (0.08f * sr)); springB.init ((int) (0.08f * sr));
        lenA = 0.0373f * sr; lenB = 0.0419f * sr;
        dampA.lowpass (4200.0f, 0.6f, sr); dampB.lowpass (3900.0f, 0.6f, sr);
        hpIn.highpass (180.0f, 0.7f, sr); bpOut.bandpass (2200.0f, 0.5f, sr);
        diffA.init ((int) (0.0071f * sr), 0.55f); diffB.init ((int) (0.0089f * sr), 0.55f);
        lfoPhase = 0.0f; fbA = fbB = 0.0f;
    }
    void setDecay (float d) { decay = d; }

    void process (float in, float& outL, float& outR)
    {
        float x = hpIn.process (in);
        float a = x + fbB * decay, b = x + fbA * decay; // cross-coupled springs
        for (int i = 0; i < kDisp; ++i) { a = dispA[i].process (a); b = dispB[i].process (b); }
        lfoPhase += 0.7f / sr; if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
        float m = std::sin (2 * kPi * lfoPhase) * 0.0006f * sr;
        springA.push (std::tanh (a)); springB.push (std::tanh (b));
        float ya = dampA.process (springA.read (lenA + m));
        float yb = dampB.process (springB.read (lenB - m));
        fbA = ya; fbB = yb;
        float l = diffA.process (ya + 0.3f * yb), r = diffB.process (yb + 0.3f * ya);
        outL = 0.6f * l + 0.4f * ya;
        outR = 0.6f * r + 0.4f * yb;
    }
private:
    static constexpr int kDisp = 10;
    float sr = 44100.0f, decay = 0.8f, lenA = 1, lenB = 1, lfoPhase = 0, fbA = 0, fbB = 0;
    Allpass dispA[kDisp], dispB[kDisp], diffA, diffB;
    DelayLine springA, springB;
    Biquad dampA, dampB, hpIn, bpOut;
};

// --------------------------------------------------------------- main unit
struct Params
{
    float input      = 0.5f;  // 0..1  preamp drive
    int   mode       = 7;     // 1..12 (see headMask/reverbOn)
    float repeatRate = 0.5f;  // 0..1  tape speed (1 = fastest = shortest)
    float intensity  = 0.45f; // 0..1  feedback (>~0.85 self-oscillates)
    float echoVol    = 0.6f;  // 0..1
    float reverbVol  = 0.3f;  // 0..1
    static constexpr int kEqBands = 6;
    float eq[kEqBands] = {};  // dB, -12..+12 per band (graphic EQ on the wet signal)
    float wowFlutter = 0.35f; // 0..1
    float tapeAge    = 0.3f;  // 0..1  hiss, darker heads, more saturation
    float dry        = 1.0f;  // 0..1
    float output     = 0.8f;  // 0..1
};

class TapeEcho
{
public:
    // Modes: 1-4 echo only, 5-11 echo + reverb, 12 reverb only
    static int headMask (int mode)
    {
        static const int m[13] = { 0, 1, 2, 4, 6, 1, 2, 4, 3, 6, 5, 7, 0 };
        return m[std::clamp (mode, 1, 12)];
    }
    static bool reverbOn (int mode) { return mode >= 5; }

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        tape.init ((int) (sr * 1.4f)); // longest head ~ 0.6 s + modulation headroom
        reverb.prepare (sr);
        speedSmooth.setTime (0.25f, sr);     // tape motor inertia -> pitch glides
        for (auto& s : paramSmooth) s.setTime (0.02f, sr);
        recHP.highpass (60.0f, 0.7f, sr);
        fbHP.highpass (90.0f, 0.6f, sr);
        wowNoise.setCutoff (1.5f, sr);
        flutNoise.setCutoff (25.0f, sr);
        hissLP.setCutoff (9000.0f, sr);
        speedSmooth.z = 0.5f; // start at mid speed
        reset();
        updateFilters (true);
    }

    void reset()
    {
        tape.clear();
        for (auto& b : headLP) b.reset();
        lastEcho = 0.0f;
    }

    void setParams (const Params& p) { target = p; }

    // Processes a mono-in / stereo-out block (inL may equal inR for mono).
    void process (const float* inL, const float* inR, float* outL, float* outR, int n)
    {
        updateFilters (false);
        const int mask = headMask (target.mode);
        const bool rev = reverbOn (target.mode);
        const float echoGainTarget = (target.mode == 12) ? 0.0f : target.echoVol * 1.4f;
        const float revGainTarget  = rev ? target.reverbVol * 1.2f : 0.0f;
        const float drive = 0.5f + target.input * 3.5f;
        const float fb = target.intensity * 1.09f; // single head: oscillates above ~0.8
        const float wf = target.wowFlutter;

        for (int i = 0; i < n; ++i)
        {
            const float x = 0.5f * (inL[i] + inR[i]);

            // smoothed parameters
            const float speed   = speedSmooth.process (target.repeatRate);
            const float echoG   = paramSmooth[0].process (echoGainTarget);
            const float revG    = paramSmooth[1].process (revGainTarget);
            const float fbG     = paramSmooth[2].process (fb);
            const float dryG    = paramSmooth[3].process (target.dry);
            const float outG    = paramSmooth[4].process (target.output * 1.25f);
            const float h1G     = paramSmooth[5].process ((mask & 1) ? 1.0f : 0.0f);
            const float h2G     = paramSmooth[6].process ((mask & 2) ? 1.0f : 0.0f);
            const float h3G     = paramSmooth[7].process ((mask & 4) ? 1.0f : 0.0f);

            // wow (slow, ~0.6 Hz + random drift) and flutter (~7 Hz + noise)
            wowPh += 0.63f / sr;  if (wowPh >= 1) wowPh -= 1;
            flPh  += 7.3f / sr;   if (flPh  >= 1) flPh  -= 1;
            const float wow = std::sin (2 * kPi * wowPh) * 0.6f + wowNoise.process (rng.next()) * 25.0f;
            const float flt = std::sin (2 * kPi * flPh) * 0.5f + flutNoise.process (rng.next()) * 4.0f;
            const float mod = 1.0f + wf * (0.0035f * wow + 0.0011f * flt);

            // head-1 delay: 175 ms (slow) .. 55 ms (fast), heads spaced 1:2:3
            const float base = (0.175f - 0.120f * speed) * sr * mod;

            float h1 = headLP[0].process (tape.read (base));
            float h2 = headLP[1].process (tape.read (base * 2.0f));
            float h3 = headLP[2].process (tape.read (base * 3.0f));
            float echo = h1 * h1G + h2 * h2G + h3 * h3G;
            // multi-head modes: tame the summed loop gain (still runs away a bit easier, like the real thing)
            const float headSum = std::max (1.0f, h1G + h2G + h3G);
            const float fbNorm = 1.0f / std::pow (headSum, 0.85f);
            const float outNorm = 1.0f / std::sqrt (headSum);

            // tape hiss at playback
            float hiss = hissLP.process (rng.next()) * (0.0004f + 0.004f * target.tapeAge * target.tapeAge);
            echo += hiss * (h1G + h2G + h3G);

            // preamp + record head (asymmetric soft saturation, bias-ish)
            float pre = std::tanh (x * drive) / std::tanh (drive * 0.6f + 0.2f);
            float fbSig = fbHP.process (echo) * fbG * fbNorm;
            float rec = recHP.process (pre * 0.7f + fbSig);
            float satAmt = 1.0f + 1.5f * target.tapeAge;
            float r = std::tanh (satAmt * rec + 0.08f) - std::tanh (0.08f);
            tape.push (r / satAmt * 1.15f);
            lastEcho = echo;

            // spring reverb fed by input + echoes
            float rl = 0, rr = 0;
            reverb.process ((pre * 0.5f + echo * 0.5f) * 0.6f, rl, rr);

            float wetL = echo * outNorm * echoG + rl * revG;
            float wetR = echo * outNorm * echoG + rr * revG;
            for (int b = 0; b < Params::kEqBands; ++b)
            {
                wetL = eqL[b].process (wetL);
                wetR = eqR[b].process (wetR);
            }

            float yl = (x * dryG + wetL) * outG;
            float yr = (x * dryG + wetR) * outG;
            // gentle output clip for safety
            outL[i] = std::tanh (yl * 0.8f) * 1.25f;
            outR[i] = std::tanh (yr * 0.8f) * 1.25f;
            (void) inR;
        }
    }

private:
    void updateFilters (bool force)
    {
        bool same = target.tapeAge == cur.tapeAge && target.repeatRate == cur.repeatRate;
        for (int b = 0; b < Params::kEqBands; ++b) same = same && target.eq[b] == cur.eq[b];
        if (! force && same) return;
        cur = target;
        for (int b = 0; b < Params::kEqBands; ++b)
        {
            const float f = std::min (eqFreqs[b], 0.45f * sr);
            for (auto* q : { &eqL[b], &eqR[b] })
            {
                const float z1 = q->z1, z2 = q->z2;
                q->peaking (f, 1.1f, target.eq[b], sr);
                q->z1 = z1; q->z2 = z2;
            }
        }
        // head loss: slower tape and older tape are darker
        float lp = (3200.0f + 3300.0f * target.repeatRate) * (1.0f - 0.35f * target.tapeAge);
        for (auto& b : headLP) { float z1 = b.z1, z2 = b.z2; b.lowpass (lp, 0.6f, sr); b.z1 = z1; b.z2 = z2; }
        reverb.setDecay (0.72f);
    }

    float sr = 44100.0f;
    Params target, cur;
    DelayLine tape;
    SpringReverb reverb;
    OnePole speedSmooth, paramSmooth[8], wowNoise, flutNoise, hissLP;
public:
    static constexpr float eqFreqs[Params::kEqBands] = { 100.0f, 250.0f, 630.0f, 1600.0f, 4000.0f, 10000.0f };
private:
    Biquad headLP[3], recHP, fbHP, eqL[Params::kEqBands], eqR[Params::kEqBands];
    Rng rng;
    float wowPh = 0, flPh = 0, lastEcho = 0;
};
} // namespace fitaeco
