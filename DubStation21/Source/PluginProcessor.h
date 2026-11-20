#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "TapeEchoDSP.h"

class FitaEcoProcessor : public juce::AudioProcessor
{
public:
    FitaEcoProcessor();
    ~FitaEcoProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FitaEco TE-12"; }
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

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    fitaeco::TapeEcho echo;
    juce::AudioBuffer<float> monoScratch;

    std::atomic<float>* pInput {}; std::atomic<float>* pMode {}; std::atomic<float>* pRate {};
    std::atomic<float>* pIntensity {}; std::atomic<float>* pEcho {}; std::atomic<float>* pReverb {};
    std::array<std::atomic<float>*, fitaeco::Params::kEqBands> pEq {}; std::atomic<float>* pWow {};
    std::atomic<float>* pAge {}; std::atomic<float>* pDry {}; std::atomic<float>* pOutput {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FitaEcoProcessor)
};
