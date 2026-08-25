# EchoMXA

A tempo-synced MIDI note echo — a MIDI effect: every note you play is repeated REPEATS times at a RATE-synced interval (1/4, 1/8, 1/16 of the host tempo), each repeat transposed by PITCH semitones more than the last and played softer (DECAY), lasting LENGTH % of the interval. It is a delay, but in MIDI: the instrument really re-plays the notes with its own attack. The played note passes through untouched. FIG. 1 is the live echo train of the last note.

MIDI effect plugin (AU MIDI FX / VST3 / Standalone) built with [JUCE](https://juce.com). Part of the [MXA plugin suite](https://mxaudio.mescalina.fr/). macOS 11+ and Windows — Windows builds (VST3 + Standalone) are available in [Releases](https://github.com/uprod/EchoMXA/releases).

In Logic Pro, insert it in the **MIDI FX** slot of an instrument track, before the instrument.

## Build

```sh
git clone --recurse-submodules https://github.com/uprod/EchoMXA.git
cd EchoMXA
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

If you already have the MXA suite checked out with a shared `../JUCE` folder, the submodule is optional — the build falls back to the sibling folder automatically.
