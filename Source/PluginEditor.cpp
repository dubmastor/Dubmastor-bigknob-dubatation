#include "PluginEditor.h"

// Visual vintage: painel de metal creme envelhecido, laterais de madeira,
// knobs de baquelite com saia de alumínio, lâmpadas piloto e displays âmbar.
namespace colours
{
    const juce::Colour plate     { 0xffe4d8bc };
    const juce::Colour plateDark { 0xffc9b994 };
    const juce::Colour ink       { 0xff2b241d };
    const juce::Colour inkSoft   { 0xff5c5144 };
    const juce::Colour labelBg   { 0xff1a1714 };
    const juce::Colour cream     { 0xffefe5cc };
    const juce::Colour red       { 0xffb3352a };   // só no VU (zona de pico)
    // cores da bandeira da Jamaica, levemente envelhecidas
    const juce::Colour jamGreen  { 0xff0b8a3c };
    const juce::Colour jamGold   { 0xffeabd12 };
    const juce::Colour jamBlack  { 0xff141210 };
    const juce::Colour gold      { 0xffd19a1e };
    const juce::Colour green     { 0xff3d7347 };
    const juce::Colour amber     { 0xffffa928 };
    const juce::Colour glass     { 0xff17110a };
    const juce::Colour woodDark  { 0xff2e1a0d };
    const juce::Colour woodLight { 0xff6b4423 };
}

static constexpr float kStartAngle = juce::MathConstants<float>::pi * 1.2f;
static constexpr float kEndAngle   = juce::MathConstants<float>::pi * 2.8f;
static constexpr float kSelStart   = juce::MathConstants<float>::twoPi - 1.40f; // seletor de onda: -80° .. +80°
static constexpr float kSelEnd     = juce::MathConstants<float>::twoPi + 1.40f;

// ícone de cada forma de onda (0 seno, 1 triângulo, 2 quadrada, 3 dente subindo, 4 dente descendo)
static juce::Path wavePath (int type, juce::Rectangle<float> r)
{
    juce::Path p;
    const float x0 = r.getX(), x1 = r.getRight(), yT = r.getY(), yB = r.getBottom(), yM = r.getCentreY(), w = r.getWidth();
    switch (type)
    {
        case 0:
            p.startNewSubPath (x0, yM);
            for (int i = 1; i <= 24; ++i)
                p.lineTo (x0 + w * (float) i / 24.0f, yM - std::sin (juce::MathConstants<float>::twoPi * (float) i / 24.0f) * (yM - yT));
            break;
        case 1:
            p.startNewSubPath (x0, yM); p.lineTo (x0 + w * 0.25f, yT); p.lineTo (x0 + w * 0.75f, yB); p.lineTo (x1, yM);
            break;
        case 2:
            p.startNewSubPath (x0, yB); p.lineTo (x0, yT); p.lineTo (x0 + w * 0.5f, yT);
            p.lineTo (x0 + w * 0.5f, yB); p.lineTo (x1, yB); p.lineTo (x1, yT);
            break;
        case 3:
            p.startNewSubPath (x0, yB); p.lineTo (x0 + w * 0.5f, yT); p.lineTo (x0 + w * 0.5f, yB); p.lineTo (x1, yT); p.lineTo (x1, yB);
            break;
        default:
            p.startNewSubPath (x0, yT); p.lineTo (x0 + w * 0.5f, yB); p.lineTo (x0 + w * 0.5f, yT); p.lineTo (x1, yB); p.lineTo (x1, yT);
            break;
    }
    return p;
}

static juce::Font stencil (float size, float kerning = 0.08f)
{
    return juce::Font (juce::FontOptions (size, juce::Font::bold)).withExtraKerningFactor (kerning);
}

// área circular do knob dentro do slider (mesma conta do LookAndFeel)
static juce::Rectangle<float> knobCircle (const juce::Slider& s)
{
    auto r = s.getBounds().toFloat();
    if (s.getTextBoxPosition() == juce::Slider::TextBoxBelow) r.removeFromBottom ((float) s.getTextBoxHeight());
    if (s.getName() == "selector")
    {
        const float rad = juce::jmin (r.getWidth() * 0.5f - 29.0f, r.getHeight() * 0.5f - 14.0f, 30.0f);
        return juce::Rectangle<float> (rad * 2, rad * 2).withCentre ({ r.getCentreX(), r.getBottom() - rad - 8.0f });
    }
    r = r.reduced (s.getName() == "big" ? 38.0f : 13.0f);
    const float d = juce::jmin (r.getWidth(), r.getHeight());
    return r.withSizeKeepingCentre (d, d);
}

//==============================================================================
DubLookAndFeel::DubLookAndFeel()
{
    setColour (juce::Label::textColourId, colours::ink);
    setColour (juce::Slider::textBoxTextColourId, colours::inkSoft);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void DubLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                       float startAngle, float endAngle, juce::Slider& s)
{
    if (s.getName() == "selector")
    {
        const auto r = knobCircle (s).translated ((float) -s.getX(), (float) -s.getY());
        const auto c = r.getCentre();
        const float radius = r.getWidth() * 0.5f;
        const float angle = startAngle + pos * (endAngle - startAngle);
        auto circle = [&] (float rr) { return juce::Rectangle<float> (rr * 2, rr * 2).withCentre (c); };

        // base escura com anel de encaixe
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillEllipse (circle (radius * 0.80f).translated (1.5f, 3.0f));
        juce::ColourGradient base (juce::Colour (0xff35302b), c.x, c.y - radius, juce::Colour (0xff0a0908), c.x, c.y + radius, false);
        g.setGradientFill (base);
        g.fillEllipse (circle (radius * 0.80f));
        g.setColour (colours::jamGold);
        g.drawEllipse (circle (radius * 0.80f), 2.0f);

        // cabeça de galinha em baquelite marfim
        juce::Path head;
        const float L = radius * 1.08f, wB = radius * 0.44f, wT = radius * 0.13f, tail = radius * 0.62f;
        head.startNewSubPath (-wT, -L);
        head.quadraticTo (0, -L - wT * 1.4f, wT, -L);
        head.lineTo (wB, -radius * 0.05f);
        head.quadraticTo (wB * 1.05f, tail, 0, tail);
        head.quadraticTo (-wB * 1.05f, tail, -wB, -radius * 0.05f);
        head.closeSubPath();
        const auto tr = juce::AffineTransform::rotation (angle).translated (c.x, c.y);

        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillPath (head, tr.translated (2.0f, 4.0f));
        juce::ColourGradient ivory (juce::Colour (0xfff6edd6), c.x - radius, c.y - radius, juce::Colour (0xffb9a882), c.x + radius, c.y + radius, false);
        g.setGradientFill (ivory);
        g.fillPath (head, tr);
        g.setColour (juce::Colour (0xff7a6a4c));
        g.strokePath (head, juce::PathStrokeType (1.0f), tr);

        // risco indicador preto e parafuso central
        juce::Path line;
        line.addRoundedRectangle (-1.4f, -L + 3.0f, 2.8f, L * 0.75f, 1.4f);
        g.setColour (colours::ink);
        g.fillPath (line, tr);
        g.setColour (juce::Colour (0xff8c7d5e));
        g.fillEllipse (circle (radius * 0.12f));
        return;
    }

    const bool isBig = s.getName() == "big";
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (isBig ? 38.0f : 13.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    auto circle = [&] (float r) { return juce::Rectangle<float> (r * 2, r * 2).withCentre (c); };

    // sombra projetada no painel
    for (int i = 4; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.07f));
        g.fillEllipse (circle (radius + (float) i).translated (1.5f, 3.0f));
    }

    // saia de alumínio escovado
    juce::ColourGradient skirt (juce::Colour (0xffd8d6d0), c.x - radius, c.y - radius,
                                juce::Colour (0xff6f6c66), c.x + radius, c.y + radius, false);
    skirt.addColour (0.5, juce::Colour (0xffa9a69f));
    g.setGradientFill (skirt);
    g.fillEllipse (circle (radius));
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawEllipse (circle (radius), 1.0f);

    // marcas radiais da saia (usinagem)
    g.setColour (juce::Colours::black.withAlpha (0.10f));
    for (int i = 0; i < 60; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 60.0f;
        g.drawLine (c.x + std::sin (a) * radius * 0.82f, c.y - std::cos (a) * radius * 0.82f,
                    c.x + std::sin (a) * radius * 0.98f, c.y - std::cos (a) * radius * 0.98f, 0.6f);
    }

    // corpo de baquelite
    const float capR = radius * (isBig ? 0.80f : 0.74f);
    juce::ColourGradient body (juce::Colour (0xff3d3833), c.x - capR, c.y - capR,
                               juce::Colour (0xff070605), c.x + capR * 0.6f, c.y + capR, false);
    g.setGradientFill (body);
    g.fillEllipse (circle (capR));

    if (isBig) // serrilhado
    {
        g.setColour (juce::Colours::black);
        for (int i = 0; i < 40; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 40.0f;
            g.drawLine (c.x + std::sin (a) * capR * 0.90f, c.y - std::cos (a) * capR * 0.90f,
                        c.x + std::sin (a) * capR,          c.y - std::cos (a) * capR, 2.2f);
        }
    }

    // brilho do topo
    juce::ColourGradient gloss (juce::Colours::white.withAlpha (0.16f), c.x, c.y - capR,
                                juce::Colours::transparentWhite, c.x, c.y + capR * 0.2f, false);
    g.setGradientFill (gloss);
    g.fillEllipse (circle (capR * 0.86f).translated (0, -capR * 0.06f));

    if (isBig) // tampa central vermelha envelhecida
    {
        const float r = capR * 0.30f;
        juce::ColourGradient cap (colours::jamGold.brighter (0.25f), c.x - r, c.y - r, colours::jamGold.darker (0.55f), c.x + r, c.y + r, false);
        g.setGradientFill (cap);
        g.fillEllipse (circle (r));
    }

    // ponteiro: risco creme no corpo + entalhe preto na saia
    auto rot = juce::AffineTransform::rotation (angle).translated (c.x, c.y);
    juce::Path line;
    const float lw = isBig ? 5.0f : 2.6f;
    line.addRoundedRectangle (-lw * 0.5f, -capR * 0.95f, lw, capR * (isBig ? 0.5f : 0.62f), lw * 0.5f);
    g.setColour (colours::cream);
    g.fillPath (line, rot);
    juce::Path notch;
    notch.addRectangle (-1.2f, -radius * 0.99f, 2.4f, radius - capR + 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.fillPath (notch, rot);
}

