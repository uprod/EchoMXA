#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace echomxa
{

// FIG. 2 - Le chemin MIDI : MIDI IN -> le direct (intact) et le rail d'echo
// (DELAY n x RATE au tempo de l'hote -> PITCH -> DECAY) -> les deux vers
// MIDI OUT. Les etats reels (intervalle, nombre d'echos en vol) sont imprimes.
class SchematicDiagram : public juce::Component
{
public:
    explicit SchematicDiagram (EchoProcessor&);

    void paint (juce::Graphics&) override;

private:
    EchoProcessor& processor;

    std::atomic<float>* rate    = nullptr;
    std::atomic<float>* repeats = nullptr;
    std::atomic<float>* decay   = nullptr;
    std::atomic<float>* pitch   = nullptr;
    std::atomic<float>* length  = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SchematicDiagram)
};

}
