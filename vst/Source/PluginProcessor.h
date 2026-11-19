#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/TapeDSP.h"

namespace ids
{
    static const juce::String drive { "drive" }, bass { "bass" }, treble { "treble" }, mix { "mix" }, inputDb { "inputDb" }, outputDb { "outputDb" }, autoGain { "autoGain" };
}

class TapeHeadProcessor : public juce::AudioProcessor
{
public:
    static constexpr const char* kVersion = "1.0.0";   // igual que VERSION en CMakeLists.txt
    TapeHeadProcessor();
    ~TapeHeadProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "TapeHead"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Presets de usuario (.preset = XML) en Documentos/TapeHead/User Presets
    static juce::File userPresetFolder();
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);
    void loadFactory (int index);
    juce::String currentPresetName { "Default" };

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Medidores para la ventana (hilo de audio → interfaz)
    std::atomic<float> meterDb { -60.0f };   // RMS de la salida del último bloque, en dBFS
    std::atomic<bool> peakHit { false };     // la salida pasó de −1 dBFS
    std::atomic<int> activity { 0 };         // bloques con señal (las bobinas giran)

private:
    float p (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }

    tape::Channel ch[2];
    juce::SmoothedValue<float> sDrive, sBass, sTreble, sMix, sIn, sOut, sAuto;
    double sr = 44100.0;
    float lastDrive = -1.0f, lastBass = -99.0f, lastTreble = -99.0f;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeHeadProcessor)
};
