#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
const juce::Colour cream     (0xffe9e2cc);
const juce::Colour ink       (0xff3a2a1c);   // brown print on the enamel
const juce::Colour amber     (0xffffa630);
const juce::Colour lampOff   (0xff4a3418);

constexpr int kWood = 30;
constexpr int kW = 900 + 2 * kWood, kH = 640;
constexpr int kTopH = 230;

// knob index -> parameter id / label
// 0 mode, 1 rate (GIANT, highlighted) | 2 intensity, 3 echo, 4 reverb (medium)
// 5 input, 6 wow, 7 age, 8 dry, 9 output (small)
constexpr int kNumKnobs = 10, kModeIdx = 0, kNumBig = 2, kNumMedium = 3;
const char* ids[kNumKnobs]    = { "mode", "rate", "intensity", "echo", "reverb",
                                  "input", "wow", "age", "dry", "output" };
const char* labels[kNumKnobs] = { "MODE SELECTOR", "REPEAT RATE", "INTENSITY", "ECHO VOLUME", "REVERB VOLUME",
                                  "INPUT", "WOW/FLUTTER", "TAPE AGE", "DRY", "OUTPUT" };
constexpr int kNumEq = 6;
const char* eqLabels[kNumEq] = { "100", "250", "630", "1.6k", "4k", "10k" };

juce::String modeDescription (int m)
{
    const int mask = fitaeco::TapeEcho::headMask (m);
    juce::StringArray heads;
    for (int h = 0; h < 3; ++h) if (mask & (1 << h)) heads.add (juce::String (h + 1));
    juce::String s = "MODE " + juce::String (m) + "  -  ";
    s << (heads.isEmpty() ? juce::String ("so reverb") : "cabeca " + heads.joinIntoString (" + "));
    if (m >= 5 && m <= 11) s << " + reverb";
    return s;
}
} // namespace

// ------------------------------------------------------------------ look&feel
VintageLookAndFeel::VintageLookAndFeel()
{
    setColour (juce::Label::textColourId, cream);
}

void VintageLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                           float a0, float a1, juce::Slider& s)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const bool stepped = s.getInterval() >= 1.0;
    const int steps = stepped ? (int) (s.getMaximum() - s.getMinimum()) + 1 : 11;
    const int sel = juce::roundToInt (pos * (float) (steps - 1));
    const float ang = stepped ? a0 + (float) sel / (float) (steps - 1) * (a1 - a0) : a0 + pos * (a1 - a0);

    // printed scale on the panel
    g.setFont (juce::FontOptions (juce::jmax (9.0f, r * 0.15f), juce::Font::bold));
    for (int i = 0; i < steps; ++i)
    {
        const float t = a0 + (float) i / (float) (steps - 1) * (a1 - a0);
        const bool lit = stepped && i == sel;
        g.setColour (lit ? amber : cream.withAlpha (0.8f));
        g.drawLine ({ c.getPointOnCircumference (r * 0.68f, t), c.getPointOnCircumference (r * 0.76f, t) }, 1.3f);
        const bool numbered = stepped || (r > 50.0f ? (i % 2 == 0) : (i == 0 || i == steps - 1 || i == steps / 2));
        if (numbered && r > 30.0f)
        {
            const auto np = c.getPointOnCircumference (r * 0.88f, t);
            const float bw = r * 0.3f;
            g.drawText (juce::String (stepped ? i + 1 : i), juce::Rectangle<float> (np.x - bw / 2, np.y - bw / 2, bw, bw),
                        juce::Justification::centred);
        }
    }

    // spun-aluminium skirt
    const float kr = r * 0.62f;
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (c.x - kr + 2, c.y - kr + 4, kr * 2, kr * 2);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeeeeee), c.x - kr, c.y - kr,
                                             juce::Colour (0xff7d7d7d), c.x + kr, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    for (int i = 0; i < 6; ++i) // concentric machining rings
    {
        const float rr = kr * (0.72f + 0.045f * (float) i);
        g.setColour (juce::Colours::white.withAlpha (i % 2 ? 0.10f : 0.0f).overlaidWith (juce::Colours::black.withAlpha (i % 2 ? 0.0f : 0.06f)));
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2, 1.0f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.2f);
    g.setColour (stepped ? amber.darker (0.2f) : juce::Colour (0xff111111)); // index line on the skirt
    g.drawLine ({ c.getPointOnCircumference (kr * 0.70f, ang), c.getPointOnCircumference (kr * 0.98f, ang) },
                juce::jmax (1.8f, r * 0.03f));

    // black ribbed cap
    const float cr = kr * 0.68f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a3a), c.x, c.y - cr,
                                             juce::Colour (0xff070707), c.x, c.y + cr, false));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
    const int ribs = r > 50 ? 30 : 20;
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    for (int i = 0; i < ribs; ++i)
    {
        const float t = (float) i / (float) ribs * juce::MathConstants<float>::twoPi;
        g.drawLine ({ c.getPointOnCircumference (cr * 0.86f, t), c.getPointOnCircumference (cr * 0.99f, t) }, 1.0f);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2c2c), c.x, c.y - cr * 0.8f,
                                             juce::Colour (0xff0e0e0e), c.x, c.y + cr * 0.8f, false));
    g.fillEllipse (c.x - cr * 0.84f, c.y - cr * 0.84f, cr * 1.68f, cr * 1.68f);
    g.setColour (cream);
    g.drawLine ({ c.getPointOnCircumference (cr * 0.35f, ang), c.getPointOnCircumference (cr * 0.80f, ang) },
                juce::jmax (2.0f, r * 0.04f));
}

void VintageLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
                                           float, float, juce::Slider::SliderStyle, juce::Slider& s)
{
    const float cx = (float) x + (float) w * 0.5f;
    const float thumbR = (float) getSliderThumbRadius (s);
    const float top = (float) y + thumbR, bot = (float) (y + h) - thumbR;

    // scale ticks every 6 dB, longer at 0 dB
    for (int i = 0; i <= 4; ++i)
    {
        const float ty = top + (bot - top) * (float) i / 4.0f;
        const float len = i == 2 ? 13.0f : 8.0f;
        g.setColour (cream.withAlpha (i == 2 ? 0.8f : 0.45f));
        g.drawLine (cx - 6 - len, ty, cx - 6, ty, 1.0f);
        g.drawLine (cx + 6, ty, cx + 6 + len, ty, 1.0f);
    }
    // slot
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (cx - 2.5f, top - 4, 5.0f, bot - top + 8, 2.5f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (cx - 2.5f, top - 4, 5.0f, bot - top + 8, 2.5f, 1.0f);

    // chrome fader cap
    const float cw = juce::jmin ((float) w - 6.0f, 30.0f), ch = 18.0f;
    auto cap = juce::Rectangle<float> (cx - cw / 2, sliderPos - ch / 2, cw, ch);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (cap.translated (1.5f, 3.0f), 3.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff0f0f0), cap.getX(), cap.getY(),
                                             juce::Colour (0xff6e6e6e), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    for (float ly : { cap.getY() + 4.0f, cap.getBottom() - 4.0f })
        g.drawLine (cap.getX() + 3, ly, cap.getRight() - 3, ly, 1.0f);
    g.setColour (amber.darker (0.1f));
    g.fillRect (cap.getX() + 2, cap.getCentreY() - 1.0f, cap.getWidth() - 4, 2.0f);
}

// ---------------------------------------------------------------------- editor
FitaEcoEditor::FitaEcoEditor (FitaEcoProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    plateFace  = juce::Typeface::createSystemTypefaceFor (BinaryData::ArchivoBlackRegular_ttf,
                                                          (size_t) BinaryData::ArchivoBlackRegular_ttfSize);
    scriptFace = juce::Typeface::createSystemTypefaceFor (BinaryData::texgyrechorusmediumitalic_otf,
                                                          (size_t) BinaryData::texgyrechorusmediumitalic_otfSize);
    for (int i = 0; i < kNumKnobs; ++i)
    {
        auto& k = knobs[(size_t) i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible (k.slider);
        k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ids[i], k.slider);
        k.label.setText (labels[i], juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        const float fs = i < kNumBig ? 17.0f : (i < kNumBig + kNumMedium ? 13.0f : 9.5f);
        k.label.setFont (juce::FontOptions (fs, juce::Font::bold));
        k.label.setColour (juce::Label::textColourId, i < kNumBig ? amber : cream);
        addAndMakeVisible (k.label);
    }

    for (int b = 0; b < kNumEq; ++b)
    {
        auto& f = eqFaders[(size_t) b];
        f.slider.setSliderStyle (juce::Slider::LinearVertical);
        f.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (f.slider);
        f.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, "eq" + juce::String (b + 1), f.slider);
        f.slider.setDoubleClickReturnValue (true, 0.0);
        f.label.setText (eqLabels[b], juce::dontSendNotification);
        f.label.setJustificationType (juce::Justification::centred);
        f.label.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        f.label.setColour (juce::Label::textColourId, cream);
        addAndMakeVisible (f.label);
    }

    status.setJustificationType (juce::Justification::centredLeft);
    status.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    status.setColour (juce::Label::textColourId, amber);
    addAndMakeVisible (status);

    setSize (kW, kH);
    startTimerHz (30);
}

FitaEcoEditor::~FitaEcoEditor() { setLookAndFeel (nullptr); }

void FitaEcoEditor::timerCallback()
{
    const float rate = proc.apvts.getRawParameterValue ("rate")->load();
    tapePhase += 0.6f + 2.2f * rate;
    shownMode = (int) proc.apvts.getRawParameterValue ("mode")->load();

    juce::String text = modeDescription (shownMode);
    for (int i = 0; i < kNumKnobs; ++i)
    {
        auto& s = knobs[(size_t) i].slider;
        if (s.isMouseOverOrDragging())
        {
            if (i != kModeIdx)
                text = juce::String (labels[i]) + ":  " + s.getTextFromValue (s.getValue());
            break;
        }
    }
    for (int b = 0; b < kNumEq; ++b)
    {
        auto& f = eqFaders[(size_t) b].slider;
        if (f.isMouseOverOrDragging())
            text = "EQ " + juce::String (eqLabels[b]) + "Hz:  " + (f.getValue() > 0 ? "+" : "")
                   + juce::String (f.getValue(), 1) + " dB";
    }
    status.setText (text, juce::dontSendNotification);
    repaint (tapeWindow);
}

void FitaEcoEditor::paint (juce::Graphics& g)
{
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());

    // metal name plate: engraved "DUB STATION", tagline, version in an amber inset
    const auto pl = plateArea;
    auto engraved = [&g] (const juce::String& t, juce::Rectangle<float> r, juce::Justification j, juce::Colour c)
    {
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawText (t, r.translated (0.0f, 1.0f), j);
        g.setColour (c);
        g.drawText (t, r, j);
    };
    g.setFont (juce::FontOptions (plateFace).withHeight (28.0f).withKerningFactor (0.04f));
    engraved ("DUB STATION", { pl.getX() + 16, pl.getY() + 10, pl.getWidth() - 100, 40 },
              juce::Justification::centredLeft, juce::Colour (0xff262626));
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold).withKerningFactor (0.12f));
    engraved (juce::String (juce::CharPointer_UTF8 ("TAPE ECHO  \xc2\xb7  SPRING REVERB")), { pl.getX() + 17, pl.getY() + 54, pl.getWidth() - 100, 14 },
              juce::Justification::centredLeft, juce::Colour (0xff3a3a3a));
    const auto inset = juce::Rectangle<float> (pl.getRight() - 74, pl.getY() + 18, 56, 46);
    g.setFont (juce::FontOptions (plateFace).withHeight (24.0f));
    g.setColour (amber.withAlpha (0.35f));
    g.drawText ("2.1", inset.expanded (1.0f), juce::Justification::centred);
    g.setColour (amber);
    g.drawText ("2.1", inset, juce::Justification::centred);

    // "DubMastor" signature in brown ink with an underline swash
    {
        juce::Graphics::ScopedSaveState ss (g);
        const auto sigPos = juce::Point<float> (pl.getX() + 14, pl.getBottom() + 8);
        g.addTransform (juce::AffineTransform::rotation (-0.06f, sigPos.x, sigPos.y + 30));
        g.setColour (ink);
        g.setFont (juce::FontOptions (scriptFace).withHeight (48.0f));
        g.drawText ("DubMastor", juce::Rectangle<float> (sigPos.x, sigPos.y, 260, 52), juce::Justification::centredLeft);
        juce::Path swash;
        swash.startNewSubPath (sigPos.x + 10, sigPos.y + 54);
        swash.cubicTo (sigPos.x + 80, sigPos.y + 46, sigPos.x + 170, sigPos.y + 60, sigPos.x + 236, sigPos.y + 44);
        g.strokePath (swash, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // origin + pilot lamp
    auto tp = topPanel.toFloat();
    g.setColour (ink.withAlpha (0.85f));
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold).withKerningFactor (0.1f));
    g.drawText ("FEITO NO BRASIL", juce::Rectangle<float> (pl.getX() + 8, tp.getBottom() - 34, 200, 14), juce::Justification::left);
    const auto lamp = juce::Point<float> (pl.getRight() - 22, tp.getBottom() - 27);
    g.setColour (juce::Colour (0xff8a8a8a));
    g.fillEllipse (lamp.x - 13, lamp.y - 13, 26, 26);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffe0a0), lamp.x - 4, lamp.y - 4,
                                             juce::Colour (0xffd2560c), lamp.x + 9, lamp.y + 9, true));
    g.fillEllipse (lamp.x - 9, lamp.y - 9, 18, 18);
    g.setColour (ink);
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("POWER", juce::Rectangle<float> (lamp.x - 78, lamp.y - 7, 58, 14), juce::Justification::right);

    // graphic EQ captions
    auto eq = eqArea.toFloat();
    g.setColour (cream.withAlpha (0.85f));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("GRAPHIC EQ  (ECHO + REVERB)", eq.withHeight (18.0f).withTrimmedLeft (8), juce::Justification::centredLeft);
    if (auto* f0 = &eqFaders[0].slider; f0->getHeight() > 0)
    {
        const float thumbR = (float) lnf.getSliderThumbRadius (*f0);
        const float top = (float) f0->getY() + thumbR, bot = (float) f0->getBottom() - thumbR;
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        const char* dbTxt[] = { "+12", "0", "-12" };
        for (int i = 0; i < 3; ++i)
        {
            const float ty = top + (bot - top) * (float) i / 2.0f;
            g.setColour (cream.withAlpha (i == 1 ? 0.85f : 0.55f));
            g.drawText (dbTxt[i], juce::Rectangle<float> (eq.getX() + 6, ty - 6, 28, 12), juce::Justification::centredRight);
        }
    }

    drawTapeTransport (g);
}

