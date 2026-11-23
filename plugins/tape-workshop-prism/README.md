# Tape Workshop Prism FM — 0.1.2 testing build

Original six-operator synth for Tape Workshop OS on the MPC Sample. Linux ARM64 VST2 ABI;
not a Mac/Windows VST, not Sytrus, and not compatible with Sytrus presets or FL Studio projects.
Original DSP, AGPL-3.0-only. Host ABI/integration based on Melodee by Kerem Kiliç (Ellic Studio).
No Image-Line code, presets, samples, interface artwork or Steinberg SDK is included.

## Playing and navigation

Copy the complete `Melodee/Plugins/Tape Workshop - VST - Prism FM` folder to the SD card.
Choose **Prism FM** in the plug-in browser. Play pads or MIDI; record and edit notes in the
existing Roll. Notes sustain while held, with release tails after note-off. Sustain pedal,
velocity, pitch bend and all-notes-off are supported. The Data knob changes factory presets
in the plug-in UI; the normal B2 preset menu and OS preset/project saving remain available.
Use the existing parameter-page navigation and K1–K3 to edit three parameters at a time.

16 hip-hop/trap presets: Trap Crystal, Soul Keys, Dark Pluck, 808 FM Bass, Cloud Pad,
Late Night Pad, Trap Metal Bell, West Coast Lead, LoFi Keys, Dark Ring Pluck,
Gospel Organ, Trap Atmosphere, Soft Trap Mallet, Gritty Mono Lead, Airy Choir Pad, Init Sine.

Version 0.1.2 defaults to four voices and 1x unison (mono lead/bass patches may use fewer).
It computes only operators connected to an audible carrier, only filters in use, and sparse
FM/RM connections. Control smoothing runs at a bounded 16-sample cadence. Saturation uses
an inexpensive bounded rational curve. These CPU changes preserve parameter IDs and state
chunks, but nonlinear timbre can differ slightly from earlier versions. Existing saved
patches retain their own voice/unison settings; select a new factory preset for light defaults.


## Native visual UI (Tape Workshop v192 or newer)

The MPC browser shows the matching Prism FM image. Plus/minus move one page;
**Shift + Plus/Minus** jump between sections, individual operators and filters.
Data continues to change presets. K1–K3 change the three displayed controls.

- Pages 1–8: main sound controls, X/Y macros, voices, unison and effects.
- Pages 9–32: four pages per operator: Tuning/Level, Wave/Routing, Amp Envelope,
  Release/Motion. Wave preview and ADSR diagrams follow the actual settings.
- Pages 33–38: two pages per filter, with response diagrams.
- Pages 39–50: FM matrix. Source > destination, selected connections outlined.
- Pages 51–62: RM matrix, same routing convention.
- Page 63: 128-partial editor: K1 operator, K2 harmonic, K3 amplitude. Choose
  Additive on that operator's Wave page to hear it. The bars show saved harmonic data.
- Page 64: motion curve: K1 operator, K2 point, K3 height. Release/Motion or main
  page 8 controls motion speed/depth. Curves show saved eight-point data.

Older OS builds retain generic 60-bank access. Parameter IDs and saved synth
states are unchanged. Version 0.1.1 adds a bounded UI-only metadata query;
no GUI drawing or file operations run in the plug-in's audio callback.

X changes overall FM intensity; Y shifts filter cutoff. Unison is one or two subvoices with
per-note stereo spread/detuning. Chorus, feedback delay and a four-line feedback room reverb
run after the voices. Reverb is algorithmic, not convolution. No commercial preset import.

## Resource and prototype limits

Eight logical notes maximum, with oldest-voice stealing and a short new-note fade-in.
Two-way unison doubles oscillator/filter work. Start with fewer voices on busy projects.
All audio buffers, MIDI queues and voice storage are bounded; no allocation, locks, file I/O,
FFT or table building in audio processing. Wavetables use a worker with lock-free triple-buffer
handoff. Large FM amounts, ring modulation and waveshaping can alias; this initial version
has no oversampling. Additive tables and saw/square generation reduce basic oscillator aliasing.
This is not the complete Sytrus articulation/unison-envelope system. Mouse/touch-drawn envelope editing and hardware CPU qualification remain future work.
Desktop timings are not MPC performance measurements. Do not claim a safe embedded voice count
until checked on the unit. State restores reject malformed, wrong-version and nonfinite data.

Build: `python3 plugins/tape-workshop-prism/build.py` from the repository root.
Set `ZIG_BIN` or `PRISM_DEST` to override compiler or output bundle.
Tests: `clang++ -std=c++17 -O2 -pthread -Isrc tests/tw_prism/test.cpp -o build/prism-test`.

Feature reference: [Image-Line’s Sytrus manual](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/plugins/Sytrus.htm). No feature or preset compatibility is implied.