// botão com tampa escura e lâmpada piloto
void DubLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (3.0f);
    const bool on = b.getToggleState();

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (r.translated (0, 2), 3.0f);
    juce::ColourGradient capG (juce::Colour (0xff3a3632).brighter (over ? 0.08f : 0.0f), r.getX(), r.getY(),
                               juce::Colour (0xff141210), r.getX(), r.getBottom(), false);
    g.setGradientFill (capG);
    g.fillRoundedRectangle (r.translated (0, down ? 1.0f : 0.0f), 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3, r.getRight() - 3);

    // lâmpada
    const auto lamp = juce::Rectangle<float> (9, 9).withCentre ({ r.getX() + 14.0f, r.getCentreY() });
    if (on)
    {
        g.setColour (colours::amber.withAlpha (0.25f));
        g.fillEllipse (lamp.expanded (5));
        juce::ColourGradient lit (juce::Colour (0xffffe3a0), lamp.getCentreX(), lamp.getCentreY(),
                                  colours::amber.darker (0.2f), lamp.getRight(), lamp.getBottom(), true);
        g.setGradientFill (lit);
    }
    else
        g.setColour (juce::Colour (0xff4a3a22));
    g.fillEllipse (lamp);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (lamp, 1.0f);

    g.setColour (colours::cream.withAlpha (on ? 1.0f : 0.65f));
    g.setFont (stencil (11.5f));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (22), juce::Justification::centred);
}

// THROW / SHAKE: tampa vermelha grande de botão de pedal
void DubLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (3.0f);
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRoundedRectangle (r, 6.0f);                 // moldura
    auto cap = r.reduced (4.0f).translated (0, down ? 1.5f : 0.0f);
    juce::ColourGradient capG ((down ? colours::jamGreen.brighter (0.4f) : colours::jamGreen).brighter (over ? 0.1f : 0.0f),
                               cap.getX(), cap.getY(), colours::jamGreen.darker (0.75f), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (capG);
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (down ? 0.10f : 0.22f));
    g.fillRoundedRectangle (cap.withHeight (cap.getHeight() * 0.38f).reduced (4, 2), 3.0f);
    if (down)
    {
        g.setColour (colours::jamGold.withAlpha (0.45f));
        g.drawRoundedRectangle (r.expanded (2), 8.0f, 3.0f);
    }
}

void DubLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    g.setColour (colours::jamGold);
    g.setFont (stencil (15.0f, 0.18f));
    g.drawText (b.getButtonText(), b.getLocalBounds().translated (0, down ? 2 : 1), juce::Justification::centred);
}

// fader PUSH do maximizer: trilho escuro + manopla dourada com seta apontando para o medidor
juce::Slider::SliderLayout DubLookAndFeel::getSliderLayout (juce::Slider& s)
{
    if (s.getName() == "push")
    {
        juce::Slider::SliderLayout l;
        l.sliderBounds = s.getLocalBounds().reduced (0, 10);
        return l;
    }
    return LookAndFeel_V4::getSliderLayout (s);
}

void DubLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float minPos, float maxPos,
                                       juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (s.getName() != "push")
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, minPos, maxPos, style, s);
        return;
    }
    const float cx = (float) x + (float) w * 0.42f;
    // trilho
    auto slot = juce::Rectangle<float> (5.0f, (float) h + 8.0f).withCentre ({ cx, (float) y + (float) h * 0.5f });
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.fillRoundedRectangle (slot, 2.5f);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawRoundedRectangle (slot.translated (0.7f, 0.7f), 2.5f, 0.6f);
    // parte "empurrada" acesa em dourado
    g.setColour (colours::jamGold.withAlpha (0.85f));
    g.fillRoundedRectangle (slot.withTop ((float) y - 2.0f).withBottom (sliderPos).reduced (1.2f, 0.0f), 1.5f);

    // manopla
    juce::Path cap;
    const float hw = (float) w * 0.40f, hh = 9.0f;
    cap.startNewSubPath (cx - hw, sliderPos - hh);
    cap.lineTo (cx + hw, sliderPos - hh);
    cap.lineTo (cx + hw + 9.0f, sliderPos);
    cap.lineTo (cx + hw, sliderPos + hh);
    cap.lineTo (cx - hw, sliderPos + hh);
    cap.closeSubPath();
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillPath (cap, juce::AffineTransform::translation (1.5f, 2.5f));
    juce::ColourGradient cg (colours::jamGold.brighter (0.35f), cx, sliderPos - hh, colours::jamGold.darker (0.45f), cx, sliderPos + hh, false);
    g.setGradientFill (cg);
    g.fillPath (cap);
    g.setColour (juce::Colour (0xff5a4510));
    g.strokePath (cap, juce::PathStrokeType (1.0f));
    g.setColour (juce::Colour (0xff3b2d08).withAlpha (0.8f));
    for (int i = -1; i <= 1; ++i)
        g.drawLine (cx - hw + 4.0f, sliderPos + (float) i * 3.5f, cx + hw - 2.0f, sliderPos + (float) i * 3.5f, 1.0f);
}

//==============================================================================
DubPanel::DubPanel (DubKnobProcessor& p)
    : proc (p),
      throwBtn ("THROW", p.apvts.getParameter ("throw")),
      shakeBtn ("SHAKE", p.apvts.getParameter ("shake")),
      fireBtn ("FIRE", p.apvts.getParameter ("sirFire"))
{
    setLookAndFeel (&lnf);

    bigKnob.setName ("big");
    bigKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    bigKnob.setRotaryParameters (kStartAngle, kEndAngle, true);
    bigKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    bigKnob.setMouseDragSensitivity (300);
    addAndMakeVisible (bigKnob);
    aBig = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "knob", bigKnob);

    setupKnob (res,   "res",   "RESONANCE");
    setupKnob (drive, "drive", "DRIVE");
    setupKnob (fmix,  "mix",   "FILTER MIX");
    setupToggle (stepped, "stepped", "STEPPED");
    setupToggle (lowPass, "mode",    "LOW PASS");
    setupToggle (steep,   "slope",   "24 dB");

    setupKnob (echoDiv,   "echoDiv",   "TIME");
    setupKnob (echoMs,    "echoMs",    "REPEAT RATE");
    setupKnob (echoFb,    "echoFb",    "INTENSITY");
    setupKnob (echoHeads, "echoHeads", "HEADS");
    setupKnob (echoTone,  "echoTone",  "TONE");
    setupKnob (echoWow,   "echoWow",   "WOW");
    setupKnob (echoSend,  "echoSend",  "SEND");
    setupToggle (echoSync, "echoSync", "SYNC");
    setupToggle (echoPP,   "echoPP",   "PING-PONG");
    addAndMakeVisible (throwBtn);
    echoSync.button.onClick = [this] { updateSyncVisibility(); };

    setupKnob (spDecay, "spDecay", "DECAY");
    setupKnob (spTone,  "spTone",  "TONE");
    setupKnob (spLevel, "spLevel", "LEVEL");
    addAndMakeVisible (shakeBtn);

    setupKnob (eqEHp, "eqEHp", "HP");   setupKnob (eqEMf, "eqEMf", "MID");
    setupKnob (eqEMg, "eqEMg", "GAIN"); setupKnob (eqELp, "eqELp", "LP");
    setupKnob (eqSHp, "eqSHp", "HP");   setupKnob (eqSMf, "eqSMf", "MID");
    setupKnob (eqSMg, "eqSMg", "GAIN"); setupKnob (eqSLp, "eqSLp", "LP");
    for (auto* k : { &eqEHp, &eqEMf, &eqEMg, &eqELp, &eqSHp, &eqSMf, &eqSMg, &eqSLp })
    {
        k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 66, 14);
        k->label.setFont (stencil (10.0f, 0.12f));
        k->slider.setDoubleClickReturnValue (true, k->slider.getValue());
    }

    setupKnob (sirPitch, "sirPitch", "PITCH");
    setupKnob (sirRate,  "sirRate",  "RATE");
    setupKnob (sirDepth, "sirDepth", "DEPTH");
    setupKnob (sirWave,  "sirWave",  "WAVE");
    sirWave.slider.setName ("selector");
    sirWave.slider.setRotaryParameters (kSelStart, kSelEnd, true);
    sirWave.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    sirWave.slider.setMouseDragSensitivity (120);
    sirWave.label.setText ("LFO WAVE", juce::dontSendNotification);
    sirWave.label.setColour (juce::Label::textColourId, colours::jamGreen.darker (0.3f));
    setupKnob (sirDly,   "sirDly",   "DELAY");
    setupKnob (sirFb,    "sirFb",    "REPEATS");
    setupKnob (sirLevel, "sirLevel", "LEVEL");
    addAndMakeVisible (fireBtn);
    setupToggle (sirHold, "sirHold", "HOLD");

    setupToggle (maxOn, "maxOn", "ON");
    setupKnob (maxCeil, "maxCeil", "CEILING");
    maxCeil.slider.setNumDecimalPlacesToDisplay (1);
    pushSlider.setName ("push");
    pushSlider.setSliderStyle (juce::Slider::LinearVertical);
    pushSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    pushSlider.setDoubleClickReturnValue (true, 0.0);
    pushSlider.setTooltip ("Arraste para baixo para empurrar o volume");
    addAndMakeVisible (pushSlider);
    aPush = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "maxThresh", pushSlider);

    setupKnob (dry, "dry", "DRY");
    setupKnob (out, "out", "OUTPUT");

    stepped.button.onClick = [this] { renderBackground(); repaint(); };

    setSize (1436, 800);
    updateSyncVisibility();
    startTimerHz (30);
}

