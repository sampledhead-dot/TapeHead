#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using APF = juce::AudioParameterFloat;
using APB = juce::AudioParameterBool;
using NR  = juce::NormalisableRange<float>;

juce::AudioProcessorValueTreeState::ParameterLayout TapeHeadProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;
    auto pct = juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int) { return juce::String (juce::roundToInt (x * 100)) + " %"; });
    auto db  = juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int) { return (x > 0.04f ? "+" : "") + juce::String (x, 1) + " dB"; });
    auto P = [] (const juce::String& id) { return juce::ParameterID { id, 1 }; };
    v.push_back (std::make_unique<APF> (P (ids::drive), "Drive", NR (0.0f, 1.0f), 0.35f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int) { return juce::String (x * 10.0f, 1); })));
    auto eq = juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int)
        { return std::abs (x) < 0.05f ? juce::String ("0 dB") : (x > 0 ? "+" : "") + juce::String (x, 1) + " dB"; });
    v.push_back (std::make_unique<APF> (P (ids::bass), "Graves", NR (-10.0f, 10.0f, 0.1f), 0.0f, eq));
    v.push_back (std::make_unique<APF> (P (ids::treble), "Agudos", NR (-10.0f, 10.0f, 0.1f), 0.0f, eq));
    v.push_back (std::make_unique<APF> (P (ids::mix), "Mix", NR (0.0f, 1.0f), 1.0f, pct));
    v.push_back (std::make_unique<APF> (P (ids::inputDb), "Input", NR (-18.0f, 18.0f, 0.1f), 0.0f, db));
    v.push_back (std::make_unique<APF> (P (ids::outputDb), "Output", NR (-18.0f, 18.0f, 0.1f), 0.0f, db));
    v.push_back (std::make_unique<APB> (P (ids::autoGain), "Auto-Gain", true));
    return { v.begin(), v.end() };
}

TapeHeadProcessor::TapeHeadProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    setLatencySamples (tape::kLatency);
}

bool TapeHeadProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    const bool inOk  = in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
    const bool outOk = out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
    return inOk && outOk;
}

void TapeHeadProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    setLatencySamples (tape::kLatency);
    for (auto& c : ch) c.prepare (sr);
    for (auto* s : { &sDrive, &sBass, &sTreble, &sMix, &sIn, &sOut, &sAuto }) s->reset (sr, 0.05);
    sDrive.setCurrentAndTargetValue (p (ids::drive));
    sBass.setCurrentAndTargetValue (p (ids::bass));
    sTreble.setCurrentAndTargetValue (p (ids::treble));
    sMix.setCurrentAndTargetValue (p (ids::mix));
    sIn.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p (ids::inputDb)));
    sOut.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p (ids::outputDb)));
    sAuto.setCurrentAndTargetValue (p (ids::autoGain) > 0.5f ? (float) tape::autoGain (p (ids::drive)) : 1.0f);
    lastDrive = -1.0f; lastBass = lastTreble = -99.0f;
}

void TapeHeadProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels(), outCh = getTotalNumOutputChannels();
    const int nProc = (inCh > 1 && outCh > 1) ? 2 : 1;   // entrada mono o salida mono → un canal

    const float drive = p (ids::drive);
    sDrive.setTargetValue (drive);
    sBass.setTargetValue (p (ids::bass));
    sTreble.setTargetValue (p (ids::treble));
    sMix.setTargetValue (p (ids::mix));
    sIn.setTargetValue (juce::Decibels::decibelsToGain (p (ids::inputDb)));
    sOut.setTargetValue (juce::Decibels::decibelsToGain (p (ids::outputDb)));
    sAuto.setTargetValue (p (ids::autoGain) > 0.5f ? (float) tape::autoGain (drive) : 1.0f);

    double e = 0; float pk = 0;
    constexpr int sub = 16;   // los filtros se recalculan cada 16 muestras mientras se mueve un mando
    for (int start = 0; start < n; start += sub)
    {
        const int len = std::min (sub, n - start);
        const float d = sDrive.skip (len), b = sBass.skip (len), t = sTreble.skip (len);
        if (std::abs (d - lastDrive) > 1e-5f || std::abs (b - lastBass) > 1e-4f || std::abs (t - lastTreble) > 1e-4f)
        {
            const auto m = tape::map (d, b, t, sr);
            for (int c = 0; c < nProc; ++c) ch[c].setParams (m);
            lastDrive = d; lastBass = b; lastTreble = t;
        }
        for (int i = start; i < start + len; ++i)
        {
            const float gi = sIn.getNextValue(), go = sOut.getNextValue(), mx = sMix.getNextValue(), ag = sAuto.getNextValue();
            float out[2] = { 0, 0 };
            for (int c = 0; c < nProc; ++c)
            {
                const float x = (inCh > c ? buffer.getSample (c, i) : buffer.getSample (0, i)) * gi;
                const double wet = ch[c].tick (x) * ag, dry = ch[c].delayDry (x);
                out[c] = (float) ((dry * (1.0 - mx) + wet * mx) * go);
            }
            if (nProc == 1) out[1] = out[0];
            if (outCh == 1) buffer.setSample (0, i, out[0]);
            else { buffer.setSample (0, i, out[0]); buffer.setSample (1, i, out[1]); }
            const float a = std::max (std::abs (out[0]), std::abs (out[1]));
            pk = std::max (pk, a); e += 0.5 * ((double) out[0] * out[0] + (double) out[1] * out[1]);
        }
    }
    for (int c = 2; c < outCh; ++c) buffer.clear (c, 0, n);

    const float rmsDb = n > 0 ? juce::Decibels::gainToDecibels ((float) std::sqrt (e / n), -60.0f) : -60.0f;
    meterDb.store (rmsDb);
    if (pk > 0.891f) peakHit.store (true);
    if (rmsDb > -55.0f) activity.fetch_add (1);
}

//==============================================================================
int TapeHeadProcessor::getNumPrograms() { return (int) factoryPresets().size(); }
const juce::String TapeHeadProcessor::getProgramName (int i)
{
    const auto& f = factoryPresets();
    return juce::isPositiveAndBelow (i, (int) f.size()) ? u8 (f[(size_t) i].name) : juce::String();
}
void TapeHeadProcessor::setCurrentProgram (int i) { loadFactory (i); }

void TapeHeadProcessor::loadFactory (int index)
{
    const auto& f = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) f.size())) return;
    // Input, Output y Auto-Gain dependen de la pista: los presets no los tocan
    for (const auto& id : { ids::drive, ids::bass, ids::treble, ids::mix })
        if (auto* r = apvts.getParameter (id)) r->setValueNotifyingHost (r->getDefaultValue());
    for (auto& kv : f[(size_t) index].values)
        if (auto* r = apvts.getParameter (kv.first)) r->setValueNotifyingHost (r->convertTo0to1 (kv.second));
    currentProgram = index;
    currentPresetName = u8 (f[(size_t) index].name);
}

juce::File TapeHeadProcessor::userPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
             .getChildFile ("TapeHead").getChildFile ("User Presets");
}
juce::StringArray TapeHeadProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    for (auto& f : userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.preset"))
        names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}
bool TapeHeadProcessor::saveUserPreset (const juce::String& name)
{
    if (name.trim().isEmpty() || name == "Default") return false;
    auto dir = userPresetFolder(); dir.createDirectory();
    auto xml = apvts.copyState().createXml();
    if (xml == nullptr || ! xml->writeTo (dir.getChildFile (juce::File::createLegalFileName (name) + ".preset"))) return false;
    currentPresetName = name;
    return true;
}
bool TapeHeadProcessor::loadUserPreset (const juce::String& name)
{
    auto xml = juce::XmlDocument::parse (userPresetFolder().getChildFile (name + ".preset"));
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return false;
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    currentPresetName = name;
    return true;
}
bool TapeHeadProcessor::deleteUserPreset (const juce::String& name)
{
    return userPresetFolder().getChildFile (name + ".preset").deleteFile();
}

void TapeHeadProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", currentPresetName, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}
void TapeHeadProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentPresetName = vt.getProperty ("presetName", "Default").toString();
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* TapeHeadProcessor::createEditor() { return new TapeHeadEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TapeHeadProcessor(); }
