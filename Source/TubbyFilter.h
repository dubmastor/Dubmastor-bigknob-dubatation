#pragma once
// TubbyFilter — DSP do filtro "big knob" inspirado no high-pass por degraus
// usado no estúdio do King Tubby. Independente do JUCE para poder ser testado.

#include <cmath>
#include <array>
#include <algorithm>

namespace dub
{

// Degraus do knob (Hz). O primeiro (20 Hz) funciona como "OFF" no modo HP.
static constexpr std::array<float, 12> kSteps {
    20.0f, 70.0f, 100.0f, 150.0f, 250.0f, 500.0f,
    1000.0f, 2000.0f, 3000.0f, 5000.0f, 7500.0f, 10000.0f
};

inline float knobToFrequency (float pos, bool stepped)
{
    pos = std::clamp (pos, 0.0f, 1.0f);
    if (stepped)
    {
        const int idx = (int) std::lround (pos * (float) (kSteps.size() - 1));
        return kSteps[(size_t) idx];
    }
    return 20.0f * std::pow (1000.0f, pos); // 20 Hz .. 20 kHz, logarítmico
}

// SVF topology-preserving (Zavalishin) de 2 polos
struct SVF
{
    float ic1 = 0.0f, ic2 = 0.0f;
    void reset() { ic1 = ic2 = 0.0f; }

    // retorna high-pass (isHP) ou low-pass
    inline float process (float x, float g, float k, bool isHP)
    {
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        // leve saturação nos integradores: "calor" analógico na ressonância
        ic1 = 3.0f * std::tanh ((2.0f * v1 - ic1) * (1.0f / 3.0f));
        ic2 = 2.0f * v2 - ic2;
        return isHP ? (x - k * v1 - v2) : v2;
    }
};

struct Settings
{
    float knob      = 0.0f;   // 0..1  (big knob)
    bool  stepped   = true;   // degraus estilo console
    bool  highPass  = true;   // HP (Tubby) ou LP
    bool  steep     = false;  // false = 12 dB/oit, true = 24 dB/oit
    float resonance = 0.2f;   // 0..1
    float driveDb   = 0.0f;   // 0..24
    float mix       = 1.0f;   // 0..1
    float outputDb  = 0.0f;   // -24..+12
};

class TubbyFilter
{
public:
    void prepare (double sampleRate, int numChannels)
    {
        fs = (float) sampleRate;
        channels = std::clamp (numChannels, 1, kMaxCh);
        // suavização ~30 ms: tira o clique da troca de degrau mas mantém o "tranco"
        smoothCoef = std::exp (-1.0f / (0.03f * fs));
        reset();
    }

    void reset()
    {
        for (auto& c : stages) for (auto& s : c) s.reset();
        logFreq = std::log (knobToFrequency (current.knob, current.stepped));
    }

    void setSettings (const Settings& s) { current = s; }

    void process (float* const* data, int numChannels, int numSamples)
    {
        numChannels = std::min (numChannels, channels);
        const float target  = std::log (knobToFrequency (current.knob, current.stepped));
        const float k       = 2.0f - 1.9f * std::clamp (current.resonance, 0.0f, 1.0f); // 1/Q
        const float drive   = std::pow (10.0f, current.driveDb / 20.0f);
        const float makeup  = 1.0f / std::sqrt (drive);
        const float outGain = std::pow (10.0f, current.outputDb / 20.0f);
        const float mix     = std::clamp (current.mix, 0.0f, 1.0f);
        const float nyq     = 0.45f * fs;

        for (int n = 0; n < numSamples; ++n)
        {
            logFreq = target + smoothCoef * (logFreq - target);
            const float fc = std::min (std::exp (logFreq), nyq);
            const float g  = std::tan (3.14159265f * fc / fs);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float dry = data[ch][n];
                float y = (current.driveDb > 0.01f) ? std::tanh (dry * drive) * makeup : dry;
                y = stages[ch][0].process (y, g, k, current.highPass);
                if (current.steep)
                    y = stages[ch][1].process (y, g, 1.414f, current.highPass);
                data[ch][n] = (dry * (1.0f - mix) + y * mix) * outGain;
            }
        }
    }

    float currentFrequency() const { return std::exp (logFreq); }

private:
    static constexpr int kMaxCh = 8;
    float fs = 44100.0f, smoothCoef = 0.0f, logFreq = std::log (20.0f);
    int channels = 2;
    Settings current;
    std::array<std::array<SVF, 2>, kMaxCh> stages {};
};

} // namespace dub