DubPanel::~DubPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DubPanel::setupKnob (Knob& k, const juce::String& id, const juce::String& name)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setRotaryParameters (kStartAngle, kEndAngle, true);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
    k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k.slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    k.slider.setColour (juce::Slider::textBoxTextColourId, colours::inkSoft);
    addAndMakeVisible (k.slider);
    k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (stencil (11.0f, 0.12f));
    k.label.setColour (juce::Label::textColourId, colours::ink);
    addAndMakeVisible (k.label);
}

void DubPanel::setupToggle (Toggle& t, const juce::String& id, const juce::String& text)
{
    t.button.setButtonText (text);
    addAndMakeVisible (t.button);
    t.att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, t.button);
}

void DubPanel::placeKnob (Knob& k, juce::Rectangle<int> r)
{
    k.label.setBounds (r.removeFromTop (15));
    k.slider.setBounds (r);
}

void DubPanel::updateSyncVisibility()
{
    const bool sync = echoSync.button.getToggleState();
    echoDiv.slider.setVisible (sync);  echoDiv.label.setVisible (sync);
    echoMs.slider.setVisible (! sync); echoMs.label.setVisible (! sync);
}

void DubPanel::timerCallback()
{
    // balística de VU: 0 VU = -18 dBFS, escala linear em tensão de -20 a +3 VU
    for (int ch = 0; ch < 2; ++ch)
    {
        const float rms = proc.getVuLevel (ch);
        const float vu  = juce::Decibels::gainToDecibels (rms, -100.0f) + 18.0f;
        const float a   = std::pow (10.0f, juce::jlimit (-30.0f, 6.0f, vu) / 20.0f);
        const float target = juce::jlimit (-0.02f, 1.06f, (a - 0.1f) / (1.4125f - 0.1f));
        needleVel[ch] = (needleVel[ch] + (target - needlePos[ch]) * 0.30f) * 0.58f;
        needlePos[ch] += needleVel[ch];
        peakLamp[ch] = vu > 3.0f;
    }
    repaint (vuPanel);

    // medidor moderno: pico com queda de 20 dB/s, hold de 1.5 s, GR com retorno suave
    for (int ch = 0; ch < 2; ++ch)
    {
        const float inDb  = juce::Decibels::gainToDecibels (proc.getMaxInPeak (ch), -60.0f);
        const float outDb = juce::Decibels::gainToDecibels (proc.getMaxOutPeak (ch), -60.0f);
        inDisp[ch]  = inDb  > inDisp[ch]  ? inDb  : juce::jmax (inDb,  inDisp[ch]  - 0.67f);
        outDisp[ch] = outDb > outDisp[ch] ? outDb : juce::jmax (outDb, outDisp[ch] - 0.67f);
        if (outDb >= holdDb[ch]) { holdDb[ch] = outDb; holdT[ch] = 1.5f; }
        else if ((holdT[ch] -= 1.0f / 30.0f) <= 0.0f) holdDb[ch] = juce::jmax (-60.0f, holdDb[ch] - 0.67f);
    }
    const float gr = proc.getMaxGrDb();
    grDisp = gr < grDisp ? gr : juce::jmin (gr, grDisp + 1.0f);
    lufsDisp += 0.25f * (proc.getLufs() - lufsDisp);
    repaint (maxPanel);
    repaint (echoGraph.expanded (4));
    repaint (springGraph.expanded (4));
    repaint (sirWave.slider.getBounds().expanded (4));

    const float f = proc.getDisplayFrequency();
    const auto ft = f < 25.0f && ! lowPass.button.getToggleState() ? juce::String ("OFF")
                  : f >= 1000.0f ? juce::String (f / 1000.0f, 1) + " kHz"
                                 : juce::String ((int) std::round (f)) + " Hz";
    const auto et = juce::String (proc.getDisplayBpm(), 1) + " BPM";
    if (ft != freqText) { freqText = ft; repaint (bigArea.expanded (0, 40)); }
    if (et != echoText) { echoText = et; repaint (echoInfo); }
    if (echoDiv.slider.isVisible() != echoSync.button.getToggleState()) updateSyncVisibility(); // automação/preset
    if (bgStepped != stepped.button.getToggleState() || bgSync != echoSync.button.getToggleState()) { renderBackground(); repaint(); }
}

//==============================================================================
// Escala impressa em volta do knob (marcas + 0 e 10 nas pontas)
void DubPanel::drawKnobScale (juce::Graphics& g, juce::Slider& s, int ticks, bool numbers)
{
    const auto circ = knobCircle (s);
    const auto c = circ.getCentre();
    const float r = circ.getWidth() * 0.5f;
    g.setColour (colours::ink.withAlpha (0.85f));
    for (int i = 0; i < ticks; ++i)
    {
        const float a = kStartAngle + (kEndAngle - kStartAngle) * (float) i / (float) (ticks - 1);
        const bool major = numbers ? (i % 5 == 0) : true;
        const float r1 = r + 4.0f, r2 = r + (major ? 10.0f : 7.0f);
        g.drawLine (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1,
                    c.x + std::sin (a) * r2, c.y - std::cos (a) * r2, major ? 1.4f : 0.9f);
    }
    if (numbers)
    {
        g.setFont (stencil (9.0f, 0.0f));
        for (auto [a, t] : { std::pair<float, const char*> { kStartAngle, "0" }, { kEndAngle, "10" } })
            g.drawText (t, juce::Rectangle<float> (18, 10).withCentre ({ c.x + std::sin (a) * (r + 17.0f),
                                                                       c.y - std::cos (a) * (r + 17.0f) }),
                        juce::Justification::centred);
    }
}

