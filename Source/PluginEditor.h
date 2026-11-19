#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class DubLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DubLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

// Botão que só fica ligado enquanto está pressionado (THROW / SHAKE)
class MomentaryButton : public juce::TextButton
{
public:
    MomentaryButton (const juce::String& text, juce::RangedAudioParameter* p) : juce::TextButton (text), param (p) {}
    void mouseDown (const juce::MouseEvent& e) override { set (true);  juce::TextButton::mouseDown (e); }
    void mouseUp   (const juce::MouseEvent& e) override { set (false); juce::TextButton::mouseUp (e); }
private:
    void set (bool on)
    {
        if (param == nullptr) return;
        param->beginChangeGesture();
        param->setValueNotifyingHost (on ? 1.0f : 0.0f);
        param->endChangeGesture();
    }
    juce::RangedAudioParameter* param;
};

class DubKnobEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DubKnobEditor (DubKnobProcessor&);
    ~DubKnobEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
    };

    void timerCallback() override;
    void setupKnob (Knob&, const juce::String& paramId, const juce::String& name);
    void setupToggle (Toggle&, const juce::String& paramId, const juce::String& text);
    void placeKnob (Knob&, juce::Rectangle<int>);
    void updateSyncVisibility();
    void renderBackground();
    void drawKnobScale (juce::Graphics&, juce::Slider&, int ticks, bool numbers);

    DubKnobProcessor& proc;
    DubLookAndFeel lnf;

    // filtro
    juce::Slider bigKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aBig;
    Knob res, drive, fmix;
    Toggle stepped, lowPass, steep;
    // eco
    Knob echoDiv, echoMs, echoFb, echoTone, echoWow, echoSend, echoHeads;
    Toggle echoSync, echoPP;
    MomentaryButton throwBtn;
    // mola + master
    Knob spDecay, spTone, spLevel, dry, out;
    MomentaryButton shakeBtn;
    // EQ de retorno
    Knob eqEHp, eqEMf, eqEMg, eqELp, eqSHp, eqSMf, eqSMg, eqSLp;
    juce::Rectangle<int> echoGraph, springGraph;
    void drawEqGraph (juce::Graphics&, juce::Rectangle<float>, bool spring);
    void placeEq (juce::Rectangle<int>, Knob&, Knob&, Knob&, Knob&, juce::Rectangle<int>& graph);
    // sirene + VU
    Knob sirPitch, sirRate, sirDepth, sirWave, sirDly, sirFb, sirLevel;
    MomentaryButton fireBtn;
    Toggle sirHold;
    juce::Rectangle<int> sirenPanel, vuPanel, vuArea[2];
    float needlePos[2] { 0.0f, 0.0f }, needleVel[2] { 0.0f, 0.0f };
    bool peakLamp[2] { false, false };
    void drawVu (juce::Graphics&, juce::Rectangle<float>, int ch);

    juce::Image background;
    bool bgStepped = true, bgSync = true;
    juce::Rectangle<int> woodL, woodR, plate;
    juce::Rectangle<int> bigArea, filterPanel, echoPanel, springPanel, masterPanel, echoInfo;
    juce::String freqText, echoText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubKnobEditor)
};
