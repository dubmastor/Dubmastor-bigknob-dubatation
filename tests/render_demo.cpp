// Teste do DSP: resposta em frequência + demo de áudio (riddim sintético passando pelo knob)
#include "../Source/TubbyFilter.h"
#include "../Source/DubFX.h"
#include <cstdio>
#include <vector>
#include <cstdint>
#include <random>

static float gainAt (float hz, dub::Settings s, float fs = 48000.0f)
{
    dub::TubbyFilter f; f.prepare (fs, 1); f.setSettings (s); f.reset();
    const int N = (int) fs; std::vector<float> b (N);
    for (int i = 0; i < N; ++i) b[i] = std::sin (2 * 3.14159265f * hz * i / fs) * 0.25f;
    float* p[1] = { b.data() }; f.process (p, 1, N);
    float peak = 0; for (int i = N / 2; i < N; ++i) peak = std::max (peak, std::fabs (b[i]));
    return 20.0f * std::log10 (peak / 0.25f);
}

static void writeWav (const char* path, const std::vector<float>& L, const std::vector<float>& R, int fs)
{
    FILE* f = std::fopen (path, "wb");
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 4;
    auto u32 = [&](uint32_t v){ std::fwrite (&v, 4, 1, f); };
    auto u16 = [&](uint16_t v){ std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + dataBytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    u32 (16); u16 (1); u16 (2); u32 (fs); u32 (fs * 4); u16 (4); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 (dataBytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t l = (int16_t) (std::clamp (L[i], -1.0f, 1.0f) * 32767), r = (int16_t) (std::clamp (R[i], -1.0f, 1.0f) * 32767);
        std::fwrite (&l, 2, 1, f); std::fwrite (&r, 2, 1, f);
    }
    std::fclose (f);
}

int main()
{
    // 1) resposta em frequência no degrau de 500 Hz
    dub::Settings s; s.stepped = true; s.knob = 5.0f / 11.0f; s.resonance = 0.0f;
    std::printf ("HP 12dB @500Hz step:  ");
    for (float hz : { 60.f, 125.f, 250.f, 500.f, 1000.f, 4000.f }) std::printf ("%gHz=%.1fdB  ", hz, gainAt (hz, s));
    s.steep = true; std::printf ("\nHP 24dB @500Hz step:  ");
    for (float hz : { 60.f, 125.f, 250.f, 500.f, 1000.f, 4000.f }) std::printf ("%gHz=%.1fdB  ", hz, gainAt (hz, s));
    s.steep = false; s.highPass = false; std::printf ("\nLP 12dB @500Hz step:  ");
    for (float hz : { 60.f, 125.f, 250.f, 500.f, 1000.f, 4000.f }) std::printf ("%gHz=%.1fdB  ", hz, gainAt (hz, s));
    s.highPass = true; s.resonance = 0.8f; std::printf ("\nHP res 0.8 @500Hz:     ");
    for (float hz : { 250.f, 500.f, 1000.f }) std::printf ("%gHz=%.1fdB  ", hz, gainAt (hz, s));
    std::printf ("\n");

    // 2) riddim sintético 75 BPM: kick no 3 (one drop), skank no contratempo, baixo
    const int fs = 44100; const float bpm = 75.0f, beat = 60.0f / bpm;
    const int bars = 8; const int N = (int) (bars * 4 * beat * fs);
    std::vector<float> L (N), R (N);
    std::mt19937 rng (1); std::uniform_real_distribution<float> noise (-1, 1);
    const float bassNotes[] = { 55.0f, 55.0f, 65.41f, 49.0f }; // A, A, C, G
    for (int i = 0; i < N; ++i)
    {
        const float t = (float) i / fs;
        const float beatPos = std::fmod (t / beat, 4.0f);
        const int bar = (int) (t / (4 * beat));
        float x = 0;
        // kick + rim no tempo 3
        float tk = (beatPos - 2.0f) * beat;
        if (tk >= 0 && tk < 0.4f) x += 0.9f * std::sin (2 * 3.14159f * (50 + 80 * std::exp (-tk * 30)) * tk) * std::exp (-tk * 7);
        if (tk >= 0 && tk < 0.08f) x += 0.35f * noise (rng) * std::exp (-tk * 60);
        // hi-hat em colcheias
        float th = std::fmod (t, beat / 2);
        x += 0.08f * noise (rng) * std::exp (-th * 80);
        // skank (acorde Am) nos contratempos
        float ts = std::fmod (t + beat / 2, beat);
        if (ts < 0.12f)
            for (float f : { 440.0f, 523.25f, 659.25f }) x += 0.07f * std::sin (2 * 3.14159f * f * t) * std::exp (-ts * 25);
        // baixo
        const float bf = bassNotes[(int) beatPos % 4] * (bar % 2 ? 1.0f : 1.0f);
        float tb = std::fmod (t, beat);
        x += 0.45f * std::tanh (2.0f * std::sin (2 * 3.14159f * bf * t)) * std::min (1.0f, tb * 50) * std::exp (-tb * 1.5f);
        L[i] = R[i] = x * 0.6f;
    }

    // --- estabilidade nos extremos
    {
        dub::TapeEcho e; e.prepare (fs); dub::EchoSettings es; es.feedback = 1.2f; es.send = 1.0f; es.heads = 7; es.delaySec = 0.2f;
        dub::SpringReverb sp; sp.prepare (fs); dub::SpringSettings ss; ss.decay = 1.0f; sp.setSettings (ss);
        e.setSettings (es); float pe = 0, ps = 0; bool nan = false;
        for (int i = 0; i < fs * 20; ++i)
        {
            const float x = i < fs / 10 ? 0.8f * std::sin (i * 0.05f) : 0.0f;
            if (i == fs / 10) { es.send = 0.0f; e.setSettings (es); }
            float a, b, c, d; e.processSample (x, x, a, b); sp.processSample (x, x, c, d);
            pe = std::max (pe, std::fabs (a)); ps = std::max (ps, std::fabs (c));
            nan |= ! std::isfinite (a) || ! std::isfinite (c);
        }
        std::printf ("estabilidade 20s: eco fb120%% pico=%.2f  mola decay100%% pico=%.2f  nan=%d\n", pe, ps, (int) nan);
    }

    // --- demo da estação completa
    dub::TubbyFilter f; f.prepare (fs, 2);
    dub::TapeEcho echo; echo.prepare (fs);
    dub::SpringReverb spring; spring.prepare (fs);
    dub::DubSiren siren; siren.prepare (fs);
    dub::SirenSettings sir; sir.pitchHz = 520; sir.rateHz = 2.2f; sir.depth = 0.5f; sir.wave = 4; sir.delaySec = 0.6f; sir.feedback = 0.55f; sir.level = 0.35f;
    dub::Settings d; d.stepped = true; d.resonance = 0.35f; d.driveDb = 3.0f;
    dub::EchoSettings es; es.feedback = 0.6f; es.toneHz = 2200.0f; es.wow = 0.3f;
    dub::SpringSettings ss; ss.decay = 0.55f; ss.toneHz = 3500.0f;
    const int block = 64;
    for (int i = 0; i < N; i += block)
    {
        const float bt = (float) i / fs / beat; // posição em tempos
        const int nb = std::min (block, N - i);
        float step = 0;
        if (bt >= 8 && bt < 20)  step = std::min (11.0f, (float) (int) ((bt - 8) * 0.75f) + 1);
        if (bt >= 20 && bt < 28) step = std::max (0.0f, 9.0f - (float) (int) ((bt - 20) * 1.25f));
        d.knob = step / 11.0f; f.setSettings (d);

        // compassos 1-4: eco sincronizado 1/4 D; THROW no rim do tempo 3 dos compassos 2 e 4
        const float bp = std::fmod (bt, 4.0f); const int bar = (int) (bt / 4);
        es.heads = 1; es.delaySec = 1.5f * beat;
        es.send = ((bar == 1 || bar == 3) && bp >= 2.0f && bp < 2.3f) ? 1.0f : 0.0f;
        // compassos 5-8: modo livre estilo Space Echo, cabeçotes 1+2+3, repeat rate girando (glide de fita)
        if (bar >= 4) { es.heads = 7; es.delaySec = bar < 6 ? 0.18f : 0.26f; es.send = (bp >= 2.0f && bp < 2.25f) ? 1.0f : 0.0f; es.feedback = 0.75f; }
        echo.setSettings (es);
        ss.shake = (bar == 6 && bp >= 0.0f && bp < 0.1f); spring.setSettings (ss);
        // sirene: disparo curto no compasso 3 (dente descendo) e longo no 7 (quadrada = sirene de polícia)
        sir.gate = (bar == 2 && bp < 1.5f) || (bar == 6 && bp >= 1.0f && bp < 3.0f);
        sir.wave = bar >= 6 ? 2 : 4; sir.rateHz = bar >= 6 ? 1.6f : 2.2f;
        siren.setSettings (sir);

        float* p[2] = { L.data() + i, R.data() + i };
        f.process (p, 2, nb);
        for (int k = 0; k < nb; ++k)
        {
            float el, er, sl, sr; const float xl = L[i + k], xr = R[i + k];
            const float sv = siren.processSample();
            echo.processSample (xl, xr, el, er);
            spring.processSample (0.5f * xl * es.send + el + 0.4f * sv, 0.5f * xr * es.send + er + 0.4f * sv, sl, sr);
            L[i + k] = 0.8f * (xl + el + 0.3f * sl + sv); R[i + k] = 0.8f * (xr + er + 0.3f * sr + sv);
        }
    }
    writeWav ("dub_knob_demo.wav", L, R, fs);
    std::printf ("demo escrita: %d s\n", N / fs);
}