void DubPanel::renderBackground()
{
    const float scale = 2.0f; // renderiza em 2x para telas retina
    background = juce::Image (juce::Image::ARGB, (int) (getWidth() * scale), (int) (getHeight() * scale), true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (scale));
    juce::Random rng (1975);
    bgStepped = stepped.button.getToggleState();

    // --- laterais de madeira (envelhecidas)
    for (auto wood : { woodL, woodR })
    {
        const float wx = (float) wood.getX(), ww = (float) wood.getWidth();
        juce::ColourGradient wg (colours::woodLight.darker (0.1f), wx, 0, colours::woodDark, wx + ww, 0, false);
        wg.addColour (0.35, colours::woodLight.darker (0.25f));
        wg.addColour (0.7,  colours::woodLight.darker (0.45f));
        g.setGradientFill (wg);
        g.fillRect (wood);
        for (int i = 0; i < 40; ++i)                             // veios
        {
            juce::Path grain;
            const float xx = wx + rng.nextFloat() * ww, ph = rng.nextFloat() * 6.0f, amp = 0.8f + rng.nextFloat() * 2.2f;
            grain.startNewSubPath (xx, 0);
            for (int yy = 0; yy <= getHeight(); yy += 14)
                grain.lineTo (xx + std::sin ((float) yy * 0.011f + ph) * amp, (float) yy);
            g.setColour ((i % 3 ? juce::Colours::black : colours::woodLight.brighter (0.3f)).withAlpha (0.10f + rng.nextFloat() * 0.16f));
            g.strokePath (grain, juce::PathStrokeType (0.5f + rng.nextFloat() * 1.5f));
        }
        {                                                        // nó da madeira
            const juce::Point<float> kc (wx + ww * (0.35f + rng.nextFloat() * 0.3f), (float) getHeight() * (0.25f + rng.nextFloat() * 0.5f));
            for (int k = 5; k >= 1; --k)
            {
                g.setColour (colours::woodDark.darker (0.4f).withAlpha (0.18f));
                g.drawEllipse (juce::Rectangle<float> ((float) k * 2.6f, (float) k * 5.5f).withCentre (kc), 0.9f);
            }
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillEllipse (juce::Rectangle<float> (3.0f, 6.0f).withCentre (kc));
        }
        for (int i = 0; i < 18; ++i)                             // marcas e amassados
        {
            const float yy = rng.nextFloat() * (float) getHeight();
            g.setColour (juce::Colours::black.withAlpha (0.18f + rng.nextFloat() * 0.2f));
            g.drawLine (wx + rng.nextFloat() * ww, yy, wx + rng.nextFloat() * ww, yy + rng.nextFloat() * 4.0f, 0.6f);
        }
        juce::ColourGradient edge (juce::Colours::white.withAlpha (0.10f), wx, 0, juce::Colours::transparentWhite, wx + 4.0f, 0, false);
        g.setGradientFill (edge);                                // quina gasta clareada
        g.fillRect (wood.withWidth (4));
        juce::ColourGradient ys (juce::Colours::black.withAlpha (0.35f), 0, 0, juce::Colours::transparentBlack, 0, 40.0f, false);
        g.setGradientFill (ys);
        g.fillRect (wood.withHeight (40));
        juce::ColourGradient yb (juce::Colours::transparentBlack, 0, (float) getHeight() - 40.0f, juce::Colours::black.withAlpha (0.35f), 0, (float) getHeight(), false);
        g.setGradientFill (yb);
        g.fillRect (wood.withTrimmedTop (getHeight() - 40));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRect (wood, 1);
    }

    // --- painel de metal pintado, amarelado pelo tempo
    juce::ColourGradient pg (colours::plate.brighter (0.03f), (float) plate.getX(), (float) plate.getY(),
                             colours::plateDark.darker (0.06f), (float) plate.getRight(), (float) plate.getBottom(), false);
    pg.addColour (0.5, colours::plate.interpolatedWith (juce::Colour (0xffd9c48f), 0.35f));
    g.setGradientFill (pg);
    g.fillRect (plate);

    auto randIn = [&] (juce::Rectangle<int> r)
    { return juce::Point<float> ((float) r.getX() + rng.nextFloat() * (float) r.getWidth(), (float) r.getY() + rng.nextFloat() * (float) r.getHeight()); };

    // amarelado irregular (nicotina / sol)
    for (int i = 0; i < 22; ++i)
    {
        const float rr = 60.0f + rng.nextFloat() * 220.0f;
        const auto pc = randIn (plate);
        juce::ColourGradient y (juce::Colour (0xffc9a64e).withAlpha (0.07f + rng.nextFloat() * 0.05f), pc.x, pc.y,
                                juce::Colours::transparentBlack, pc.x + rr, pc.y, true);
        g.setGradientFill (y);
        g.fillEllipse (juce::Rectangle<float> (rr * 2, rr * 1.4f).withCentre (pc));
    }
    // granulado
    for (int i = 0; i < 70000; ++i)
    {
        const auto pt = randIn (plate);
        g.setColour ((rng.nextBool() ? juce::Colours::black : juce::Colours::white).withAlpha (0.03f + rng.nextFloat() * 0.07f));
        g.fillRect (pt.x, pt.y, 1.0f, 1.0f);
    }
    // manchas marrons de uso
    for (int i = 0; i < 34; ++i)
    {
        const float rr = 10.0f + rng.nextFloat() * 90.0f;
        const auto pc = randIn (plate);
        juce::ColourGradient stain (juce::Colour (0xff7a5a2c).withAlpha (0.05f + rng.nextFloat() * 0.07f), pc.x, pc.y,
                                    juce::Colours::transparentBlack, pc.x + rr, pc.y, true);
        g.setGradientFill (stain);
        g.fillEllipse (juce::Rectangle<float> (rr * 2, rr * 2).withCentre (pc));
    }
    // marcas d'água (anéis de copo)
    for (int i = 0; i < 3; ++i)
    {
        const float rr = 22.0f + rng.nextFloat() * 14.0f;
        const auto pc = randIn (plate.reduced (60));
        g.setColour (juce::Colour (0xff6b4d22).withAlpha (0.10f));
        g.drawEllipse (juce::Rectangle<float> (rr * 2, rr * 2).withCentre (pc), 1.6f);
        g.setColour (juce::Colour (0xff6b4d22).withAlpha (0.05f));
        g.drawEllipse (juce::Rectangle<float> (rr * 2 - 5, rr * 2 - 5).withCentre (pc), 3.0f);
    }
    // riscos finos
    for (int i = 0; i < 160; ++i)
    {
        const auto p0 = randIn (plate);
        const float ang = rng.nextFloat() * 3.14159f, len = 4.0f + rng.nextFloat() * 38.0f;
        g.setColour ((rng.nextFloat() < 0.6f ? juce::Colours::white : juce::Colour (0xff4a3a24)).withAlpha (0.10f + rng.nextFloat() * 0.18f));
        g.drawLine (p0.x, p0.y, p0.x + std::cos (ang) * len, p0.y + std::sin (ang) * len, 0.5f);
    }
    // lascas de pintura mostrando o metal
    for (int i = 0; i < 40; ++i)
    {
        const auto pc = randIn (plate);
        const float w = 1.0f + rng.nextFloat() * 4.0f, h = 1.0f + rng.nextFloat() * 3.0f;
        g.setColour (juce::Colour (0xff6e6a62).withAlpha (0.55f));
        g.fillEllipse (pc.x, pc.y, w, h);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawEllipse (pc.x - 0.5f, pc.y - 0.5f, w + 1.0f, h + 1.0f, 0.4f);
    }
    // sujeira acumulada nas bordas do painel
    {
        const auto pf = plate.toFloat();
        const float d = 34.0f;
        const juce::Colour grime (0xff3d2b14);
        auto band = [&] (juce::Rectangle<float> r, juce::Point<float> from, juce::Point<float> to)
        {
            juce::ColourGradient eg (grime.withAlpha (0.30f), from.x, from.y, grime.withAlpha (0.0f), to.x, to.y, false);
            g.setGradientFill (eg);
            g.fillRect (r);
        };
        band (pf.withWidth (d),                       { pf.getX(), 0 },              { pf.getX() + d, 0 });
        band (pf.withLeft (pf.getRight() - d),        { pf.getRight(), 0 },          { pf.getRight() - d, 0 });
        band (pf.withHeight (d),                      { 0, pf.getY() },              { 0, pf.getY() + d });
        band (pf.withTop (pf.getBottom() - d),        { 0, pf.getBottom() },         { 0, pf.getBottom() - d });
    }
    // vinheta
    juce::ColourGradient vig (juce::Colours::transparentBlack, (float) plate.getCentreX(), (float) plate.getCentreY(),
                              juce::Colours::black.withAlpha (0.28f), (float) plate.getX(), (float) plate.getY(), true);
    g.setGradientFill (vig);
    g.fillRect (plate);

    // parafusos
    auto screw = [&] (float sx, float sy)
    {
        for (int k = 0; k < 4; ++k)  // ferrugem escorrida em volta
        {
            const float rr = 5.0f + rng.nextFloat() * 6.0f;
            const juce::Point<float> rc (sx + (rng.nextFloat() - 0.5f) * 8.0f, sy + rng.nextFloat() * 9.0f);
            juce::ColourGradient rust (juce::Colour (0xff8a4a1c).withAlpha (0.35f), rc.x, rc.y, juce::Colours::transparentBlack, rc.x + rr, rc.y, true);
            g.setGradientFill (rust);
            g.fillEllipse (juce::Rectangle<float> (rr * 2, rr * 2).withCentre (rc));
        }
        const auto r = juce::Rectangle<float> (11, 11).withCentre ({ sx, sy });
        juce::ColourGradient sg (juce::Colour (0xffe0ded8), r.getX(), r.getY(), juce::Colour (0xff5e5b55), r.getRight(), r.getBottom(), false);
        g.setGradientFill (sg);
        g.fillEllipse (r);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawEllipse (r, 0.8f);
        const float a = rng.nextFloat() * 3.14f;
        g.drawLine (sx - std::cos (a) * 4.0f, sy - std::sin (a) * 4.0f, sx + std::cos (a) * 4.0f, sy + std::sin (a) * 4.0f, 1.4f);
    };
    const float m = 12.0f;
    screw ((float) plate.getX() + m, (float) plate.getY() + m);
    screw ((float) plate.getRight() - m, (float) plate.getY() + m);
    screw ((float) plate.getX() + m, (float) plate.getBottom() - m);
    screw ((float) plate.getRight() - m, (float) plate.getBottom() - m);

    // --- cabeçalho
    g.setColour (colours::ink);
    g.setFont (stencil (30.0f, 0.22f));
    g.drawText ("DUBMASTOR BIG KNOB", plate.getX() + 32, 20, 720, 34, juce::Justification::left);
    const float titleRight = (float) plate.getX() + 32.0f
                           + juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), "DUBMASTOR BIG KNOB");

    // --- placa metálica antiga "DUB STATION" (latão escovado, rebites, letras gravadas)
    {
        g.setFont (stencil (15.0f, 0.34f));
        const juce::String t ("DUB STATION");
        const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), t);
        auto pl = juce::Rectangle<float> (titleRight + 26.0f, 21.0f, tw + 64.0f, 32.0f);

        // sombra no painel
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (pl.translated (1.5f, 3.0f), 3.0f);

        // corpo de latão
        juce::ColourGradient brass (juce::Colour (0xffe3cd85), pl.getX(), pl.getY(),
                                    juce::Colour (0xff8a6d2c), pl.getX(), pl.getBottom(), false);
        brass.addColour (0.45, juce::Colour (0xffc7a95a));
        brass.addColour (0.55, juce::Colour (0xffb39446));
        g.setGradientFill (brass);
        g.fillRoundedRectangle (pl, 3.0f);

        // escovado horizontal
        for (int i = 0; i < 70; ++i)
        {
            const float yy = pl.getY() + 1.0f + rng.nextFloat() * (pl.getHeight() - 2.0f);
            const float x0 = pl.getX() + rng.nextFloat() * pl.getWidth() * 0.5f;
            g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colour (0xff5a4316)).withAlpha (0.06f + rng.nextFloat() * 0.08f));
            g.drawHorizontalLine ((int) yy, x0, juce::jmin (pl.getRight() - 1.0f, x0 + 20.0f + rng.nextFloat() * pl.getWidth() * 0.6f));
        }
        // pátina e manchas (só dentro da placa)
        g.saveState();
        {
            juce::Path clip; clip.addRoundedRectangle (pl, 3.0f);
            g.reduceClipRegion (clip);
        }
        for (int i = 0; i < 9; ++i)
        {
            const float rr = 3.0f + rng.nextFloat() * 9.0f;
            const juce::Point<float> pc (pl.getX() + rng.nextFloat() * pl.getWidth(), pl.getY() + rng.nextFloat() * pl.getHeight());
            juce::ColourGradient pat (juce::Colour (0xff4f5a2a).withAlpha (0.22f), pc.x, pc.y,
                                      juce::Colours::transparentBlack, pc.x + rr, pc.y, true);
            g.setGradientFill (pat);
            g.fillEllipse (juce::Rectangle<float> (rr * 2, rr * 2).withCentre (pc));
        }
        // riscos de uso
        for (int i = 0; i < 6; ++i)
        {
            const float x0 = pl.getX() + rng.nextFloat() * pl.getWidth(), y0 = pl.getY() + rng.nextFloat() * pl.getHeight();
            g.setColour (juce::Colours::white.withAlpha (0.18f));
            g.drawLine (x0, y0, x0 + 6.0f + rng.nextFloat() * 14.0f, y0 + (rng.nextFloat() - 0.5f) * 5.0f, 0.5f);
        }
        g.restoreState();

        // chanfro: luz em cima/esquerda, sombra embaixo/direita
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawLine (pl.getX() + 2, pl.getY() + 1, pl.getRight() - 2, pl.getY() + 1, 1.0f);
        g.drawLine (pl.getX() + 1, pl.getY() + 2, pl.getX() + 1, pl.getBottom() - 2, 1.0f);
        g.setColour (juce::Colour (0xff3d2c0c).withAlpha (0.7f));
        g.drawLine (pl.getX() + 2, pl.getBottom() - 1, pl.getRight() - 2, pl.getBottom() - 1, 1.2f);
        g.drawLine (pl.getRight() - 1, pl.getY() + 2, pl.getRight() - 1, pl.getBottom() - 2, 1.2f);
        g.setColour (juce::Colour (0xff3d2c0c).withAlpha (0.55f));
        g.drawRoundedRectangle (pl, 3.0f, 0.8f);
        // filete gravado interno
        auto inner = pl.reduced (24.0f, 5.0f);
        g.setColour (juce::Colour (0xff4a3712).withAlpha (0.55f));
        g.drawRoundedRectangle (inner, 2.0f, 0.8f);
        g.setColour (juce::Colours::white.withAlpha (0.3f));
        g.drawRoundedRectangle (inner.translated (0.8f, 0.8f), 2.0f, 0.6f);

        // rebites
        for (float rx : { pl.getX() + 12.0f, pl.getRight() - 12.0f })
        {
            const auto rv = juce::Rectangle<float> (9, 9).withCentre ({ rx, pl.getCentreY() });
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillEllipse (rv.translated (0.8f, 1.2f));
            juce::ColourGradient rg (juce::Colour (0xfff0dfa6), rv.getX(), rv.getY(), juce::Colour (0xff6b5220), rv.getRight(), rv.getBottom(), false);
            g.setGradientFill (rg);
            g.fillEllipse (rv);
            g.setColour (juce::Colours::white.withAlpha (0.6f));
            g.fillEllipse (rv.reduced (3.0f).translated (-1.0f, -1.0f));
        }

        // letras gravadas: realce claro deslocado + tinta escura
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawText (t, pl.translated (0.8f, 1.0f), juce::Justification::centred);
        g.setColour (juce::Colour (0xff2e2108));
        g.drawText (t, pl, juce::Justification::centred);
    }
    g.setColour (colours::inkSoft);
    g.setFont (stencil (10.5f, 0.25f));
    g.drawText ("KING TUBBY TRIBUTE   /   V2.0", plate.getX() + 34, 52, 500, 14, juce::Justification::left);

    // faixas pintadas (vermelho / dourado / verde), desgastadas
    {
        auto stripe = juce::Rectangle<float> ((float) plate.getRight() - 330.0f, 30.0f, 290.0f, 7.0f);
        for (auto col : { colours::jamGreen, colours::jamGold, colours::jamBlack })
        {
            g.setColour (col.withAlpha (0.9f));
            g.fillRect (stripe);
            g.setColour (colours::plate.withAlpha (0.25f));
            for (int i = 0; i < 40; ++i)
                g.fillRect (stripe.getX() + rng.nextFloat() * stripe.getWidth(), stripe.getY() + rng.nextFloat() * stripe.getHeight(), 2.0f, 1.0f);
            stripe.translate (0, 9.0f);
        }
    }

    // --- seções: moldura gravada + plaqueta preta com o nome
    auto section = [&] (juce::Rectangle<int> r, const juce::String& title)
    {
        auto rf = r.toFloat();
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawRoundedRectangle (rf.translated (1, 1), 4.0f, 1.0f);
        g.setColour (colours::ink.withAlpha (0.55f));
        g.drawRoundedRectangle (rf, 4.0f, 1.2f);

        g.setFont (stencil (11.5f, 0.22f));
        const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title) + 22.0f;
        auto tag = juce::Rectangle<float> (rf.getX() + 12, rf.getY() - 9, tw, 18);
        g.setColour (colours::labelBg);
        g.fillRoundedRectangle (tag, 2.0f);
        g.setColour (colours::cream);
        g.drawText (title, tag, juce::Justification::centred);
    };
    section (filterPanel, "FILTER");
    section (echoPanel,   "TAPE ECHO");
    section (springPanel, "SPRING");
    section (masterPanel, "MASTER");
    section (sirenPanel,  "DUB SIREN");
    section (maxPanel,    "MAXIMIZER");
    section (vuPanel,     "OUTPUT LEVEL");

    // --- desgaste em volta dos knobs mais usados
    for (auto* sl : { &bigKnob, &echoDiv.slider, &echoFb.slider, &echoSend.slider, &spLevel.slider, &out.slider, &sirPitch.slider })
    {
        const auto kc = knobCircle (*sl);
        const float rr = kc.getWidth() * 0.5f * 1.55f;
        juce::ColourGradient wear (juce::Colour (0xff5a4220).withAlpha (0.0f), kc.getCentreX(), kc.getCentreY(),
                                   juce::Colour (0xff5a4220).withAlpha (0.0f), kc.getCentreX() + rr, kc.getCentreY(), true);
        wear.addColour (0.62, juce::Colour (0xff5a4220).withAlpha (0.12f));
        g.setGradientFill (wear);
        g.fillEllipse (juce::Rectangle<float> (rr * 2, rr * 2).withCentre (kc.getCentre()));
    }

    // --- escalas dos knobs
    for (auto* k : { &res, &drive, &fmix, &echoFb, &echoTone, &echoWow, &echoSend,
                     &spDecay, &spTone, &spLevel, &dry, &out })
        drawKnobScale (g, k->slider, 11, true);
    drawKnobScale (g, echoHeads.slider, 7, false);
    for (auto* k : { &sirPitch, &sirRate, &sirDepth, &sirDly, &sirFb, &sirLevel })
        drawKnobScale (g, k->slider, 11, true);
    {   // placa de destaque do seletor: fundo creme mais claro com moldura vermelha
        auto cell = sirWave.slider.getBounds().toFloat().reduced (2.0f, 0.0f).withTop ((float) sirWave.label.getY() - 4.0f).withTrimmedBottom (-6.0f);
        g.setColour (juce::Colour (0xffe8dbb9));
        g.fillRoundedRectangle (cell, 6.0f);
        g.saveState();
        {
            juce::Path clip; clip.addRoundedRectangle (cell, 6.0f);
            g.reduceClipRegion (clip);
        }
        for (int i = 0; i < 2500; ++i)   // granulado
        {
            g.setColour ((rng.nextBool() ? juce::Colours::black : juce::Colours::white).withAlpha (0.04f + rng.nextFloat() * 0.07f));
            g.fillRect (cell.getX() + rng.nextFloat() * cell.getWidth(), cell.getY() + rng.nextFloat() * cell.getHeight(), 1.0f, 1.0f);
        }
        for (int i = 0; i < 6; ++i)      // manchas
        {
            const float rr = 8.0f + rng.nextFloat() * 30.0f;
            const juce::Point<float> pc (cell.getX() + rng.nextFloat() * cell.getWidth(), cell.getY() + rng.nextFloat() * cell.getHeight());
            juce::ColourGradient st (juce::Colour (0xff8a6a3a).withAlpha (0.10f), pc.x, pc.y, juce::Colours::transparentBlack, pc.x + rr, pc.y, true);
            g.setGradientFill (st);
            g.fillEllipse (juce::Rectangle<float> (rr * 2, rr * 2).withCentre (pc));
        }
        juce::ColourGradient eg (juce::Colours::transparentBlack, cell.getCentreX(), cell.getCentreY(),
                                 juce::Colour (0xff3d2b14).withAlpha (0.22f), cell.getX(), cell.getY(), true);
        g.setGradientFill (eg);
        g.fillRect (cell);
        g.restoreState();
        g.setColour (colours::jamGreen.withAlpha (0.85f));
        g.drawRoundedRectangle (cell, 6.0f, 1.6f);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawRoundedRectangle (cell.reduced (2.5f), 4.5f, 0.8f);
    }
    for (auto* k : { &eqEHp, &eqEMf, &eqEMg, &eqELp, &eqSHp, &eqSMf, &eqSMg, &eqSLp })
        drawKnobScale (g, k->slider, 11, false);
    drawKnobScale (g, maxCeil.slider, 11, false);

    // divisória gravada + "RETURN EQ" acima dos visores
    for (auto gr : { echoGraph, springGraph })
    {
        const float y = (float) gr.getY() - 14.0f;
        g.setColour (colours::ink.withAlpha (0.4f));
        g.drawHorizontalLine ((int) y, (float) gr.getX(), (float) gr.getRight());
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.drawHorizontalLine ((int) y + 1, (float) gr.getX(), (float) gr.getRight());
        g.setFont (stencil (9.0f, 0.3f));
        const juce::String t ("RETURN EQ");
        const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), t) + 14.0f;
        auto tr = juce::Rectangle<float> ((float) gr.getCentreX() - tw * 0.5f, y - 6.0f, tw, 12.0f);
        g.setColour (colours::plate);
        g.fillRect (tr);
        g.setColour (colours::inkSoft);
        g.drawText (t, tr, juce::Justification::centred);
    }
    bgSync = echoSync.button.getToggleState();
    if (bgSync) drawKnobScale (g, echoDiv.slider, 7, false);
    else        drawKnobScale (g, echoMs.slider, 11, true);

    // --- degraus do knob grande
    const auto bc = knobCircle (bigKnob);
    const auto c = bc.getCentre();
    const float kr = bc.getWidth() * 0.5f;
    for (size_t i = 0; i < dub::kSteps.size(); ++i)
    {
        const float pos = (float) i / (float) (dub::kSteps.size() - 1);
        const float a = kStartAngle + pos * (kEndAngle - kStartAngle);
        const float hz = dub::kSteps[i];
        const juce::String lbl = i == 0 ? "OFF" : hz >= 1000.0f ? juce::String (hz / 1000.0f, hz == 7500.0f ? 1 : 0) + "k"
                                                                  : juce::String ((int) hz);
        g.setColour (colours::ink.withAlpha (bgStepped ? 0.9f : 0.35f));
        g.drawLine (c.x + std::sin (a) * (kr + 5), c.y - std::cos (a) * (kr + 5),
                    c.x + std::sin (a) * (kr + 13), c.y - std::cos (a) * (kr + 13), 1.8f);
        g.setFont (stencil (11.0f, 0.04f));
        g.drawText (lbl, juce::Rectangle<float> (40, 14).withCentre ({ c.x + std::sin (a) * (kr + 26), c.y - std::cos (a) * (kr + 26) }),
                    juce::Justification::centred);
    }
    g.setColour (colours::inkSoft);
    g.setFont (stencil (9.0f, 0.3f));
    g.drawText ("CUTOFF  -  Hz", juce::Rectangle<float> (200, 12).withCentre ({ c.x, bc.getY() - 44 }), juce::Justification::centred);
}

