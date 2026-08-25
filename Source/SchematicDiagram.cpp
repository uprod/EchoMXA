#include "SchematicDiagram.h"
#include "ManualStyle.h"
#include "EchoEngine.h"

namespace echomxa
{

namespace
{
    const char* kRateNames[3] = { "1/4", "1/8", "1/16" };

    void drawArrowHead (juce::Graphics& g, juce::Point<float> tip, juce::Point<float> dir, float size)
    {
        dir = dir / (dir.getDistanceFromOrigin() + 1.0e-6f);
        const juce::Point<float> n (-dir.y, dir.x);
        juce::Path p;
        p.addTriangle (tip, tip - dir * size + n * (size * 0.55f),
                             tip - dir * size - n * (size * 0.55f));
        g.fillPath (p);
    }

    void drawBlock (juce::Graphics& g, juce::Rectangle<float> block, const juce::String& name)
    {
        g.setColour (palette::film);
        g.fillRect (block);
        g.setColour (palette::ink);
        g.drawRect (block, 1.2f);
        g.setFont (fonts::lettering (10.0f));
        g.drawText (name, block.withHeight (13.0f), juce::Justification::centred);
    }

    void drawValue (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> under, bool live = true)
    {
        const auto font = fonts::mono (9.0f);
        const float tw  = juce::GlyphArrangement::getStringWidth (font, text);
        auto area = juce::Rectangle<float> (tw + 6.0f, 11.0f)
                        .withCentre ({ under.getCentreX(), under.getBottom() + 9.0f });
        g.setColour (palette::film);
        g.fillRect (area);
        g.setFont (font);
        g.setColour (live ? palette::ink : palette::inkMid);
        g.drawText (text, area, juce::Justification::centred);
    }

    void drawSummingNode (juce::Graphics& g, juce::Point<float> c, float r)
    {
        g.setColour (palette::film);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (palette::ink);
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.1f);
        g.drawLine (c.x - r * 0.5f, c.y, c.x + r * 0.5f, c.y, 1.0f);
        g.drawLine (c.x, c.y - r * 0.5f, c.x, c.y + r * 0.5f, 1.0f);
    }
}

SchematicDiagram::SchematicDiagram (EchoProcessor& proc)
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

