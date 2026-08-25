#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace echomxa
{

// FIG. 1 - Le train d'echos de la derniere note jouee : l'axe horizontal est
// le temps en intervalles (RATE), l'axe vertical la hauteur (PITCH par
// repetition), la taille de chaque point sa velocite reelle (velocityFor,
// la meme loi que le moteur). Les repetitions deja jouees sont pleines, a
// venir creuses ; la tete de lecture avance. Sans note : le train que la
// prochaine note declencherait.
class ScopePlot : public juce::Component
{
public:
    explicit ScopePlot (EchoProcessor&);

    void paint (juce::Graphics&) override;

private:
    EchoProcessor& processor;

    std::atomic<float>* rate    = nullptr;
    std::atomic<float>* repeats = nullptr;
    std::atomic<float>* decay   = nullptr;
    std::atomic<float>* pitch   = nullptr;
    std::atomic<float>* length  = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopePlot)
};

}