void DubPanel::paint (juce::Graphics& g)
{
    g.fillAll (colours::woodDark);
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());

    // displays âmbar (vidro escuro + brilho)
    auto glowText = [&] (juce::Rectangle<float> r, const juce::String& t, float size)
    {
        g.setColour (juce::Colour (0xff0c0907));
        g.fillRoundedRectangle (r.expanded (3), 4.0f);
        juce::ColourGradient gl (colours::glass.brighter (0.15f), r.getX(), r.getY(), colours::glass, r.getX(), r.getBottom(), false);
        g.setGradientFill (gl);
        g.fillRoundedRectangle (r, 3.0f);
        g.setFont (stencil (size, 0.1f));
        g.setColour (colours::amber.withAlpha (0.18f));
        for (auto o : { juce::Point<float> (-1, 0), { 1, 0 }, { 0, -1 }, { 0, 1 } })
            g.drawText (t, r.translated (o.x, o.y), juce::Justification::centred);
        g.setColour (colours::amber);
        g.drawText (t, r, juce::Justification::centred);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f), 3.0f);
    };

    const auto bc = knobCircle (bigKnob);
    glowText (juce::Rectangle<float> (104, 24).withCentre ({ bc.getCentreX(), bc.getBottom() + 44.0f }), freqText, 15.0f);
    glowText (echoInfo.toFloat(), echoText, 11.5f);

    for (int ch = 0; ch < 2; ++ch)
        drawVu (g, vuArea[ch].toFloat(), ch);

    {   // símbolos de onda em volta do seletor; o ativo acende em vermelho
        const auto circ = knobCircle (sirWave.slider);
        const auto c = circ.getCentre();
        const float rr = circ.getWidth() * 0.5f;
        const int active = juce::roundToInt (sirWave.slider.getValue());
        for (int i = 0; i < 5; ++i)
        {
            const float a = kSelStart + (kSelEnd - kSelStart) * (float) i / 4.0f;
            const juce::Point<float> pc (c.x + std::sin (a) * (rr + 17.0f), c.y - std::cos (a) * (rr + 17.0f));
            const auto ir = juce::Rectangle<float> (17, 10).withCentre (pc);
            const juce::Point<float> d1 (c.x + std::sin (a) * (rr + 4.0f), c.y - std::cos (a) * (rr + 4.0f));
            if (i == active)
            {
                g.setColour (colours::jamGold.withAlpha (0.35f));
                g.fillRoundedRectangle (ir.expanded (3, 3), 3.0f);
                g.setColour (colours::jamGreen.darker (0.2f));
                g.strokePath (wavePath (i, ir), juce::PathStrokeType (2.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
                g.fillEllipse (juce::Rectangle<float> (5, 5).withCentre (d1));
            }
            else
            {
                g.setColour (colours::ink.withAlpha (0.75f));
                g.strokePath (wavePath (i, ir), juce::PathStrokeType (1.4f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
                g.fillEllipse (juce::Rectangle<float> (3, 3).withCentre (d1));
            }
        }
    }

    drawEqGraph (g, echoGraph.toFloat(), false);
    drawEqGraph (g, springGraph.toFloat(), true);
    drawMaximizer (g);
}

// medidor moderno do MAXIMIZER: leituras digitais + barras de LED (IN, GR, OUT)
void DubPanel::drawMaximizer (juce::Graphics& g)
{
    auto glass = [&] (juce::Rectangle<float> r)
    {
        g.setColour (juce::Colour (0xff0c0907));
        g.fillRoundedRectangle (r.expanded (3), 5.0f);
        juce::ColourGradient gl (juce::Colour (0xff1b1712), r.getX(), r.getY(), juce::Colour (0xff0d0b09), r.getX(), r.getBottom(), false);
        g.setGradientFill (gl);
        g.fillRoundedRectangle (r, 4.0f);
    };

    // leituras
    const auto ro = maxReadout.toFloat();
    glass (ro);
    const float outPk = juce::jmax (outDisp[0], outDisp[1]);
    const float push  = -(float) pushSlider.getValue();
    struct Row { const char* label; juce::String value; juce::Colour col; };
    const Row rows[] = {
        { "PUSH",  "+" + juce::String (push, 1) + " dB",                                             colours::jamGold },
        { "GR",    juce::String (grDisp, 1) + " dB",                                                juce::Colour (0xffff9f1c) },
        { "PEAK",  outPk <= -59.0f ? juce::String ("-inf") : juce::String (outPk, 1) + " dB",        outPk > -0.5f ? juce::Colour (0xffff5a47) : juce::Colour (0xff4be08a) },
        { "LUFS-S", lufsDisp <= -69.0f ? juce::String ("--") : juce::String (lufsDisp, 1),            juce::Colour (0xff6fd3ff) } };
    auto rr = ro.reduced (10.0f, 6.0f);
    const float rh = rr.getHeight() / 4.0f;
    for (auto& row : rows)
    {
        auto line = rr.removeFromTop (rh);
        g.setFont (stencil (10.0f, 0.2f));
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawText (row.label, line, juce::Justification::centredLeft);
        g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 15.0f, juce::Font::bold)));
        g.setColour (row.col.withAlpha (0.25f));
        g.drawText (row.value, line.translated (0, 1), juce::Justification::centredRight);
        g.setColour (row.col);
        g.drawText (row.value, line, juce::Justification::centredRight);
    }

    // baia dos medidores
    const auto bay = meterBay.toFloat();
    glass (bay);
    const bool on = maxOn.button.getToggleState();

    // escala dB
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 9.5f, juce::Font::plain)));
    for (float db : { 0.0f, -3.0f, -6.0f, -10.0f, -15.0f, -20.0f, -30.0f })
    {
        const float y = dbToY (db);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawHorizontalLine ((int) y, inX - 4.0f, outX + 40.0f);
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawText (juce::String ((int) db), juce::Rectangle<float> (24, 10).withCentre ({ bay.getX() + 15.0f, y }), juce::Justification::centredRight);
    }

    auto ledBar = [&] (float x, float w, float levelDb, float hold, bool isGr)
    {
        const float top = meterTop, bot = meterBottom;
        for (float y = bot; y > top; y -= 4.0f)
        {
            const float segDb = -30.0f * (y - top) / (bot - top);
            bool lit; juce::Colour c;
            if (isGr) { lit = segDb >= levelDb && levelDb < -0.05f; c = juce::Colour (0xffff9f1c); }
            else
            {
                lit = segDb <= levelDb;
                c = segDb > -1.0f ? juce::Colour (0xffff4a3a) : segDb > -6.0f ? juce::Colour (0xffffd23f) : juce::Colour (0xff2fd673);
            }
            if (isGr) lit = (-30.0f * (y - 3.0f - top) / (bot - top)) >= levelDb && levelDb < -0.05f;
            g.setColour (lit ? c : c.withAlpha (0.10f));
            g.fillRect (x, y - 3.0f, w, 3.0f);
        }
        if (! isGr && hold > -30.0f)
        {
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.fillRect (x, dbToY (hold) - 1.0f, w, 2.0f);
        }
    };

    const float bw = 13.0f, gap = 3.0f;
    ledBar (inX,            bw, inDisp[0], -100.0f, false);
    ledBar (inX + bw + gap, bw, inDisp[1], -100.0f, false);
    ledBar (grX,            16.0f, on ? grDisp : 0.0f, 0.0f, true);
    ledBar (outX,            bw, outDisp[0], holdDb[0], false);
    ledBar (outX + bw + gap, bw, outDisp[1], holdDb[1], false);

    // linha do threshold atravessando os medidores de entrada
    const float ty = dbToY ((float) pushSlider.getValue());
    g.setColour (colours::jamGold.withAlpha (on ? 0.95f : 0.35f));
    g.drawLine (inX - 6.0f, ty, inX + 2 * bw + gap + 3.0f, ty, 1.6f);
    // teto nos medidores de saída
    const float cy = dbToY ((float) maxCeil.slider.getValue());
    g.setColour (juce::Colour (0xffff4a3a).withAlpha (0.8f));
    for (float x = outX - 3.0f; x < outX + 2 * bw + gap + 3.0f; x += 5.0f)
        g.drawLine (x, cy, x + 2.5f, cy, 1.2f);

    // rótulos
    g.setFont (stencil (9.5f, 0.2f));
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    auto lab = [&] (const char* t, float x, float w) { g.drawText (t, juce::Rectangle<float> (x - 6.0f, bay.getY() + 4.0f, w + 12.0f, 12.0f), juce::Justification::centred); };
    lab ("IN", inX, 2 * bw + gap);
    lab ("GR", grX, 16.0f);
    lab ("OUT", outX, 2 * bw + gap);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    auto lab2 = [&] (const char* t, float x) { g.drawText (t, juce::Rectangle<float> (x, meterBottom + 3.0f, bw, 10.0f), juce::Justification::centred); };
    lab2 ("L", inX); lab2 ("R", inX + bw + gap); lab2 ("L", outX); lab2 ("R", outX + bw + gap);

    if (! on)
    {
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (bay, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.setFont (stencil (12.0f, 0.3f));
        g.drawText ("BYPASS", bay, juce::Justification::centred);
    }
}

