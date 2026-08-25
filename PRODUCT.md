# Product

<!-- impeccable:product-schema 1 -->

## Platform

desktop

Native JUCE MIDI-effect plugin (AU `aumi` MIDI Processor / VST3 / Standalone, macOS 11+, Windows via CI). Sibling of the MXA suite (fourth MIDI effect after ArpMXA, ChordMXA, StrumMXA); the family design authority is `../PhaserMXA/DESIGN.md` and the family product context is `../PhaserMXA/PRODUCT.md`.

## Product Purpose

A tempo-synced MIDI note echo: each note-on schedules REPEATS echoes at k × RATE (host tempo), note + k × PITCH semitones, velocity × (1 − DECAY)^k, each lasting LENGTH × interval. The original note passes through untouched. Echoes that fall outside 0–127 or below velocity 1 are omitted. Success: a stranger plays one note, hears a rhythmic, pitched, fading cascade re-played by the instrument, and reads the exact train (times, pitches, velocities) on FIG. 1.

## Capabilities and Constraints

- Exactly five parameters: `rate` (choice 1/4, 1/8, 1/16 — family switch "switchRate", default 1/8), `repeats` (1–8, default 4), `decay` (0–90 % per echo, default 30), `pitch` (−12..+12 st per echo, default 0), `length` (10–100 % of interval, default 50).
- Engine (`EchoEngine`): 1024-slot scheduled event array crossing blocks; per-pitch `owner[128]` (0 none / 1 original / 2 echo) so an echo landing on a held pitch retriggers cleanly and the original's note-off does not cut an echo that took over the pitch; non-note MIDI passes through; All Notes Off flushes the schedule.
- `velocityFor` / `noteFor` are the single source of truth, shared with FIG. 1 and FIG. 2's DECAY glyph.
- Physics-checked by the snapshot tool at 120 BPM: RATE 1/8, 4 repeats, PITCH +12, DECAY 30 %, note 60 vel 100 → note-ons exactly at 0 / 12000 / 24000 / 36000 / 48000 samples, notes 60/72/84/96/108, velocities 100/70/49/34/24, five note-offs.
- UI truth taps: atomics `uiNote`, `uiVel`, `uiFired`, `uiProgress` (position in the train, in intervals), `uiActive` (echoes in flight), `uiBpm`.
- Editor: Service Manual family sheet, 820×470, spot ink mint green #7FE3B0, DWG NO. MXA-EC-01; one family switch. FIG. 1 = echo train: columns NOTE, +1×rate … +N×rate; rows = pitch (note names in margin), dot radius = velocity (numbers printed), dashed link showing the PITCH slope, LENGTH bars, played dots filled in spot / pending hollow, omitted echoes as crosses, playhead, tally ECHO k/N and RATE @ BPM; idle text shows the train the next note would trigger. FIG. 2 = MIDI IN → DIRECT rail (untouched) + echo rail DELAY (n × rate, ms, HOST TEMPO) → PITCH (staircase) → DECAY (velocity bars from the real law) → summing node → MIDI OUT with "n ECHOES IN FLIGHT".

## Roadmap (later phases)

- V2 candidates: dotted/triplet rates, feedback-style infinite mode with a kill switch, echo velocity taken from the played note's release, alternate-octave (ping-pong pitch) mode, swing.
