#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// 70s studio-hardware look: aluminium-skirt knobs with a printed scale.
class VintageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VintageLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    // graphic-EQ fader: slot + chrome cap with a centre line
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos,
                           float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
};

// Layout: cream enamel upper panel (name plate + tape transport window with
// moving tape and head lamps) over a dark lower panel: MODE and REPEAT RATE as
// highlighted giant knobs, medium echo knobs, a 6-band graphic EQ and small
// utility knobs; wooden end cheeks on both sides.
class FitaEcoEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit FitaEcoEditor (FitaEcoProcessor&);
    ~FitaEcoEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void renderBackground();
    void drawTapeTransport (juce::Graphics&);

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    FitaEcoProcessor& proc;
    VintageLookAndFeel lnf;
    std::array<Knob, 10> knobs;
    std::array<Knob, 6> eqFaders;
    juce::Label status;
    int shownMode = -1;
    float tapePhase = 0.0f;
    juce::Rectangle<int> topPanel, bottomPanel, tapeWindow, eqArea;
    juce::Image background; // pre-rendered wood / enamel / panel (2x for HiDPI)
    juce::Typeface::Ptr plateFace, scriptFace; // embedded fonts (Archivo Black, TeX Gyre Chorus)
    juce::Rectangle<float> plateArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FitaEcoEditor)
};
