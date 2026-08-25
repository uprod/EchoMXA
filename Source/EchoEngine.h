#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>

namespace echomxa
{

// Echo MIDI : chaque note jouee est repetee REPEATS fois, a un intervalle
// cale sur le tempo (RATE), chaque repetition transposee de PITCH demi-tons
// de plus que la precedente et jouee moins fort (DECAY). C'est un delay,
// mais en MIDI : l'instrument REJOUE vraiment les notes, avec sa propre
// attaque — pas de copie du son. La note originale traverse intacte.
//
// Les echos sont des evenements planifies (note-on puis note-off a LENGTH %
// de l'intervalle) qui traversent les blocs. Une note qui ne peut pas exister
// (au-dela de 0..127, ou de velocite nulle) est simplement omise.
class EchoEngine
{
public:
    static constexpr int kMaxRepeats = 8;
    static constexpr int kMaxEvents  = 1024;

    void prepare (double sampleRate);
    void reset();

    void setRateQuarters (float q)  noexcept { rateQ = q; }
    void setRepeats (int n)         noexcept { repeats = juce::jlimit (1, kMaxRepeats, n); }
    void setDecay   (float d01)     noexcept { decay = d01; }
    void setPitch   (int semis)     noexcept { pitch = semis; }
    void setLength  (float l01)     noexcept { length = l01; }
    void setBpm     (double b)      noexcept { bpm = b > 1.0 ? b : 120.0; }

    void process (const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples);

    // LA loi de l'echo — une seule source de verite, partagee avec FIG. 1.
    // Velocite (0..1) et note de la k-ieme repetition (k = 1..REPEATS).
    static float velocityFor (float vel01, int k, float decay01) noexcept
    {
        return vel01 * std::pow (1.0f - juce::jlimit (0.0f, 0.99f, decay01), (float) k);
    }
    static int noteFor (int note, int k, int pitchSemis) noexcept { return note + k * pitchSemis; }

    // Verites d'affichage (FIG. 1, ~30 Hz) : la derniere note jouee et son train.
    int   getUiNote() const noexcept        { return uiNote.load(); }
    float getUiVelocity() const noexcept    { return uiVel.load(); }
    int   getUiFired() const noexcept       { return uiFired.load(); }     // repetitions deja jouees
    float getUiProgress() const noexcept    { return uiProgress.load(); }  // position dans le train, en intervalles
    float getUiBpm() const noexcept         { return uiBpm.load(); }
    int   getUiActive() const noexcept      { return uiActive.load(); }    // echos encore a venir (toutes notes)

private:
    struct Event { int when = 0; int note = -1; juce::uint8 vel = 0; bool on = false; int tag = -1; int k = 0; };

    void schedule (int when, int note, juce::uint8 vel, bool on, int tag, int k);

    double fs = 48000.0;
    double bpm = 120.0;

    Event events[kMaxEvents];
    int   numEvents = 0;

    // Qui fait sonner chaque hauteur : 0 personne, 1 la note originale, 2 un echo.
    juce::uint8 owner[128] = {};

    float rateQ   = 0.5f;
    int   repeats = 4;
    float decay   = 0.3f;
    int   pitch   = 0;
    float length  = 0.5f;

    int   lastTag = 0;               // identifiant du dernier train
    int   uiTag = -1;
    long long samplesSinceLast = 0;
    int   lastInterval = 1;

    std::atomic<int>   uiNote { -1 };
    std::atomic<float> uiVel { 0.0f };
    std::atomic<int>   uiFired { 0 };
    std::atomic<float> uiProgress { 0.0f };
    std::atomic<float> uiBpm { 120.0f };
    std::atomic<int>   uiActive { 0 };
};

}
