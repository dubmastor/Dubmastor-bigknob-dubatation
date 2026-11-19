#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "TubbyFilter.h"
#include "DubFX.h"

class DubKnobProcessor : public juce::AudioProcessor
{
public:
    DubKnobProcessor();
    ~DubKnobProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    float getDisplayFrequency() const { return displayFreq.load(); }
    float getDisplayBpm() const       { return displayBpm.load(); }
    dub::EQSettings getEqSettings (bool spring) const
    {
        const char* pre = spring ? "eqS" : "eqE";
        dub::EQSettings e;
        e.hpHz  = getP ((std::string (pre) + "Hp").c_str());
        e.lpHz  = getP ((std::string (pre) + "Lp").c_str());
        e.midHz = getP ((std::string (pre) + "Mf").c_str());
        e.midDb = getP ((std::string (pre) + "Mg").c_str());
        return e;
    }
    float getVuLevel (int ch) const   { return vuRms[ch & 1].load(); } // RMS linear da saída

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const juce::StringArray& divisionNames();

private:
    float getP (const char* id) const { return params.at (id)->load(); }

    dub::TubbyFilter   filter;
    dub::TapeEcho      echo;
    dub::SpringReverb  spring;
    dub::DubSiren      siren;
    dub::ReturnEQ      echoEq, springEq;
    std::atomic<float> vuRms[2] { { 0.0f }, { 0.0f } };
    juce::SmoothedValue<float> dryGain, springGain, outGain;

    std::atomic<float> displayFreq { 20.0f }, displayBpm { 120.0f };
    std::map<std::string, std::atomic<float>*> params;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubKnobProcessor)
};