void FitaEcoEditor::drawTapeTransport (juce::Graphics& g)
{
    auto win = tapeWindow.toFloat();
    // recessed smoked-glass window
    g.setColour (juce::Colour (0xff0d0f0e));
    g.fillRoundedRectangle (win, 10.0f);

    const float L = win.getX() + 70, R = win.getRight() - 70;
    const float T = win.getY() + 38, B = win.getY() + 128;
    const float cy = (T + B) * 0.5f, rad = (B - T) * 0.5f;

    // tape loop path
    juce::Path loop;
    loop.addRoundedRectangle (L - rad, T, (R - L) + 2 * rad, B - T, rad);
    g.setColour (juce::Colour (0xff4a2e18));
    g.strokePath (loop, juce::PathStrokeType (7.0f));
    // moving oxide marks (show tape speed)
    g.setColour (juce::Colour (0xff6b4626));
    const float perim = loop.getLength();
    for (float d = std::fmod (tapePhase, 14.0f); d < perim; d += 14.0f)
    {
        const auto pt = loop.getPointAlongPath (d);
        g.fillEllipse (pt.x - 1.5f, pt.y - 1.5f, 3.0f, 3.0f);
    }

    // guide rollers + capstan with pinch roller
    auto roller = [&g] (juce::Point<float> c, float r, float spin)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd0d0d0), c.x - r, c.y - r,
                                                 juce::Colour (0xff6a6a6a), c.x + r, c.y + r, false));
        g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
        g.setColour (juce::Colour (0xff2a2a2a));
        for (int i = 0; i < 3; ++i)
            g.drawLine ({ c, c.getPointOnCircumference (r * 0.8f, spin + (float) i * 2.0944f) }, 1.5f);
        g.fillEllipse (c.x - 3, c.y - 3, 6, 6);
    };
    roller ({ L - rad + 26.0f, cy }, 22.0f, tapePhase * 0.05f);
    roller ({ R + rad - 26.0f, cy }, 22.0f, tapePhase * 0.05f);
    roller ({ R - 20.0f, B + 13.0f }, 9.0f, tapePhase * 0.2f); // capstan (below tape)
    g.setColour (juce::Colour (0xff151515));
    g.fillEllipse (R - 34.0f, B - 26.0f, 28, 28);              // rubber pinch roller

    // heads along the bottom run: ERASE, REC, 1, 2, 3
    const int mask = fitaeco::TapeEcho::headMask (juce::jlimit (1, 12, shownMode));
    const char* hn[] = { "ERASE", "REC", "1", "2", "3" };
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    for (int h = 0; h < 5; ++h)
    {
        const float hx = L + 40.0f + (float) h * ((R - L - 90.0f) / 4.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb8b8b8), hx - 12, B - 2,
                                                 juce::Colour (0xff5e5e5e), hx + 12, B + 20, false));
        g.fillRoundedRectangle (hx - 12, B - 1, 24, 20, 3.0f);
        g.setColour (juce::Colour (0xff1a1a1a));
        g.drawLine (hx, B - 1, hx, B + 4, 1.0f); // head gap
        const bool on = h <= 1 ? (h == 1) : (mask & (1 << (h - 2))) != 0;
        g.setColour (on ? amber : lampOff);
        g.fillEllipse (hx - 4, B + 24, 8, 8);
        g.setColour (cream.withAlpha (0.75f));
        g.drawText (hn[h], juce::Rectangle<float> (hx - 22, B + 33, 44, 11), juce::Justification::centred);
    }

    // spring reverb lamp
    const bool rev = fitaeco::TapeEcho::reverbOn (shownMode);
    const auto rp = juce::Point<float> (win.getRight() - 30.0f, win.getY() + 18.0f);
    g.setColour (rev ? amber : lampOff);
    g.fillEllipse (rp.x - 4, rp.y - 4, 8, 8);
    g.setColour (cream.withAlpha (0.75f));
    g.drawText ("SPRING", juce::Rectangle<float> (rp.x - 60, rp.y - 6, 50, 12), juce::Justification::right);

    // glass reflection + bezel
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), win.getX(), win.getY(),
                                             juce::Colours::transparentWhite, win.getX() + 120, win.getBottom(), false));
    g.fillRoundedRectangle (win, 10.0f);
    g.setColour (juce::Colour (0xff8f8a7c));
    g.drawRoundedRectangle (win.expanded (2.0f), 11.0f, 3.0f);
}

