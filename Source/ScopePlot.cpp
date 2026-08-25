#include "ScopePlot.h"
#include "ManualStyle.h"
#include "EchoEngine.h"

namespace echomxa
{

namespace
{
    const char* kNoteNames[12] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    const char* kRateNames[3]  = { "1/4", "1/8", "1/16" };

    juce::String noteName (int n) { return juce::String (kNoteNames[((n % 12) + 12) % 12]) + juce::String (n / 12 - 1); }
}

ScopePlot::ScopePlot (EchoProcessor& proc)
    : processor (proc)
{
    auto& apvts = processor.getAPVTS();
    rate    = apvts.getRawParameterValue ("rate");
    repeats = apvts.getRawParameterValue ("repeats");
    decay   = apvts.getRawParameterValue ("decay");
    pitch   = apvts.getRawParameterValue ("pitch");
    length  = apvts.getRawParameterValue ("length");

    setInterceptsMouseClicks (false, false);
}

void ScopePlot::paint (juce::Graphics& g)
{
    auto full = getLocalBounds().toFloat();
    auto caption = full.removeFromBottom (16.0f);
    auto box = full;

    const auto& engine = processor.getEngine();
    const int   rateV  = juce::jlimit (0, 2, juce::roundToInt (rate->load()));
    const int   nRep   = juce::jlimit (1, EchoEngine::kMaxRepeats, juce::roundToInt (repeats->load()));
    const float decayV = decay->load();
    const int   pitchV = juce::roundToInt (pitch->load());
    const float lenV   = length->load();

    const bool  hasNote = engine.getUiNote() >= 0;
    const int   note   = hasNote ? engine.getUiNote() : 60;     // sans note : train hypothetique sur C4
    const float vel0   = hasNote ? engine.getUiVelocity() : 0.8f;
    const int   fired  = hasNote ? engine.getUiFired() : 0;
    const float prog   = hasNote ? engine.getUiProgress() : -1.0f;

    auto grid = box.withTrimmedLeft (52.0f).withTrimmedRight (18.0f)
                   .withTrimmedTop (30.0f).withTrimmedBottom (24.0f);

    // Colonnes : 0 = la note, 1..N = les echos.
    const int cols = nRep + 1;
    const float colW = grid.getWidth() / (float) cols;
    auto xFor = [&] (float k) { return grid.getX() + (k + 0.5f) * colW; };

    // Lignes : la hauteur. On cadre entre la note la plus basse et la plus haute du train.
    int lo = note, hi = note;
    for (int k = 1; k <= nRep; ++k)
    {
        const int n = EchoEngine::noteFor (note, k, pitchV);
        if (n >= 0 && n <= 127) { lo = juce::jmin (lo, n); hi = juce::jmax (hi, n); }
    }
    // Une marge de deux demi-tons au moins : le train ne colle jamais aux bords,
    // et une hauteur unique (PITCH 0) se dessine au milieu.
    const int pad = juce::jmax (2, (hi - lo) / 6);
    lo -= pad; hi += pad;
    const int span = juce::jmax (1, hi - lo);
    auto yFor = [&] (int n) { return grid.getBottom() - (float) (n - lo) / (float) span * grid.getHeight(); };

    // --- La grille du temps : une ligne par intervalle, etiquette en intervalles --------
    for (int k = 0; k < cols; ++k)
    {
        const float x = grid.getX() + (float) k * colW;
        g.setColour (k == 0 ? palette::inkMid.withAlpha (0.6f) : palette::inkFaint);
        g.drawVerticalLine ((int) x, grid.getY() - 4.0f, grid.getBottom() + 4.0f);
        g.setFont (fonts::mono (8.0f));
        g.setColour (palette::inkMid);
        g.drawText (k == 0 ? juce::String ("NOTE") : juce::String ("+") + juce::String (k) + " x " + kRateNames[rateV],
                    juce::Rectangle<float> (colW, 10.0f).withPosition (x, grid.getBottom() + 8.0f),
                    juce::Justification::centred);
    }
    g.setColour (palette::inkMid.withAlpha (0.6f));
    g.drawHorizontalLine ((int) grid.getBottom(), grid.getX(), grid.getRight());

    // Reperes de hauteur en marge : la note, et la derniere hauteur du train.
    g.setFont (fonts::mono (9.0f));
    g.setColour (palette::inkMid);
    g.drawText (noteName (note), juce::Rectangle<float> (40.0f, 11.0f).withPosition (box.getX() + 8.0f, yFor (note) - 5.5f),
                juce::Justification::centredRight);
    if (pitchV != 0)
    {
        const int last = juce::jlimit (0, 127, EchoEngine::noteFor (note, nRep, pitchV));
        g.drawText (noteName (last), juce::Rectangle<float> (40.0f, 11.0f).withPosition (box.getX() + 8.0f, yFor (last) - 5.5f),
                    juce::Justification::centredRight);
        g.setColour (palette::inkFaint);
        g.drawHorizontalLine ((int) yFor (last), grid.getX(), grid.getRight());
    }
    g.setColour (palette::inkFaint);
    g.drawHorizontalLine ((int) yFor (note), grid.getX(), grid.getRight());

    // --- Le train : la note puis ses echos ---------------------------------------------
    // Le trait de liaison, en pointille, qui montre la pente de PITCH.
    {
        juce::Path p;
        p.startNewSubPath (xFor (0.0f), yFor (note));
        for (int k = 1; k <= nRep; ++k)
        {
            const int n = EchoEngine::noteFor (note, k, pitchV);
            if (n < 0 || n > 127) break;
            p.lineTo (xFor ((float) k), yFor (n));
        }
        juce::Path dashed;
        const float dashes[] = { 3.0f, 3.0f };
        juce::PathStrokeType (0.8f).createDashedStroke (dashed, p, dashes, 2);
        g.setColour (palette::inkMid.withAlpha (0.6f));
        g.fillPath (dashed);
    }

    for (int k = 0; k <= nRep; ++k)
    {
        const int   n = k == 0 ? note : EchoEngine::noteFor (note, k, pitchV);
        const float v = k == 0 ? vel0 : EchoEngine::velocityFor (vel0, k, decayV);
        const bool  exists = n >= 0 && n <= 127 && v >= 1.0f / 127.0f;
        const float x = xFor ((float) k);
        const float y = yFor (juce::jlimit (0, 127, n));
        const float r = 2.0f + 6.0f * juce::jlimit (0.0f, 1.0f, v);   // la taille EST la velocite

        // La barre de LENGTH : du point jusqu'a l'extinction.
        if (exists)
        {
            const float len = (k == 0 ? 0.0f : lenV) * colW;
            if (len > 0.0f)
            {
                g.setColour ((k <= fired && hasNote ? palette::spot : palette::ink).withAlpha (0.45f));
                g.drawLine (x, y, x + len, y, 1.4f);
            }
        }

        if (! exists)
        {
            // Echo omis (hors clavier / eteint) : une croix fine.
            g.setColour (palette::inkFaint);
            g.drawLine (x - 3.0f, y - 3.0f, x + 3.0f, y + 3.0f, 0.8f);
            g.drawLine (x - 3.0f, y + 3.0f, x + 3.0f, y - 3.0f, 0.8f);
            continue;
        }

        const bool played = hasNote && k <= fired;
        if (played)
        {
            g.setColour (k == 0 ? palette::ink : palette::spot);
            g.fillEllipse (x - r, y - r, 2.0f * r, 2.0f * r);
        }
        else
        {
            g.setColour (palette::film);
            g.fillEllipse (x - r, y - r, 2.0f * r, 2.0f * r);
            g.setColour (hasNote ? palette::spot : palette::ink);
            g.drawEllipse (x - r, y - r, 2.0f * r, 2.0f * r, 1.2f);
        }

        // La velocite, en chiffres, au-dessus de chaque point.
        g.setFont (fonts::mono (8.0f));
        g.setColour (palette::inkMid);
        g.drawText (juce::String (juce::roundToInt (v * 127.0f)),
                    juce::Rectangle<float> (30.0f, 10.0f).withCentre ({ x, y - r - 8.0f }),
                    juce::Justification::centred);
    }

    // La tete de lecture, tant que le train court.
    if (hasNote && prog >= 0.0f && prog <= (float) cols - 0.5f)
    {
        const float px = xFor (prog);
        g.setColour (palette::spot.withAlpha (0.55f));
        g.drawVerticalLine ((int) px, grid.getY() - 4.0f, grid.getBottom());
    }

    // --- Tally ------------------------------------------------------------------------
    {
        g.setColour (palette::film);
        g.fillRect (juce::Rectangle<float> (box.getRight() - 262.0f, box.getY() + 4.0f, 256.0f, 16.0f));
        auto tally = juce::Rectangle<float> (252.0f, 12.0f).withPosition (box.getRight() - 258.0f, box.getY() + 6.0f);
        g.setFont (fonts::mono (10.0f));
        g.setColour (palette::inkMid);
        g.drawText ("ECHO", tally.removeFromLeft (40.0f), juce::Justification::centredLeft);
        g.setColour (palette::ink);
        g.drawText (hasNote ? juce::String (juce::jmin (fired, nRep)) + "/" + juce::String (nRep) : juce::String ("-/") + juce::String (nRep),
                    tally.removeFromLeft (42.0f), juce::Justification::centredLeft);
        g.setColour (palette::inkMid);
        g.drawText ("RATE", tally.removeFromLeft (40.0f), juce::Justification::centredLeft);
        g.setColour (palette::ink);
        g.drawText (juce::String (kRateNames[rateV]) + " @ " + juce::String (juce::roundToInt (engine.getUiBpm())),
                    tally, juce::Justification::centredLeft);
    }

    if (! hasNote)
    {
        g.setFont (fonts::lettering (10.0f));
        g.setColour (palette::inkMid);
        g.drawText ("PLAY A NOTE - THE TRAIN SHOWN IS WHAT IT WOULD TRIGGER",
                    juce::Rectangle<float> (340.0f, 12.0f).withPosition (box.getX() + 8.0f, box.getY() + 8.0f),
                    juce::Justification::centredLeft);
    }

    // --- Cadre + legende de figure ---------------------------------------------
    g.setColour (palette::ink);
    g.drawRect (box, 1.0f);

    const juce::String cap = "FIG. 1 - ECHO TRAIN OF THE LAST NOTE, TIME x PITCH, DOT SIZE IS VELOCITY";
    g.setFont (fonts::lettering (10.0f));
    g.setColour (palette::inkMid);
    g.drawText (cap, caption.withTrimmedTop (4.0f), juce::Justification::bottomLeft);

    const float capW = juce::GlyphArrangement::getStringWidth (fonts::lettering (10.0f), cap);
    g.setColour (palette::inkFaint);
    g.drawHorizontalLine ((int) (caption.getBottom() - 3.0f),
                          caption.getX() + capW + 10.0f, caption.getRight());
}

}
