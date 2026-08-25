#include "EchoEngine.h"

namespace echomxa
{

void EchoEngine::prepare (double sampleRate)
{
    fs = sampleRate;
    reset();
}

void EchoEngine::reset()
{
    numEvents = 0;
    for (auto& o : owner) o = 0;
    uiNote.store (-1);
    uiFired.store (0);
    uiProgress.store (0.0f);
    uiActive.store (0);
    uiTag = -1;
}

void EchoEngine::schedule (int when, int note, juce::uint8 vel, bool on, int tag, int k)
{
    if (numEvents >= kMaxEvents) return;
    events[numEvents++] = { when, note, vel, on, tag, k };
}

void EchoEngine::process (const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples)
{
    const int interval = juce::jmax (64, (int) ((double) rateQ * 60.0 / bpm * fs));
    const int echoLen  = juce::jmax (32, (int) (length * (float) interval));

    // 1. Le MIDI entrant : l'original traverse, et chaque note-on lance un train.
    for (const auto metadata : in)
    {
        const auto msg = metadata.getMessage();
        const int pos = metadata.samplePosition;

        if (msg.isNoteOn())
        {
            const int n = msg.getNoteNumber();
            if (owner[n] != 0)
                out.addEvent (juce::MidiMessage::noteOff (msg.getChannel(), n), pos);   // pas deux note-on empiles
            out.addEvent (msg, pos);
            owner[n] = 1;

            // Le train d'echos.
            const int tag = ++lastTag;
            const float v01 = msg.getFloatVelocity();
            for (int k = 1; k <= repeats; ++k)
            {
                const int   en = noteFor (n, k, pitch);
                const float ev = velocityFor (v01, k, decay);
                if (en < 0 || en > 127 || ev < 1.0f / 127.0f)
                    continue;   // hors clavier ou eteint : omis
                const auto vel = (juce::uint8) juce::jlimit (1, 127, juce::roundToInt (ev * 127.0f));
                schedule (pos + k * interval, en, vel, true, tag, k);
                schedule (pos + k * interval + echoLen, en, 0, false, tag, k);
            }

            // Photo pour FIG. 1 : ce train devient le train affiche.
            uiTag = tag;
            uiNote.store (n);
            uiVel.store (v01);
            uiFired.store (0);
            samplesSinceLast = -pos;
            lastInterval = interval;
        }
        else if (msg.isNoteOff())
        {
            const int n = msg.getNoteNumber();
            if (owner[n] == 1)
            {
                out.addEvent (msg, pos);
                owner[n] = 0;
            }
            // owner == 2 : un echo a repris cette hauteur, son propre note-off l'eteindra.
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            numEvents = 0;
            for (auto& o : owner) o = 0;
            out.addEvent (msg, pos);
        }
        else
            out.addEvent (msg, pos);
    }

    // 2. Les echos dont l'heure est venue.
    for (int e = 0; e < numEvents;)
    {
        auto& ev = events[e];
        if (ev.when < numSamples)
        {
            const int at = juce::jmax (0, ev.when);
            if (ev.on)
            {
                if (owner[ev.note] != 0)
                    out.addEvent (juce::MidiMessage::noteOff (1, ev.note), at);
                out.addEvent (juce::MidiMessage::noteOn (1, ev.note, ev.vel), at);
                owner[ev.note] = 2;
                if (ev.tag == uiTag)
                    uiFired.store (juce::jmax (uiFired.load(), ev.k));
            }
            else if (owner[ev.note] == 2)
            {
                out.addEvent (juce::MidiMessage::noteOff (1, ev.note), at);
                owner[ev.note] = 0;
            }
            ev = events[--numEvents];
        }
        else
        {
            ev.when -= numSamples;
            ++e;
        }
    }

    // 3. Photo : la position dans le train affiche, en intervalles.
    samplesSinceLast += numSamples;
    uiProgress.store ((float) samplesSinceLast / (float) lastInterval);
    int active = 0;
    for (int e = 0; e < numEvents; ++e) if (events[e].on) ++active;
    uiActive.store (active);
    uiBpm.store ((float) bpm);
}

}