void SchematicDiagram::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();
    auto caption  = getLocalBounds().toFloat().removeFromBottom (16.0f);

    const int   rateV  = juce::jlimit (0, 2, juce::roundToInt (rate->load()));
    const int   nRep   = juce::roundToInt (repeats->load());
    const float decayV = decay->load();
    const int   pitchV = juce::roundToInt (pitch->load());
    const float lenV   = length->load();
    const auto& engine = processor.getEngine();
    const int   active = engine.getUiActive();
    const float bpm    = engine.getUiBpm();
    const float rateQ  = rateV == 0 ? 1.0f : rateV == 2 ? 0.25f : 0.5f;
    const float intervalMs = 60000.0f / juce::jmax (20.0f, bpm) * rateQ;

    const float dryY  = 22.0f;    // le direct, en haut
    const float echoY = 56.0f;    // le rail d'echo
    const float inX   = 12.0f;
    const float tapX  = 40.0f;
    const float blockW = 66.0f, blockH = 26.0f;
    const float dlyX  = w * 0.20f;
    const float pitX  = w * 0.42f;
    const float decX  = w * 0.62f;
    const float mixX  = w * 0.85f;
    const float outX  = w - 16.0f;

    const juce::Rectangle<float> dly (dlyX, echoY - blockH * 0.5f, blockW, blockH);
    const juce::Rectangle<float> pit (pitX, echoY - blockH * 0.5f, blockW, blockH);
    const juce::Rectangle<float> dec (decX, echoY - blockH * 0.5f, blockW, blockH);

    // --- MIDI IN et derivation -------------------------------------------------------
    const float ioY = (dryY + echoY) * 0.5f;
    g.setColour (palette::ink);
    g.drawEllipse (inX - 3.0f, ioY - 3.0f, 6.0f, 6.0f, 1.1f);
    g.drawLine (inX + 3.0f, ioY, tapX, ioY, 1.2f);
    g.fillEllipse (tapX - 2.2f, ioY - 2.2f, 4.4f, 4.4f);
    g.drawLine (tapX, ioY, tapX, dryY, 1.2f);
    g.drawLine (tapX, ioY, tapX, echoY, 1.2f);
    g.setFont (fonts::lettering (9.0f));
    g.setColour (palette::inkMid);
    g.drawText ("MIDI IN", juce::Rectangle<float> (50.0f, 10.0f).withPosition (inX - 6.0f, ioY - 19.0f),
                juce::Justification::centredLeft);

    // --- Le direct : la note traverse intacte ---------------------------------------------
    g.setColour (palette::ink);
    g.drawLine (tapX, dryY, mixX, dryY, 1.2f);
    g.drawLine (mixX, dryY, mixX, ioY - 8.0f, 1.2f);
    g.setColour (palette::inkMid);
    g.drawText ("DIRECT - THE PLAYED NOTE, UNTOUCHED",
                juce::Rectangle<float> (240.0f, 10.0f).withPosition (tapX + 10.0f, dryY - 14.0f),
                juce::Justification::centredLeft);

    // --- Le rail d'echo : DELAY -> PITCH -> DECAY ------------------------------------------
    g.setColour (palette::ink);
    g.drawLine (tapX, echoY, dly.getX(), echoY, 1.2f);
    drawArrowHead (g, { dly.getX(), echoY }, { 1.0f, 0.0f }, 5.0f);
    drawBlock (g, dly, "DELAY");
    {
        // Glyphe : n impulsions espacees.
        auto r = dly.reduced (8.0f, 3.0f).withTrimmedTop (12.0f);
        for (int k = 0; k <= juce::jmin (nRep, 8); ++k)
        {
            const float x = r.getX() + (float) k / 8.0f * r.getWidth();
            g.setColour (k == 0 ? palette::ink : palette::inkMid);
            g.drawLine (x, r.getBottom(), x, r.getY() + (k == 0 ? 0.0f : 3.0f), 1.0f);
        }
    }
    drawValue (g, juce::String (nRep) + " x " + kRateNames[rateV] + "  " + juce::String (juce::roundToInt (intervalMs)) + " ms", dly);

    // L'horloge de l'hote, qui descend sur DELAY.
    {
        const float cx = dly.getCentreX();
        g.setColour (palette::inkMid);
        g.drawLine (cx, dly.getY() - 10.0f, cx, dly.getY(), 0.9f);
        drawArrowHead (g, { cx, dly.getY() }, { 0.0f, 1.0f }, 4.0f);
        g.setFont (fonts::lettering (8.5f));
        g.drawText ("HOST TEMPO " + juce::String (juce::roundToInt (bpm)),
                    juce::Rectangle<float> (110.0f, 10.0f).withPosition (cx + 6.0f, dly.getY() - 14.0f),
                    juce::Justification::centredLeft);
    }

    g.setColour (palette::ink);
    g.drawLine (dly.getRight(), echoY, pit.getX(), echoY, 1.2f);
    drawArrowHead (g, { pit.getX(), echoY }, { 1.0f, 0.0f }, 5.0f);
    drawBlock (g, pit, "PITCH");
    {
        // Glyphe : l'escalier des hauteurs, pente = PITCH.
        auto r = pit.reduced (10.0f, 3.0f).withTrimmedTop (12.0f);
        const float slope = juce::jmap ((float) pitchV, -12.0f, 12.0f, -r.getHeight(), r.getHeight());
        g.setColour (pitchV != 0 ? palette::ink : palette::inkMid);
        for (int k = 0; k < 4; ++k)
        {
            const float x = r.getX() + (float) k / 3.0f * r.getWidth();
            const float y = r.getCentreY() - (float) k / 3.0f * slope * 0.5f;
            g.fillEllipse (x - 1.5f, y - 1.5f, 3.0f, 3.0f);
        }
    }
    drawValue (g, pitchV == 0 ? "0 st" : (pitchV > 0 ? "+" : "") + juce::String (pitchV) + " st / ECHO", pit, pitchV != 0);

    g.setColour (palette::ink);
    g.drawLine (pit.getRight(), echoY, dec.getX(), echoY, 1.2f);
    drawArrowHead (g, { dec.getX(), echoY }, { 1.0f, 0.0f }, 5.0f);
    drawBlock (g, dec, "DECAY");
    {
        // Glyphe : les barres de velocite decroissantes (la vraie loi).
        auto r = dec.reduced (10.0f, 3.0f).withTrimmedTop (12.0f);
        for (int k = 0; k < 5; ++k)
        {
            const float h = r.getHeight() * EchoEngine::velocityFor (1.0f, k, decayV);
            const float x = r.getX() + (float) k / 5.0f * r.getWidth();
            g.setColour (k == 0 ? palette::ink : palette::inkMid);
            g.fillRect (juce::Rectangle<float> (x, r.getBottom() - h, 5.0f, h));
        }
    }
    drawValue (g, "-" + juce::String (juce::roundToInt (decayV * 100.0f)) + " % / ECHO  LEN " + juce::String (juce::roundToInt (lenV * 100.0f)) + "%", dec);

    // --- Le noeud de sortie -----------------------------------------------------------------
    g.setColour (palette::ink);
    g.drawLine (dec.getRight(), echoY, mixX, echoY, 1.2f);
    g.drawLine (mixX, echoY, mixX, ioY + 8.0f, 1.2f);
    drawSummingNode (g, { mixX, ioY }, 8.0f);
    g.drawLine (mixX + 8.0f, ioY, outX - 3.0f, ioY, 1.4f);
    g.fillEllipse (outX - 3.0f, ioY - 3.0f, 6.0f, 6.0f);
    g.setFont (fonts::lettering (9.0f));
    g.setColour (palette::inkMid);
    g.drawText ("MIDI OUT", juce::Rectangle<float> (60.0f, 10.0f).withPosition (outX - 56.0f, ioY - 19.0f),
                juce::Justification::centredRight);
    g.setFont (fonts::mono (9.0f));
    g.setColour (palette::ink);
    g.drawText (juce::String (active) + (active == 1 ? " ECHO" : " ECHOES") + " IN FLIGHT",
                juce::Rectangle<float> (110.0f, 10.0f).withPosition (outX - 106.0f, ioY + 10.0f),
                juce::Justification::centredRight);

    // --- Legende de figure ----------------------------------------------------
    const juce::String cap = "FIG. 2 - MIDI PATH, DIRECT NOTE AND ECHO RAIL, HOST-TEMPO DELAY, PITCH STEP, DECAY";
    g.setFont (fonts::lettering (10.0f));
    g.setColour (palette::inkMid);
    g.drawText (cap, caption.withTrimmedTop (4.0f), juce::Justification::bottomLeft);

    const float capW = juce::GlyphArrangement::getStringWidth (fonts::lettering (10.0f), cap);
    g.setColour (palette::inkFaint);
    g.drawHorizontalLine ((int) (caption.getBottom() - 3.0f),
                          caption.getX() + capW + 10.0f, caption.getRight());
}

}
