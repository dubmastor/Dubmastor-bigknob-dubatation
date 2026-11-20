#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const float kDivBeats[] = { 0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };
}

const juce::StringArray& DubKnobProcessor::divisionNames()
{
    static const juce::StringArray names { "1/16", "1/8 T", "1/8", "1/8 D", "1/4", "1/4 D", "1/2" };
    return names;
}

DubKnobProcessor::DubKnobProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DubKnob", createLayout())
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            params[rp->getParameterID().toStdString()] = apvts.getRawParameterValue (rp->getParameterID());
}

juce::AudioProcessorValueTreeState::ParameterLayout DubKnobProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    using Attr = AudioParameterFloatAttributes;
    auto pct = [] { return Attr().withLabel ("%").withStringFromValueFunction ([] (float v, int) { return String (juce::roundToInt (v)) + " %"; }); };
    auto db  = [] { return Attr().withLabel ("dB").withStringFromValueFunction ([] (float v, int) { return (v > 0.05f ? "+" : "") + String (v, 1) + " dB"; }); };
    auto hz  = [] { return Attr().withLabel ("Hz").withStringFromValueFunction ([] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 1) + " kHz" : String (juce::roundToInt (v)) + " Hz"; }); };
    auto ms  = [] { return Attr().withLabel ("ms").withStringFromValueFunction ([] (float v, int) { return String (juce::roundToInt (v)) + " ms"; }); };
    auto unitPct = [] { return Attr().withStringFromValueFunction ([] (float v, int) { return String (juce::roundToInt (v * 100.0f)) + " %"; }); };
    auto logRange = [] (float lo, float hi) { NormalisableRange<float> r (lo, hi); r.setSkewForCentre (std::sqrt (lo * hi)); return r; };

    // --- FILTRO
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "knob", 1 }, "Big Knob", NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "stepped", 1 }, "Stepped", true));
    p.push_back (std::make_unique<AudioParameterChoice>(ParameterID { "mode", 1 }, "Mode", StringArray { "High Pass", "Low Pass" }, 0));
    p.push_back (std::make_unique<AudioParameterChoice>(ParameterID { "slope", 1 }, "Slope", StringArray { "12 dB", "24 dB" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "res", 1 }, "Resonance", NormalisableRange<float> (0.0f, 1.0f), 0.25f, unitPct()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 }, "Drive", NormalisableRange<float> (0.0f, 24.0f), 0.0f, db()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Filter Mix", NormalisableRange<float> (0.0f, 100.0f), 100.0f, pct()));

    // --- ECO DE FITA
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "echoSync", 1 }, "Echo Sync", true));
    p.push_back (std::make_unique<AudioParameterChoice>(ParameterID { "echoDiv", 1 }, "Echo Time (Sync)", divisionNames(), 5));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "echoMs", 1 }, "Repeat Rate (ms)", logRange (40.0f, 1500.0f), 400.0f, ms()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "echoFb", 1 }, "Echo Feedback", NormalisableRange<float> (0.0f, 120.0f), 55.0f, pct()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "echoTone", 1 }, "Echo Tone", logRange (500.0f, 8000.0f), 2500.0f, hz()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "echoWow", 1 }, "Echo Wow", NormalisableRange<float> (0.0f, 100.0f), 25.0f, pct()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "echoSend", 1 }, "Echo Send", NormalisableRange<float> (0.0f, 100.0f), 0.0f, pct()));
    p.push_back (std::make_unique<AudioParameterChoice>(ParameterID { "echoHeads", 1 }, "Echo Heads",
                                                        StringArray { "1", "2", "3", "1+2", "2+3", "1+3", "1+2+3" }, 0));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "echoPP", 1 }, "Ping Pong", false));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "throw", 1 }, "Throw", false));

    // --- EQ de retorno do eco (eqE*) e da mola (eqS*)
    for (auto [pre, name] : { std::pair<const char*, const char*> { "eqE", "Echo EQ" }, { "eqS", "Spring EQ" } })
    {
        const String pr (pre), nm (name);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { pr + "Hp", 1 }, nm + " HP", logRange (20.0f, 2000.0f), 20.0f,
                     Attr().withLabel ("Hz").withStringFromValueFunction ([] (float v, int) { return v < 21.0f ? String ("OFF") : v >= 1000.0f ? String (v / 1000.0f, 1) + " kHz" : String (roundToInt (v)) + " Hz"; })));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { pr + "Mf", 1 }, nm + " Mid Freq", logRange (150.0f, 6000.0f), 1000.0f, hz()));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { pr + "Mg", 1 }, nm + " Mid Gain", NormalisableRange<float> (-12.0f, 12.0f), 0.0f, db()));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { pr + "Lp", 1 }, nm + " LP", logRange (800.0f, 20000.0f), 20000.0f,
                     Attr().withLabel ("Hz").withStringFromValueFunction ([] (float v, int) { return v > 19500.0f ? String ("OFF") : String (v / 1000.0f, 1) + " kHz"; })));
    }

    // --- MOLA
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "spDecay", 1 }, "Spring Decay", NormalisableRange<float> (0.0f, 100.0f), 50.0f, pct()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "spTone", 1 }, "Spring Tone", logRange (1000.0f, 8000.0f), 3500.0f, hz()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "spLevel", 1 }, "Spring Level", NormalisableRange<float> (0.0f, 100.0f), 20.0f, pct()));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "shake", 1 }, "Shake", false));

    // --- SIRENE
    auto logR = [] (float lo, float hi) { NormalisableRange<float> r (lo, hi); r.setSkewForCentre (std::sqrt (lo * hi)); return r; };
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "sirPitch", 1 }, "Siren Pitch", logR (80.0f, 2500.0f), 520.0f, hz()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "sirRate", 1 }, "Siren Rate", logR (0.1f, 25.0f), 2.0f,
                                                        Attr().withLabel ("Hz").withStringFromValueFunction ([] (float v, int) { return String (v, v < 10.0f ? 1 : 0) + " Hz"; })));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "sirDepth", 1 }, "Siren Depth", NormalisableRange<float> (0.0f, 100.0f), 50.0f, pct()));
    p.push_back (std::make_unique<AudioParameterChoice>(ParameterID { "sirWave", 1 }, "Siren Wave",
                                                        StringArray { "Sine", "Triangle", "Square", "Saw Up", "Saw Down" }, 4));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "sirDly", 1 }, "Siren Delay Time", logR (30.0f, 2000.0f), 350.0f, ms()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "sirFb", 1 }, "Siren Delay Feedback", NormalisableRange<float> (0.0f, 95.0f), 50.0f, pct()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "sirLevel", 1 }, "Siren Level", NormalisableRange<float> (0.0f, 100.0f), 50.0f, pct()));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "sirFire", 1 }, "Siren Fire", false));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "sirHold", 1 }, "Siren Hold", false));

    // --- MAXIMIZER (v2.0)
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "maxOn", 1 }, "Maximizer On", true));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "maxThresh", 1 }, "Maximizer Threshold",
                                                        NormalisableRange<float> (-20.0f, 0.0f), 0.0f, db()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "maxCeil", 1 }, "Maximizer Ceiling",
                                                        NormalisableRange<float> (-3.0f, 0.0f), -0.3f, db()));

    // --- MASTER
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "dry", 1 }, "Dry", NormalisableRange<float> (0.0f, 100.0f), 100.0f, pct()));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Output", NormalisableRange<float> (-24.0f, 12.0f), 0.0f, db()));
    return { p.begin(), p.end() };
}