void FitaEcoEditor::resized()
{
    auto content = getLocalBounds().reduced (kWood, 0);
    topPanel = content.removeFromTop (kTopH);
    plateArea = juce::Rectangle<float> ((float) topPanel.getX() + 28.0f, 24.0f, 332.0f, 82.0f);
    bottomPanel = content;
    tapeWindow = topPanel.withTrimmedLeft (380).reduced (24, 22);
    status.setBounds (tapeWindow.getX() + 14, tapeWindow.getY() + 8, 300, 18);

    auto rows = bottomPanel.reduced (10, 0).withTrimmedTop (12).withTrimmedBottom (10);
    auto rowA = rows.removeFromTop (222);
    rows.removeFromTop (8);
    auto rowB = rows;

    auto place = [] (Knob& k, juce::Rectangle<int> cell, int labelH)
    {
        cell = cell.reduced (4);
        const int side = juce::jmin (cell.getWidth(), cell.getHeight() - labelH);
        cell = cell.withSizeKeepingCentre (cell.getWidth(), labelH + side); // label sits right above its knob
        k.label.setBounds (cell.removeFromTop (labelH));
        k.slider.setBounds (cell.withSizeKeepingCentre (side, side));
    };

    // row A: giant MODE + REPEAT RATE, then the medium echo knobs
    for (int i = 0; i < kNumBig; ++i) place (knobs[(size_t) i], rowA.removeFromLeft (240), 34);
    const int wm = rowA.getWidth() / kNumMedium;
    for (int i = kNumBig; i < kNumBig + kNumMedium; ++i) place (knobs[(size_t) i], rowA.removeFromLeft (wm), 18);

    // row B: graphic EQ on the left, small utility knobs on the right
    eqArea = rowB.removeFromLeft (460);
    auto faders = eqArea.reduced (8, 6);
    faders.removeFromTop (16);   // caption
    faders.removeFromLeft (30);  // dB scale
    auto freqRow = faders.removeFromBottom (14);
    const int fw = faders.getWidth() / kNumEq;
    for (int b = 0; b < kNumEq; ++b)
    {
        auto& f = eqFaders[(size_t) b];
        f.slider.setBounds (faders.removeFromLeft (fw).reduced (8, 0));
        f.label.setBounds (freqRow.removeFromLeft (fw));
    }
    const int ws = rowB.getWidth() / (kNumKnobs - kNumBig - kNumMedium);
    for (int i = kNumBig + kNumMedium; i < kNumKnobs; ++i) place (knobs[(size_t) i], rowB.removeFromLeft (ws), 15);

    renderBackground();
}

