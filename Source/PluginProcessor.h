#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "EchoEngine.h"

namespace echomxa
{

// Effet MIDI (categorie "MIDI FX" de Logic) : pas d'audio, du MIDI entre et
// du MIDI sort. L'instrument (SynthMXA ou autre) vient apres dans la chaine.
class EchoProcessor : public juce::AudioProcessor
{
public:
    EchoProcessor();
    ~EchoProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "EchoMXA"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    // Verites d'affichage pour l'editeur (FIG. 1 et FIG. 2, ~30 Hz).
    const EchoEngine& getEngine() const noexcept { return engine; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void pushParameterUpdatesToEngine();

    juce::AudioProcessorValueTreeState apvts;
    EchoEngine engine;

    // Tampon MIDI de sortie, pre-alloue : processBlock ne doit jamais
    // allouer sur le thread audio.
    juce::MidiBuffer midiOut;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoProcessor)
};

}