// visor da curva de EQ: vidro escuro, grade e curva âmbar (20 Hz – 20 kHz, ±15 dB)
void DubPanel::drawEqGraph (juce::Graphics& g, juce::Rectangle<float> r, bool isSpring)
{
    g.setColour (juce::Colour (0xff0c0907));
    g.fillRoundedRectangle (r.expanded (3), 4.0f);
    juce::ColourGradient gl (colours::glass.brighter (0.12f), r.getX(), r.getY(), colours::glass, r.getX(), r.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (r, 3.0f);

    auto xOf = [&] (float f) { return r.getX() + r.getWidth() * std::log (f / 20.0f) / std::log (1000.0f); };
    auto yOf = [&] (float db) { return r.getCentreY() - juce::jlimit (-15.0f, 15.0f, db) / 15.0f * (r.getHeight() * 0.5f - 4.0f); };

    g.setColour (colours::amber.withAlpha (0.12f));
    for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
        g.drawVerticalLine ((int) xOf (f), r.getY() + 2, r.getBottom() - 2);
    for (float db : { -12.0f, 12.0f })
        g.drawHorizontalLine ((int) yOf (db), r.getX() + 2, r.getRight() - 2);
    g.setColour (colours::amber.withAlpha (0.25f));
    g.drawHorizontalLine ((int) yOf (0), r.getX() + 2, r.getRight() - 2);

    g.setFont (stencil (8.0f, 0.0f));
    g.setColour (colours::amber.withAlpha (0.45f));
    for (auto [f, t] : { std::pair<float, const char*> { 100.0f, "100" }, { 1000.0f, "1k" }, { 10000.0f, "10k" } })
        g.drawText (t, juce::Rectangle<float> (24, 9).withCentre ({ xOf (f), r.getBottom() - 6 }), juce::Justification::centred);

    const auto e = proc.getEqSettings (isSpring);
    const float fs = proc.getSampleRate() > 0 ? (float) proc.getSampleRate() : 48000.0f;
    juce::Path curve;
    const int n = juce::jmax (40, (int) r.getWidth());
    for (int i = 0; i <= n; ++i)
    {
        const float f = 20.0f * std::pow (1000.0f, (float) i / (float) n);
        const juce::Point<float> p (xOf (f), yOf (dub::ReturnEQ::magnitudeDb (f, e, fs)));
        if (i == 0) curve.startNewSubPath (p); else curve.lineTo (p);
    }
    g.saveState();
    g.reduceClipRegion (r.toNearestInt());
    g.setColour (colours::amber.withAlpha (0.20f));
    g.strokePath (curve, juce::PathStrokeType (4.0f));
    g.setColour (colours::amber);
    g.strokePath (curve, juce::PathStrokeType (1.6f));
    g.restoreState();

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.45f), 3.0f);
}

