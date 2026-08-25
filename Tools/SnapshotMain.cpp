// Outil de capture hors-ecran pour la revue de design : instancie le
// processeur et l'editeur sans peripherique audio ni fenetre, envoie une
// note MIDI et verifie la loi de l'echo a 120 BPM :
//   1. RATE 1/8 (250 ms = 12000 echantillons), 4 REPEATS, PITCH +12, DECAY 30 % :
//      note-ons a 0, 12000, 24000, 36000, 48000 ; notes 60, 72, 84, 96, 108 ;
//      velocites 100, 70, 49, 34, 24 ;
//   2. chaque note-on a son note-off (LENGTH), et le direct traverse intact.
// Puis peint l'editeur en 2x.
//   usage : EchoMXASnapshot <sortie.png> [alt]
//   "alt" : valeurs non par defaut (1/16, 8 echos, descente -2 st, decay fort)

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "../Source/PluginProcessor.h"

#include <vector>

int main (int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "usage: EchoMXASnapshot <sortie.png> [alt]\n";
        return 1;
    }

    juce::ScopedJuceInitialiser_GUI juceInit;

    echomxa::EchoProcessor proc;

    auto setReal = [&proc] (const juce::String& id, float real)
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (proc.getAPVTS().getParameter (id)))
            p->setValueNotifyingHost (p->convertTo0to1 (real));
    };

    const bool alt = argc > 2 && juce::String (argv[2]) == "alt";

    proc.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buffer (1, 512);

    // --- Garde-fou ------------------------------------------------------------------------
    setReal ("rate", 1.0f); setReal ("repeats", 4.0f); setReal ("decay", 0.3f);
    setReal ("pitch", 12.0f); setReal ("length", 0.5f);

    struct Ev { long long t; int note; bool on; int vel; };
    std::vector<Ev> log;
    long long t = 0;
    for (int i = 0; i < 130; ++i)   // 1,4 s : le train de 4 echos tient en 1,0 s + leurs extinctions
    {
        juce::MidiBuffer midi;
        if (i == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
        if (i == 10) midi.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
        buffer.clear();
        proc.processBlock (buffer, midi);
        for (const auto metadata : midi)
        {
            const auto m = metadata.getMessage();
            if (m.isNoteOnOrOff())
                log.push_back ({ t + metadata.samplePosition, m.getNoteNumber(), m.isNoteOn(), m.getVelocity() });
        }
        t += 512;
    }

    std::vector<Ev> ons, offs;
    for (const auto& e : log) (e.on ? ons : offs).push_back (e);

    bool ok = ons.size() == 5 && offs.size() == 5;
    const int expNote[5] = { 60, 72, 84, 96, 108 };
    const int expVel[5]  = { 100, 70, 49, 34, 24 };
    for (size_t k = 0; ok && k < ons.size(); ++k)
    {
        std::cout << "note-on " << ons[k].note << " vel " << ons[k].vel << " @ " << ons[k].t << "\n";
        ok = ons[k].note == expNote[k] && ons[k].vel == expVel[k] && ons[k].t == (long long) k * 12000;
    }
    std::cout << "note-ons: " << ons.size() << "  note-offs: " << offs.size() << "\n";
    if (! ok)
    {
        std::cerr << "ERREUR: le train d'echos ne suit pas la loi\n";
        return 4;
    }

    // --- Reglages de la capture ------------------------------------------------------
    if (alt)
    {
        setReal ("rate", 2.0f);      // 1/16
        setReal ("repeats", 8.0f);
        setReal ("decay", 0.45f);
        setReal ("pitch", -2.0f);
        setReal ("length", 0.9f);
    }
    else
    {
        setReal ("rate", 1.0f);
        setReal ("repeats", 4.0f);
        setReal ("decay", 0.3f);
        setReal ("pitch", 0.0f);
        setReal ("length", 0.5f);
    }

    // Une note (E4), puis on avance jusqu'au milieu du train pour la feuille.
    const int blocksToRun = alt ? 30 : 60;
    for (int i = 0; i < blocksToRun; ++i)
    {
        juce::MidiBuffer midi;
        if (i == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 64, 0.85f), 0);
        if (i == 6) midi.addEvent (juce::MidiMessage::noteOff (1, 64), 0);
        buffer.clear();
        proc.processBlock (buffer, midi);
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    if (editor == nullptr)
        return 2;

    const int w = editor->getWidth();
    const int h = editor->getHeight();

    juce::Image img (juce::Image::ARGB, w * 2, h * 2, true);
    {
        juce::Graphics g (img);
        g.addTransform (juce::AffineTransform::scale (2.0f));
        editor->paintEntireComponent (g, true);
    }

    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);
    out.deleteFile();
    juce::FileOutputStream os (out);
    if (! os.openedOk())
        return 3;

    juce::PNGImageFormat().writeImageToStream (img, os);
    std::cout << "ecrit: " << out.getFullPathName() << " (" << w * 2 << "x" << h * 2 << ")\n";
    return 0;
}
