// Offline test: renders test signals through the DSP, checks stability and writes WAVs.
#include "../Source/TapeEchoDSP.h"
#include <cstdio>
#include <cstring>
#include <string>

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    uint32_t n = (uint32_t) L.size(), dataBytes = n * 4, riff = 36 + dataBytes, fmtLen = 16, srU = (uint32_t) sr, br = srU * 4;
    uint16_t pcm = 1, ch = 2, ba = 4, bits = 16;
    std::fwrite ("RIFF", 1, 4, f); std::fwrite (&riff, 4, 1, f); std::fwrite ("WAVEfmt ", 1, 8, f);
    std::fwrite (&fmtLen, 4, 1, f); std::fwrite (&pcm, 2, 1, f); std::fwrite (&ch, 2, 1, f);
    std::fwrite (&srU, 4, 1, f); std::fwrite (&br, 4, 1, f); std::fwrite (&ba, 2, 1, f); std::fwrite (&bits, 2, 1, f);
    std::fwrite ("data", 1, 4, f); std::fwrite (&dataBytes, 4, 1, f);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t a = (int16_t) std::clamp (L[i] * 32767.0f, -32768.0f, 32767.0f);
        int16_t b = (int16_t) std::clamp (R[i] * 32767.0f, -32768.0f, 32767.0f);
        std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f);
    }
    std::fclose (f);
}

struct Result { float peak; float tailRms; bool finite; };

static Result run (const char* name, fitaeco::Params p, int sr, float seconds, bool sweepRate)
{
    fitaeco::TapeEcho fx; fx.prepare (sr); fx.setParams (p);
    int total = (int) (seconds * sr), block = 256;
    std::vector<float> in (total, 0.0f), L (total), R (total);
    // short plucked tone burst at t=0.1s (220 Hz, decaying) + click
    for (int i = 0; i < (int) (0.25f * sr); ++i)
    {
        float t = (float) i / sr;
        in[(size_t) (0.1f * sr) + i] = 0.5f * std::sin (2 * fitaeco::kPi * 220.0f * t) * std::exp (-t * 12.0f);
    }
    Result r { 0, 0, true };
    for (int s = 0; s < total; s += block)
    {
        int n = std::min (block, total - s);
        if (sweepRate) { p.repeatRate = 0.5f + 0.45f * std::sin ((float) s / sr * 0.8f); fx.setParams (p); }
        fx.process (&in[(size_t) s], &in[(size_t) s], &L[(size_t) s], &R[(size_t) s], n);
    }
    double acc = 0; int tailStart = total - sr;
    for (int i = 0; i < total; ++i)
    {
        if (! std::isfinite (L[(size_t) i]) || ! std::isfinite (R[(size_t) i])) r.finite = false;
        r.peak = std::max (r.peak, std::max (std::fabs (L[(size_t) i]), std::fabs (R[(size_t) i])));
        if (i >= tailStart) acc += L[(size_t) i] * L[(size_t) i];
    }
    r.tailRms = (float) std::sqrt (acc / sr);
    writeWav (std::string ("out_") + name + ".wav", L, R, sr);
    std::printf ("%-22s finite=%d peak=%.3f tailRMS(last 1s)=%.5f\n", name, r.finite, r.peak, r.tailRms);
    return r;
}

int main()
{
    bool ok = true;
    for (int sr : { 44100, 48000, 96000 })
    {
        std::printf ("--- %d Hz ---\n", sr);
        for (int m = 1; m <= 12; ++m)
        {
            fitaeco::Params p; p.mode = m; p.intensity = 0.5f;
            char nm[32]; std::snprintf (nm, sizeof nm, "mode%02d_%d", m, sr);
            auto r = run (nm, p, sr, 4.0f, false);
            ok &= r.finite && r.peak < 1.3f;
        }
        fitaeco::Params p; p.mode = 11; p.intensity = 1.0f; p.input = 1.0f; p.tapeAge = 1.0f;
        auto r = run (("selfosc_" + std::to_string (sr)).c_str(), p, sr, 10.0f, false);
        ok &= r.finite && r.peak < 1.3f;
        p.intensity = 0.7f;
        r = run (("sweep_" + std::to_string (sr)).c_str(), p, sr, 8.0f, true);
        ok &= r.finite;
        p.intensity = 0.0f; p.mode = 1; p.echoVol = 0; p.reverbVol = 0;
        r = run (("dryonly_" + std::to_string (sr)).c_str(), p, sr, 2.0f, false);
        ok &= r.finite;
    }
    std::printf (ok ? "ALL OK\n" : "FAILURES\n");
    return ok ? 0 : 1;
}