void DubPanel::placeEq (juce::Rectangle<int> r, Knob& a, Knob& b, Knob& c, Knob& d, juce::Rectangle<int>& graph)
{
    r.removeFromTop (18);                       // espaço do título "RETURN EQ"
    graph = r.removeFromTop (54).reduced (8, 0);
    r.removeFromTop (6);
    const int w = r.getWidth() / 4;
    placeKnob (a, r.removeFromLeft (w));
    placeKnob (b, r.removeFromLeft (w));
    placeKnob (c, r.removeFromLeft (w));
    placeKnob (d, r);
}

// VU de ponteiro: moldura escura, mostrador creme iluminado, zona vermelha acima de 0
void DubPanel::drawVu (juce::Graphics& g, juce::Rectangle<float> r, int ch)
{
    // moldura
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (r.translated (1, 3), 6.0f);
    juce::ColourGradient bez (juce::Colour (0xff3b3631), r.getX(), r.getY(), juce::Colour (0xff0f0d0b), r.getX(), r.getBottom(), false);
    g.setGradientFill (bez);
    g.fillRoundedRectangle (r, 6.0f);

    auto face = r.reduced (7.0f);
    juce::ColourGradient lit (juce::Colour (0xfffbe8b0), face.getCentreX(), face.getBottom(),
                              juce::Colour (0xffd9bf7a), face.getX(), face.getY(), true);
    g.setGradientFill (lit);
    g.fillRoundedRectangle (face, 3.0f);

    const float aMin = -0.70f, aMax = 0.70f; // radianos a partir da vertical
    const float len = juce::jmin (face.getHeight() * 0.92f, (face.getWidth() * 0.5f - 10.0f) / (std::sin (aMax) * 0.99f));
    const juce::Point<float> pivot (face.getCentreX(), face.getY() + 8.0f + len);
    auto posOf = [] (float vu) { return (std::pow (10.0f, vu / 20.0f) - 0.1f) / (1.4125f - 0.1f); };
    auto ang   = [&] (float p) { return aMin + p * (aMax - aMin); };
    auto pt    = [&] (float a, float rad) { return juce::Point<float> (pivot.x + std::sin (a) * rad, pivot.y - std::cos (a) * rad); };

    g.saveState();
    g.reduceClipRegion (face.toNearestInt());

    // arco vermelho de 0 a +3
    juce::Path redArc;
    redArc.addCentredArc (pivot.x, pivot.y, len * 0.80f, len * 0.80f, 0, ang (posOf (0.0f)), ang (1.0f), true);
    g.setColour (juce::Colour (0xffc0392b));
    g.strokePath (redArc, juce::PathStrokeType (4.0f));
    juce::Path blackArc;
    blackArc.addCentredArc (pivot.x, pivot.y, len * 0.80f, len * 0.80f, 0, ang (0.0f), ang (posOf (0.0f)), true);
    g.setColour (colours::ink);
    g.strokePath (blackArc, juce::PathStrokeType (1.2f));

    g.setFont (stencil (8.0f, 0.0f));
    for (auto [vu, txt] : { std::pair<float, const char*> { -20.0f, "20" }, { -10.0f, "10" }, { -7.0f, "7" }, { -5.0f, "5" },
                            { -3.0f, "3" }, { -2.0f, "" }, { -1.0f, "" }, { 0.0f, "0" }, { 1.0f, "" }, { 2.0f, "" }, { 3.0f, "+3" } })
    {
        const float a = ang (posOf (vu));
        g.setColour (vu > 0 ? juce::Colour (0xffc0392b) : colours::ink);
        g.drawLine ({ pt (a, len * 0.80f), pt (a, len * (txt[0] ? 0.90f : 0.86f)) }, 1.2f);
        if (txt[0])
            g.drawText (txt, juce::Rectangle<float> (20, 10).withCentre (pt (a, len * 0.99f)), juce::Justification::centred);
    }
    g.setColour (colours::ink);
    g.setFont (stencil (13.0f, 0.1f));
    g.drawText ("VU", juce::Rectangle<float> (40, 14).withCentre (pt (0, len * 0.52f)), juce::Justification::centred);
    g.setFont (stencil (9.0f, 0.1f));
    g.drawText (ch == 0 ? "L" : "R", juce::Rectangle<float> (14, 10).withCentre ({ face.getX() + 10, face.getBottom() - 9 }),
                juce::Justification::centred);

    // ponteiro
    const float na = ang (juce::jlimit (-0.04f, 1.08f, needlePos[ch]));
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.drawLine ({ pivot.translated (2, 2), pt (na, len * 0.95f).translated (2, 2) }, 1.6f);
    g.setColour (juce::Colour (0xff1a1410));
    g.drawLine ({ pivot, pt (na, len * 0.95f) }, 1.4f);
    g.restoreState();

    // tampa do pivô
    g.setColour (juce::Colour (0xff1a1714));
    g.fillRect (face.withTop (face.getBottom() - 8).reduced (face.getWidth() * 0.3f, 0));

    // brilho do vidro
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.35f).reduced (3, 2), 3.0f);

    // lâmpada de pico
    const auto lamp = juce::Rectangle<float> (8, 8).withCentre ({ face.getRight() - 9, face.getBottom() - 9 });
    if (peakLamp[ch])
    {
        g.setColour (juce::Colour (0x66ff3b2a));
        g.fillEllipse (lamp.expanded (4));
    }
    g.setColour (peakLamp[ch] ? juce::Colour (0xffff4a35) : juce::Colour (0xff5a1e18));
    g.fillEllipse (lamp);
}