// ---------------------------------------------------------------- textures
void FitaEcoEditor::renderBackground()
{
    const int S = 2;
    const int W = getWidth() * S, H = getHeight() * S, topH = kTopH * S;
    background = juce::Image (juce::Image::RGB, W, H, false);
    juce::Random rnd (1975);

    {   // enamel (top) + dark painted steel (bottom), per-pixel
        juce::Image::BitmapData bd (background, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < H; ++y)
        {
            const float vy = (float) y / (float) H;
            for (int x = 0; x < W; ++x)
            {
                const float vx = (float) x / (float) W;
                const float n = rnd.nextFloat() - 0.5f;
                if (y < topH)
                {
                    // cream enamel, slightly yellowed and grimy towards the edges
                    const float ey = (float) y / (float) topH;
                    const float edge = juce::jmax (std::pow (std::abs (vx - 0.5f) * 2.0f, 4.0f), std::pow (std::abs (ey - 0.5f) * 2.0f, 4.0f));
                    float k = 1.0f - 0.18f * edge + 0.025f * n + 0.02f * std::sin (vx * 23.0f) * std::sin (ey * 7.0f);
                    bd.setPixelColour (x, y, juce::Colour::fromFloatRGBA (0.85f * k, 0.80f * k * (1.0f - 0.04f * edge), 0.68f * k * (1.0f - 0.10f * edge), 1.0f));
                }
                else
                {
                    float k = 0.12f + 0.015f * n + 0.01f * std::sin (vy * 90.0f + vx * 4.0f);
                    k *= 1.0f - 0.3f * std::pow (std::abs (vx - 0.5f) * 2.0f, 4.0f);
                    bd.setPixelColour (x, y, juce::Colour::fromFloatRGBA (k * 1.08f, k * 0.98f, k * 0.90f, 1.0f));
                }
            }
        }
    }

    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale ((float) S));
    const float w = (float) getWidth(), h = (float) getHeight(), tH = (float) kTopH;

    // enamel wear: chips and stains
    for (int i = 0; i < 40; ++i)
    {
        const float cx = kWood + rnd.nextFloat() * (w - 2 * kWood), cy = rnd.nextFloat() * tH;
        const float r = 0.6f + rnd.nextFloat() * (rnd.nextFloat() < 0.1f ? 3.5f : 1.2f);
        g.setColour (juce::Colour (0xff3b3026).withAlpha (0.35f + 0.4f * rnd.nextFloat()));
        g.fillEllipse (cx - r, cy - r, r * 2, r * 1.6f);
    }
    for (int i = 0; i < 8; ++i)
    {
        const float cx = kWood + rnd.nextFloat() * (w - 2 * kWood), cy = rnd.nextFloat() * tH, r = 25.0f + rnd.nextFloat() * 60.0f;
        auto col = juce::Colour (0xff8a6a3a).withAlpha (0.08f);
        g.setGradientFill (juce::ColourGradient (col, cx, cy, col.withAlpha (0.0f), cx + r, cy, true));
        g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
    }
    // scratches on the dark panel
    for (int i = 0; i < 90; ++i)
    {
        const float x0 = kWood + rnd.nextFloat() * (w - 2 * kWood), y0 = tH + rnd.nextFloat() * (h - tH);
        const float len = 3.0f + rnd.nextFloat() * 22.0f, a = rnd.nextFloat() * juce::MathConstants<float>::pi;
        g.setColour (juce::Colours::white.withAlpha (0.04f + 0.07f * rnd.nextFloat()));
        g.drawLine (x0, y0, x0 + len * std::cos (a), y0 + len * std::sin (a), 0.6f);
    }

    // brushed-aluminium name plate with rivets
    {
        const auto pl = plateArea;
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (pl.translated (2.0f, 3.0f), 4.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffdcdcd8), 0, pl.getY(), juce::Colour (0xff9c9c97), 0, pl.getBottom(), false));
        g.fillRoundedRectangle (pl, 4.0f);
        for (float yy = pl.getY() + 1.0f; yy < pl.getBottom() - 1.0f; yy += 0.5f) // brushing
        {
            g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.04f + 0.05f * rnd.nextFloat()));
            const float x0 = pl.getX() + 2.0f + rnd.nextFloat() * 30.0f, x1 = pl.getRight() - 2.0f - rnd.nextFloat() * 30.0f;
            g.drawLine (x0, yy, x1, yy, 0.5f);
        }
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawLine (pl.getX() + 3, pl.getY() + 1, pl.getRight() - 3, pl.getY() + 1, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (pl, 4.0f, 1.0f);
        for (auto rv : { juce::Point<float> (pl.getX() + 8, pl.getY() + 8), juce::Point<float> (pl.getRight() - 8, pl.getY() + 8),
                         juce::Point<float> (pl.getX() + 8, pl.getBottom() - 8), juce::Point<float> (pl.getRight() - 8, pl.getBottom() - 8) })
        {
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.fillEllipse (rv.x - 3.0f, rv.y - 2.0f, 6.5f, 6.5f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff4f4f4), rv.x - 3, rv.y - 3, juce::Colour (0xff6a6a6a), rv.x + 3, rv.y + 3, false));
            g.fillEllipse (rv.x - 3.0f, rv.y - 3.0f, 6.0f, 6.0f);
        }
        // recessed black window for the version number
        const auto inset = juce::Rectangle<float> (pl.getRight() - 74, pl.getY() + 18, 56, 46);
        g.setColour (juce::Colour (0xff0e0e0e));
        g.fillRoundedRectangle (inset, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawLine (inset.getX() + 2, inset.getBottom() + 0.5f, inset.getRight() - 2, inset.getBottom() + 0.5f, 1.0f);
    }

    // panel seam with chrome trim strip
    g.setColour (juce::Colour (0xff111111));
    g.fillRect ((float) kWood, tH - 2.0f, w - 2.0f * kWood, 6.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd8d8d8), 0, tH - 1, juce::Colour (0xff6c6c6c), 0, tH + 3, false));
    g.fillRect ((float) kWood, tH - 1.0f, w - 2.0f * kWood, 3.0f);

    // engraved dividers (row A / row B, EQ / small knobs)
    auto engrave = [&g] (float x0, float y0, float x1, float y1)
    {
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawLine (x0, y0, x1, y1, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawLine (x0 + (x0 == x1 ? 1.0f : 0.0f), y0 + (y0 == y1 ? 1.0f : 0.0f),
                    x1 + (x0 == x1 ? 1.0f : 0.0f), y1 + (y0 == y1 ? 1.0f : 0.0f), 1.0f);
    };
    const float divY = (float) eqArea.getY() - 4.0f;
    engrave (kWood + 20.0f, divY, w - kWood - 20.0f, divY);
    engrave ((float) eqArea.getRight(), (float) eqArea.getY() + 6.0f, (float) eqArea.getRight(), (float) eqArea.getBottom() - 4.0f);

    // recessed EQ plate
    auto eqPlate = eqArea.toFloat().reduced (4.0f, 2.0f);
    g.setColour (juce::Colour (0xff0b0a09).withAlpha (0.7f));
    g.fillRoundedRectangle (eqPlate, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (eqPlate.translated (0, 1), 6.0f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (eqPlate, 6.0f, 1.0f);

    // highlighted wells behind MODE and REPEAT RATE: recess + chrome bezel + amber glow
    for (int i = 0; i < kNumBig; ++i)
    {
        const auto kb = knobs[(size_t) i].slider.getBounds().toFloat();
        const auto c = kb.getCentre();
        const float rr = kb.getWidth() * 0.5f + 2.0f;
        g.setGradientFill (juce::ColourGradient (amber.withAlpha (0.22f), c.x, c.y, amber.withAlpha (0.0f), c.x + rr + 12.0f, c.y, true));
        g.fillEllipse (c.x - rr - 12.0f, c.y - rr - 12.0f, (rr + 12.0f) * 2, (rr + 12.0f) * 2);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff050505), c.x, c.y - rr,
                                                 juce::Colour (0xff2b2622), c.x, c.y + rr, false));
        g.fillEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe8e8e8), c.x - rr, c.y - rr,
                                                 juce::Colour (0xff4c4c4c), c.x + rr, c.y + rr, false));
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2, 3.5f);
        g.setColour (amber.withAlpha (0.55f));
        g.drawEllipse (c.x - rr - 4.0f, c.y - rr - 4.0f, (rr + 4.0f) * 2, (rr + 4.0f) * 2, 1.2f);
    }

    // screws
    auto screw = [&g, &rnd] (float cx, float cy)
    {
        const float r = 5.5f;
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillEllipse (cx - r + 1, cy - r + 2, r * 2, r * 2);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd0d0d0), cx - r, cy - r,
                                                 juce::Colour (0xff5a5a5a), cx + r, cy + r, false));
        g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
        const float a = rnd.nextFloat() * juce::MathConstants<float>::pi;
        g.setColour (juce::Colour (0xff2a2a2a));
        g.drawLine (cx - r * 0.7f * std::cos (a), cy - r * 0.7f * std::sin (a),
                    cx + r * 0.7f * std::cos (a), cy + r * 0.7f * std::sin (a), 1.5f);
    };
    for (float sy : { 12.0f, tH - 14.0f, h - 12.0f })
    {
        screw (kWood + 12.0f, sy);
        screw (w - kWood - 12.0f, sy);
    }

    // wooden end cheeks
    {
        juce::Image::BitmapData bd (background, juce::Image::BitmapData::readWrite);
        const int woodW = kWood * S;
        for (int side = 0; side < 2; ++side)
        {
            const int x0 = side == 0 ? 0 : W - woodW;
            const float seed = side == 0 ? 1.3f : 4.7f;
            for (int y = 0; y < H; ++y)
                for (int i = 0; i < woodW; ++i)
                {
                    const float fx = (float) i / (float) S, fy = (float) y / (float) S;
                    const float warp = 3.0f * std::sin (fy * 0.012f + seed) + 1.2f * std::sin (fy * 0.047f + seed * 2.0f);
                    const float grain = std::sin ((fx + warp) * 1.7f + seed);
                    const float ring = std::pow (std::abs (std::sin ((fx * 0.35f + warp * 0.6f) + fy * 0.002f)), 6.0f);
                    float r = 0.40f + 0.06f * grain - 0.12f * ring + 0.03f * (rnd.nextFloat() - 0.5f);
                    r *= 0.55f + 0.45f * std::sin (juce::MathConstants<float>::pi * ((float) i + 0.5f) / (float) woodW);
                    r *= 1.0f - 0.15f * std::pow (std::abs ((float) y / (float) H - 0.5f) * 2.0f, 4.0f);
                    bd.setPixelColour (x0 + i, y, juce::Colour::fromFloatRGBA (r * 1.05f, r * 0.62f, r * 0.33f, 1.0f));
                }
        }
    }
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawLine ((float) kWood, 0.0f, (float) kWood, h, 1.5f);
    g.drawLine (w - (float) kWood, 0.0f, w - (float) kWood, h, 1.5f);
}
