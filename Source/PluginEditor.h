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
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Slider::SliderLayout getSliderLayout (juce::Slider&) override;
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

// Painel com todo o layout no tamanho base; o editor o escala para a janela redimensionável
class DubPanel : public juce::Component, private juce::Timer
{
public:
    explicit DubPanel (DubKnobProcessor&);
    ~DubPanel() override;

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
    void mouseUp (const juce::MouseEvent&) override;   // clique nos símbolos / na chave LFO WAVE
    juce::Point<float> waveIconCentre (int i) const;
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
    // master
    Knob dry, out;
    // EQ de retorno
    Knob eqEHp, eqEMf, eqEMg, eqELp;
    juce::Rectangle<int> echoGraph;
    void drawEqGraph (juce::Graphics&, juce::Rectangle<float>);
    void placeEq (juce::Rectangle<int>, Knob&, Knob&, Knob&, Knob&, juce::Rectangle<int>& graph);
    // sirene + VU
    Knob sirPitch, sirRate, sirDepth, sirWave, sirDly, sirFb, sirLevel;
    MomentaryButton fireBtn;
    Toggle sirHold;
    juce::Rectangle<int> sirenPanel, vuPanel, vuArea[2];
    float needlePos[2] { 0.0f, 0.0f }, needleVel[2] { 0.0f, 0.0f };
    bool peakLamp[2] { false, false };
    void drawVu (juce::Graphics&, juce::Rectangle<float>, int ch);

    // MAXIMIZER (v2.0)
    Toggle maxOn;
    Knob maxCeil;
    juce::Slider pushSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aPush;
    juce::Rectangle<int> maxPanel, maxReadout, meterBay;
    float meterTop = 0, meterBottom = 1, inX = 0, grX = 0, outX = 0;
    float inDisp[2] { -60, -60 }, outDisp[2] { -60, -60 }, holdDb[2] { -60, -60 }, holdT[2] { 0, 0 }, grDisp = 0, lufsDisp = -70;
    float dbToY (float db) const { return meterTop + juce::jlimit (0.0f, 1.0f, -db / 30.0f) * (meterBottom - meterTop); }
    void drawMaximizer (juce::Graphics&);

    juce::Image background;
    bool bgStepped = true, bgSync = true;
    juce::Rectangle<int> woodL, woodR, plate;
    juce::Rectangle<int> bigArea, filterPanel, echoPanel, masterPanel, echoInfo;
    juce::String freqText, echoText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubPanel)
};

class DubKnobEditor : public juce::AudioProcessorEditor
{
public:
    explicit DubKnobEditor (DubKnobProcessor& p) : juce::AudioProcessorEditor (p), panel (p)
    {
        addAndMakeVisible (panel);
        const int w = panel.getWidth(), h = panel.getHeight();
        setResizable (true, true);
        setResizeLimits (w / 2, h / 2, w * 3 / 2, h * 3 / 2);
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) w / (double) h);
        setSize (w, h);
    }
    void resized() override
    {
        panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) panel.getWidth()));
    }
private:
    DubPanel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubKnobEditor)
};