void DubPanel::resized()
{
    auto full = getLocalBounds();
    woodL = full.removeFromLeft (30);
    woodR = full.removeFromRight (30);
    plate = full;

    auto area = plate.reduced (12);
    area.removeFromTop (66);
    maxPanel = area.removeFromRight (236);
    area.removeFromRight (12);
    auto bottomRow = area.removeFromBottom (170);
    area.removeFromBottom (18);
    sirenPanel = bottomRow.removeFromLeft (690);
    bottomRow.removeFromLeft (12);
    vuPanel = bottomRow;

    filterPanel = area.removeFromLeft (380);
    area.removeFromLeft (12);
    echoPanel = area.removeFromLeft (380);
    area.removeFromLeft (12);
    masterPanel = area.removeFromBottom (140);
    area.removeFromBottom (16);
    springPanel = area;

    // FILTER
    {
        auto r = filterPanel.reduced (10);
        r.removeFromTop (14);
        auto knobs = r.removeFromBottom (100);
        auto btns  = r.removeFromBottom (36);
        bigArea = r.withSizeKeepingCentre (300, 300).translated (0, -22);
        bigKnob.setBounds (bigArea);
        const int bw = btns.getWidth() / 3;
        stepped.button.setBounds (btns.removeFromLeft (bw).reduced (3, 2));
        lowPass.button.setBounds (btns.removeFromLeft (bw).reduced (3, 2));
        steep.button.setBounds   (btns.reduced (3, 2));
        const int kw = knobs.getWidth() / 3;
        placeKnob (res,   knobs.removeFromLeft (kw));
        placeKnob (drive, knobs.removeFromLeft (kw));
        placeKnob (fmix,  knobs);
    }

    // TAPE ECHO
    {
        auto r = echoPanel.reduced (10);
        r.removeFromTop (18);
        echoInfo = juce::Rectangle<int> (echoPanel.getRight() - 104, echoPanel.getY() - 9, 88, 18);
        auto row1 = r.removeFromTop (150);
        const int w3 = row1.getWidth() / 3;
        auto t = row1.removeFromLeft (w3);
        placeKnob (echoDiv, t);
        placeKnob (echoMs,  t);
        placeKnob (echoFb,    row1.removeFromLeft (w3));
        placeKnob (echoHeads, row1);
        r.removeFromTop (6);
        auto row2 = r.removeFromTop (100);
        placeKnob (echoTone, row2.removeFromLeft (w3));
        placeKnob (echoWow,  row2.removeFromLeft (w3));
        placeKnob (echoSend, row2);
        r.removeFromTop (10);
        auto btns = r.removeFromTop (46);
        echoSync.button.setBounds (btns.removeFromLeft (w3).reduced (4, 6));
        echoPP.button.setBounds   (btns.removeFromLeft (w3).reduced (4, 6));
        throwBtn.setBounds (btns.reduced (3, 0));
        r.removeFromTop (6);
        placeEq (r, eqEHp, eqEMf, eqEMg, eqELp, echoGraph);
    }

    // SPRING
    {
        auto r = springPanel.reduced (10);
        r.removeFromTop (14);
        auto row = r.removeFromTop (100);
        const int w3 = row.getWidth() / 3;
        placeKnob (spDecay, row.removeFromLeft (w3));
        placeKnob (spTone,  row.removeFromLeft (w3));
        placeKnob (spLevel, row);
        r.removeFromTop (12);
        shakeBtn.setBounds (r.removeFromTop (46).reduced (24, 0));
        r.removeFromTop (6);
        placeEq (r, eqSHp, eqSMf, eqSMg, eqSLp, springGraph);
    }

    // MASTER
    {
        auto r = masterPanel.reduced (10);
        r.removeFromTop (14);
        const int w2 = r.getWidth() / 2;
        placeKnob (dry, r.removeFromLeft (w2));
        placeKnob (out, r);
    }

    // DUB SIREN
    {
        auto r = sirenPanel.reduced (10);
        r.removeFromTop (14);
        auto btns = r.removeFromRight (124);
        const int waveW = 122, kw = (r.getWidth() - waveW) / 6;
        for (auto* k : { &sirPitch, &sirRate, &sirDepth, &sirDly, &sirFb, &sirLevel, &sirWave })
            placeKnob (*k, r.removeFromLeft (k == &sirWave ? waveW : kw).withTrimmedTop (8));
        btns = btns.reduced (6, 4);
        fireBtn.setBounds (btns.removeFromTop (70));
        btns.removeFromTop (10);
        sirHold.button.setBounds (btns.removeFromTop (40).reduced (2, 4));
    }

    // MAXIMIZER
    {
        auto r = maxPanel.reduced (12);
        r.removeFromTop (14);
        maxReadout = r.removeFromTop (96).reduced (2, 0);
        r.removeFromTop (14);
        auto bottom = r.removeFromBottom (118);
        meterBay = r.reduced (2, 0);
        meterTop = (float) meterBay.getY() + 24.0f;
        meterBottom = (float) meterBay.getBottom() - 18.0f;
        const float bx = (float) meterBay.getX();
        const float pushX = bx + 30.0f;
        inX  = pushX + 40.0f;
        grX  = inX + 2 * 13.0f + 3.0f + 16.0f;
        outX = grX + 16.0f + 16.0f;
        const int sy = (int) dbToY (0.0f) - 10, sh = (int) (dbToY (-20.0f) - dbToY (0.0f)) + 20;
        pushSlider.setBounds ((int) pushX, sy, 36, sh);

        bottom.removeFromTop (8);
        auto ceilCell = bottom.removeFromLeft (bottom.getWidth() / 2);
        placeKnob (maxCeil, ceilCell);
        maxOn.button.setBounds (bottom.withSizeKeepingCentre (bottom.getWidth() - 12, 38).translated (0, -8));
    }

    // VU L/R
    {
        auto r = vuPanel.reduced (14);
        r.removeFromTop (12);
        const int w = (r.getWidth() - 12) / 2;
        vuArea[0] = r.removeFromLeft (w).withSizeKeepingCentre (w, juce::jmin (r.getHeight(), 118));
        r.removeFromLeft (12);
        vuArea[1] = r.withSizeKeepingCentre (w, juce::jmin (r.getHeight(), 118));
    }

    renderBackground();
}
