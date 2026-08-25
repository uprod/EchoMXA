#pragma once

/*  EchoMXA — feuille "Service Manual" de la famille MXA.
    Encre spot vert menthe. FIG. 1 = le train d'echos de la derniere note :
    temps x hauteur, taille = velocite reelle, deja joues pleins, a venir
    creux ; FIG. 2 = le chemin MIDI (direct + rail d'echo : retard, hauteur,
    extinction) ; cinq commandes (RATE, REPEATS, DECAY, PITCH, LENGTH).
    Le gabarit de la feuille (cadre, cartouche, figures, cadrans) suit
    ../PhaserMXA/DESIGN.md, l'autorite de design de la famille.
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ManualStyle.h"
#include "ScopePlot.h"
#include "SchematicDiagram.h"

namespace echomxa
{

class EchoEditor : public juce::AudioProcessorEditor,
                   private juce::Timer
{
public:
    explicit EchoEditor (EchoProcessor& proc);
    ~EchoEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    using APVTS   = juce::AudioProcessorValueTreeState;
    using SAttach = APVTS::SliderAttachment;

    struct Dial
    {
        juce::Slider slider;
        juce::Label  name;
        std::unique_ptr<SAttach> attachment;
    };

    void setupDial (Dial& d, const juce::String& labelText, const juce::String& paramID);
    void setupSwitch (Dial& d, const juce::String& labelText, const juce::String& paramID,
                      const juce::String& componentID);
    void timerCallback() override;

    void drawSheetFrame (juce::Graphics& g);
    void drawHeader (juce::Graphics& g);

    EchoProcessor& echoProcessor;

    ManualLookAndFeel lookAndFeel;
    juce::Image       filmTexture;

    ScopePlot        plot;
    SchematicDiagram schematic;

    Dial rateSwitch, repeatsDial, decayDial, pitchDial, lengthDial;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoEditor)
};

}
