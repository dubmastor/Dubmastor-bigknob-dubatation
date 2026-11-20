#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout FitaEcoProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto knob = [&] (const char* id, const char* name, float def, float lo = 0.0f, float hi = 1.0f)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (lo, hi, 0.001f), def));
    };
    knob ("input",     "Input Volume", 0.5f);
    p.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "mode", 1 }, "Mode Selector", 1, 12, 7));
    knob ("rate",      "Repeat Rate",  0.5f);
    knob ("intensity", "Intensity",    0.45f);
    knob ("echo",      "Echo Volume",  0.6f);
    knob ("reverb",    "Reverb Volume",0.3f);
    const char* eqNames[] = { "EQ 100 Hz", "EQ 250 Hz", "EQ 630 Hz", "EQ 1.6 kHz", "EQ 4 kHz", "EQ 10 kHz" };
    for (int b = 0; b < fitaeco::Params::kEqBands; ++b)
        p.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "eq" + juce::String (b + 1), 1 }, eqNames[b],
            juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));
    knob ("wow",       "Wow & Flutter",0.35f);
    knob ("age",       "Tape Age",     0.3f);
    knob ("dry",       "Dry Level",    1.0f);
    knob ("output",    "Output",       0.8f);
    return { p.begin(), p.end() };
}

FitaEcoProcessor::FitaEcoProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pInput = apvts.getRawParameterValue ("input");       pMode = apvts.getRawParameterValue ("mode");
    pRate = apvts.getRawParameterValue ("rate");         pIntensity = apvts.getRawParameterValue ("intensity");
    pEcho = apvts.getRawParameterValue ("echo");         pReverb = apvts.getRawParameterValue ("reverb");
    for (int b = 0; b < fitaeco::Params::kEqBands; ++b)
        pEq[(size_t) b] = apvts.getRawParameterValue ("eq" + juce::String (b + 1));
    pWow = apvts.getRawParameterValue ("wow");           pAge = apvts.getRawParameterValue ("age");
    pDry = apvts.getRawParameterValue ("dry");           pOutput = apvts.getRawParameterValue ("output");
}

bool FitaEcoProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    const auto in  = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void FitaEcoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    echo.prepare (sampleRate);
    monoScratch.setSize (2, samplesPerBlock);
}

void FitaEcoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels(), outCh = getTotalNumOutputChannels();
    if (monoScratch.getNumSamples() < n) monoScratch.setSize (2, n, false, false, true);

    fitaeco::Params prm;
    prm.input = pInput->load();          prm.mode = (int) pMode->load();
    prm.repeatRate = pRate->load();      prm.intensity = pIntensity->load();
    prm.echoVol = pEcho->load();         prm.reverbVol = pReverb->load();
    for (int b = 0; b < fitaeco::Params::kEqBands; ++b) prm.eq[b] = pEq[(size_t) b]->load();
    prm.wowFlutter = pWow->load();       prm.tapeAge = pAge->load();
    prm.dry = pDry->load();              prm.output = pOutput->load();
    echo.setParams (prm);

    const float* inL = buffer.getReadPointer (0);
    const float* inR = buffer.getReadPointer (inCh > 1 ? 1 : 0);
    float* oL = monoScratch.getWritePointer (0);
    float* oR = monoScratch.getWritePointer (1);
    echo.process (inL, inR, oL, oR, n);

    buffer.copyFrom (0, 0, oL, n);
    if (outCh > 1) buffer.copyFrom (1, 0, oR, n);
    else
    {
        buffer.addFrom (0, 0, oR, n);
        buffer.applyGain (0, 0, n, 0.5f);
    }
}

juce::AudioProcessorEditor* FitaEcoProcessor::createEditor() { return new FitaEcoEditor (*this); }

void FitaEcoProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void FitaEcoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FitaEcoProcessor(); }