bool DubKnobProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void DubKnobProcessor::prepareToPlay (double sampleRate, int)
{
    filter.prepare (sampleRate, 2);
    echo.prepare (sampleRate);
    spring.prepare (sampleRate);
    siren.prepare (sampleRate);
    echoEq.prepare (sampleRate);
    maximizer.prepare (sampleRate);
    loudness.prepare (sampleRate);
    setLatencySamples (maximizer.getLatency());
    springEq.prepare (sampleRate);
    for (auto* s : { &dryGain, &springGain, &outGain })
        s->reset (sampleRate, 0.02);
}

void DubKnobProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = buffer.getNumChannels(), n = buffer.getNumSamples();
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);
    if (numCh == 0) return;

    // andamento do Cubase
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = juce::jlimit (30.0, 300.0, *b);
    displayBpm.store ((float) bpm);

    // filtro
    dub::Settings fs;
    fs.knob = getP ("knob"); fs.stepped = getP ("stepped") > 0.5f; fs.highPass = getP ("mode") < 0.5f;
    fs.steep = getP ("slope") > 0.5f; fs.resonance = getP ("res"); fs.driveDb = getP ("drive");
    fs.mix = getP ("mix") / 100.0f; fs.outputDb = 0.0f;
    filter.setSettings (fs);

    // eco
    dub::EchoSettings es;
    const int div = juce::jlimit (0, 6, (int) getP ("echoDiv"));
    es.delaySec = getP ("echoSync") > 0.5f ? kDivBeats[div] * 60.0f / (float) bpm : getP ("echoMs") / 1000.0f;
    es.feedback = getP ("echoFb") / 100.0f;
    es.toneHz   = getP ("echoTone");
    es.wow      = getP ("echoWow") / 100.0f;
    es.send     = getP ("throw") > 0.5f ? 1.0f : getP ("echoSend") / 100.0f;
    es.pingPong = getP ("echoPP") > 0.5f;
    static const int headMasks[] = { 1, 2, 4, 3, 6, 5, 7 };
    es.heads    = headMasks[juce::jlimit (0, 6, (int) getP ("echoHeads"))];
    echo.setSettings (es);

    // mola
    dub::SpringSettings ss;
    ss.decay = getP ("spDecay") / 100.0f; ss.toneHz = getP ("spTone"); ss.shake = getP ("shake") > 0.5f;
    spring.setSettings (ss);

    dub::SirenSettings sr;
    sr.pitchHz = getP ("sirPitch"); sr.rateHz = getP ("sirRate"); sr.depth = getP ("sirDepth") / 100.0f;
    sr.wave = juce::jlimit (0, 4, (int) getP ("sirWave"));
    sr.delaySec = getP ("sirDly") / 1000.0f; sr.feedback = getP ("sirFb") / 100.0f;
    sr.level = getP ("sirLevel") / 100.0f;
    sr.gate = getP ("sirFire") > 0.5f || getP ("sirHold") > 0.5f;
    siren.setSettings (sr);
    {
        dub::MaximizerSettings ms;
        ms.on = getP ("maxOn") > 0.5f;
        ms.pushDb = -getP ("maxThresh");
        ms.ceilingDb = getP ("maxCeil");
        maximizer.setSettings (ms);
    }
    float inPk[2] = { 0, 0 }, outPk[2] = { 0, 0 }, grMin = 0.0f;
    echoEq.setSettings (getEqSettings (false));
    springEq.setSettings (getEqSettings (true));

    dryGain.setTargetValue (getP ("dry") / 100.0f);
    springGain.setTargetValue (getP ("spLevel") / 100.0f);
    outGain.setTargetValue (juce::Decibels::decibelsToGain (getP ("out")));

    // cadeia: entrada -> filtro -> (seco + eco) -> mola sobre tudo
    filter.process (buffer.getArrayOfWritePointers(), numCh, n);
    displayFreq.store (filter.currentFrequency());

    float sumSq[2] = { 0.0f, 0.0f };
    float* L = buffer.getWritePointer (0);
    float* R = numCh > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        const float xl = L[i], xr = R ? R[i] : xl;
        float el, er, sl, sr2;
        const float sir = siren.processSample();
        echo.processSample (xl, xr, el, er);
        echoEq.processSample (el, er);
        spring.processSample (0.5f * xl * es.send + el + 0.4f * sir, 0.5f * xr * es.send + er + 0.4f * sir, sl, sr2);

        springEq.processSample (sl, sr2);
        const float sg = springGain.getNextValue(), dg = dryGain.getNextValue(), og = outGain.getNextValue();
        float yl = (xl * dg + el + sl * sg + sir) * og;
        float yr = (xr * dg + er + sr2 * sg + sir) * og;
        inPk[0] = std::max (inPk[0], std::fabs (yl)); inPk[1] = std::max (inPk[1], std::fabs (yr));
        maximizer.processSample (yl, yr);
        grMin = std::min (grMin, maximizer.getLastGainReductionDb());
        outPk[0] = std::max (outPk[0], std::fabs (yl)); outPk[1] = std::max (outPk[1], std::fabs (yr));
        loudness.processSample (yl, yr);
        sumSq[0] += yl * yl; sumSq[1] += yr * yr;
        if (R) { L[i] = yl; R[i] = yr; }
        else     L[i] = 0.5f * (yl + yr);
    }
    for (int ch = 0; ch < 2; ++ch)
    {
        vuRms[ch].store (std::sqrt (sumSq[ch] / (float) juce::jmax (1, n)));
        mInPeak[ch].store (inPk[ch]);
        mOutPeak[ch].store (outPk[ch]);
    }
    mGrDb.store (grMin);
    mLufs.store (loudness.getShortTermLufs());
}

void DubKnobProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DubKnobProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* DubKnobProcessor::createEditor() { return new DubKnobEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DubKnobProcessor(); }
