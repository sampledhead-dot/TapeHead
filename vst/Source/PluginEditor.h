#pragma once
#include "PluginProcessor.h"
#include "UpdateBanner.h"

namespace col
{
    // Multipista de casete de los 80 + paleta Sampled Head (naranja, crema, negro)
    const juce::Colour panel (0xff1f1e1c), panel2 (0xff2a2926), line (0xff3a3833), silk (0xffece7dd), ink (0xff111111),
                       metal1 (0xffd9d5cc), metal2 (0xffa9a59d), orange (0xffd9600b), orangeL (0xffef7a22), orangeD (0xffa94a07),
                       cream (0xfff3efe9), red (0xffc8321f), blue (0xff2f5d9e), yellow (0xffe3b320), grey (0xff8d8a84),
                       amber (0xfff6d58a), pen (0xff1b3f8a);
}

class TapeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TapeLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

// Mando con capuchón de color, nombre serigrafiado y valor en ámbar
struct Knob : public juce::Component
{
    Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& label, juce::Colour cap, bool big = false);
    void resized() override;
    void paint (juce::Graphics&) override;
    juce::Slider slider;
    juce::String label, paramId;
    juce::AudioProcessorValueTreeState& state;
    bool big;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class TapeHeadEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TapeHeadEditor (TapeHeadProcessor&);
    ~TapeHeadEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;

    // para las pruebas
    juce::StringArray presetList() const;
    void stepPreset (int dir);
    float vuLevel() const { return vu; }
    void tickForTests() { timerCallback(); }

private:
    void timerCallback() override;
    void showPresetMenu();
    void loadByName (const juce::String& name);
    void askSave();
    void askDelete();
    void drawTop (juce::Graphics&);
    void drawCassette (juce::Graphics&, juce::Rectangle<float>);
    void drawVU (juce::Graphics&, juce::Rectangle<float>);
    void drawCounter (juce::Graphics&);

    TapeHeadProcessor& proc;
    TapeLookAndFeel lnf;
    std::unique_ptr<Knob> kDrive, kBass, kTreble, kMix, kIn, kOut;
    juce::TextButton rewB, ffB, autoB, recB, ejectB;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoAtt;

    juce::Rectangle<int> deckArea { 20, 104, 450, 250 }, vuArea { 490, 104, 350, 214 }, ledArea { 490, 326, 350, 22 }, counterArea { 20, 364, 820, 30 };
    float vu = -20.0f, reelAngle = 0.0f, reelSpeed = 0.0f;
    int lastActivity = 0;
    double lastTime = 0, peakUntil = 0;
public:
    shupdate::UpdateBanner update { "TapeHead", "TapeHead", TapeHeadProcessor::kVersion, col::panel, col::silk, col::orange };
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeHeadEditor)
};
