#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace echomxa
{

namespace IDs
{
    constexpr auto rate    = "rate";
    constexpr auto repeats = "repeats";
    constexpr auto decay   = "decay";
    constexpr auto pitch   = "pitch";
    constexpr auto length  = "length";
}

// Duree d'un echo en noires pour chaque cran du commutateur RATE.
static float quartersForRate (int index)
{
    switch (index)
    {
        case 0: return 1.0f;      // 1/4
        case 2: return 0.25f;     // 1/16
        default: return 0.5f;     // 1/8
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout EchoProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    const auto pctAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue() / 100.0f; });

    const auto intAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)); })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });

    const auto stAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int)
        {
            const int s = juce::roundToInt (v);
            return (s > 0 ? "+" : "") + juce::String (s) + " st";
        })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { IDs::rate, 1 }, "Rate",
        juce::StringArray { "1/4", "1/8", "1/16" }, 1));   // defaut = 1/8

    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::repeats, 1 },
        "Repeats", juce::NormalisableRange<float> (1.0f, 8.0f, 1.0f), 4.0f, intAttr));

    // DECAY : perte de velocite par repetition.
    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::decay, 1 },
        "Decay", juce::NormalisableRange<float> (0.0f, 0.9f, 0.01f), 0.3f, pctAttr));

    // PITCH : transposition ajoutee a chaque repetition.
    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::pitch, 1 },
        "Pitch", juce::NormalisableRange<float> (-12.0f, 12.0f, 1.0f), 0.0f, stAttr));

    // LENGTH : duree des echos, en fraction de l'intervalle.
    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::length, 1 },
        "Length", juce::NormalisableRange<float> (0.1f, 1.0f, 0.01f), 0.5f, pctAttr));

    return { params.begin(), params.end() };
}

// Effet MIDI pur : aucun bus audio.
EchoProcessor::EchoProcessor()
    : AudioProcessor (BusesProperties()),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

EchoProcessor::~EchoProcessor() = default;

void EchoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    pushParameterUpdatesToEngine();
    engine.prepare (sampleRate);
    midiOut.ensureSize (8192);
}

void EchoProcessor::releaseResources()
{
    engine.reset();
}

void EchoProcessor::pushParameterUpdatesToEngine()
{
    engine.setRateQuarters (quartersForRate ((int) apvts.getRawParameterValue (IDs::rate)->load()));
    engine.setRepeats (juce::roundToInt (apvts.getRawParameterValue (IDs::repeats)->load()));
    engine.setDecay   (apvts.getRawParameterValue (IDs::decay)->load());
    engine.setPitch   (juce::roundToInt (apvts.getRawParameterValue (IDs::pitch)->load()));
    engine.setLength  (apvts.getRawParameterValue (IDs::length)->load());

    // Le tempo de l'hote fixe l'intervalle des echos.
    double bpm = 120.0;
    if (auto* head = getPlayHead())
        if (auto position = head->getPosition())
            if (auto b = position->getBpm()) bpm = *b;
    engine.setBpm (bpm);
}

void EchoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    pushParameterUpdatesToEngine();

    midiOut.clear();
    engine.process (midi, midiOut, juce::jmax (1, buffer.getNumSamples()));
    midi.swapWith (midiOut);
}

juce::AudioProcessorEditor* EchoProcessor::createEditor()
{
    return new EchoEditor (*this);
}

void EchoProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void EchoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

}

// Point d'entree du plugin JUCE — doit etre au niveau global.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new echomxa::EchoProcessor();
}
