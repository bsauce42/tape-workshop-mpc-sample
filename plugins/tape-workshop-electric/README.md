# Tape Workshop Electric — 0.1.0

Playable, physically modeled electric guitar with an editable three-slot pedal rack, four original amp voicings and three cabinet filters plus Direct. Linux ARM64 VST2 ABI for MPC Sample running Tape Workshop / compatible Melodee. Host ABI based on Melodee by Kerem Kiliç (Ellic Studio). Original DSP, AGPL-3.0-only. No proprietary samples, amp captures, impulse responses or withdrawn SDK.

This is a synthesis prototype: an acoustic-style plucked waveguide with electric pickup tonal shaping, followed by effects. It is not sampled electric guitar, a branded amp emulation, a circuit-level model, or a claim of studio-grade recorded realism. Cabinet choices are original low-pass voicings, not measured speaker IRs. Overdrive uses two nonlinear substeps and filtering; it is not a full oversampled amp solver. Audition on MPC before relying on its realism or CPU load.

Install: merge the ZIP's Melodee folder onto the SD-card root. Restart/rescan plug-ins. Create a Plugin track, choose Tape Workshop Electric. It is separate from Tape Workshop Guitar and needs no OS update. Play pads/MIDI and record with the existing Roll/sequencer. Data-knob preset navigation remains the host's normal behavior.

Rack order: Guitar → Slot 1 → Slot 2 → Amp → Cabinet → Slot 3 → Output.
Each slot independently selects Off, Compressor, Overdrive, Fuzz, Wah, Chorus, Phaser, Tremolo, Delay or Reverb. Use slots 1–2 for compression/drive/wah and slot 3 for ambience. Slot positions are fixed; this version does not load third-party pedals or provide drag-and-drop reordering. Any effect can be assigned to any slot. Amount controls strength; Tone affects drive brightness, wah center, delay damping or reverb damping. Tone is unused for compressor, chorus, phaser and tremolo. Shared FxRate sets modulation speed; DelayMs sets 40–300ms. No tempo sync yet.

Native MPC parameter pages (K1–K3):
1. Pickup / Pick position / Brightness
2. Ring / Palm mute / Resonance
3. Attack / Release / Velocity
4. Strum / Direction / Detune
5. Width / Vibrato / Vibrato rate
6. Extra room / Extra echo / Output
7. Amp / Gain / Amp level
8. Bass / Mid / Treble
9. Cabinet / Mic tone / Gate
10. Slot 1 / Amount 1 / Tone 1
11. Slot 2 / Amount 2 / Tone 2
12. Slot 3 / Amount 3 / Tone 3
13. Shared modulation rate / Delay time

Amps: Clean, Jazz, Brit, HighGain. Cabinets: Direct, Open112, Closed212, Stack412. Pickup continuously adjusts brightness (display labels Humbuck/Single identify its ends); it is not a measured pickup-position model. Mic changes cabinet brightness, not a microphone IR. Eight complete presets: Clean Single, Warm Jazz, Brit Crunch, High Gain, Funk Wah, Dream Chorus, Lead Echo, Ambient Swell. All presets use implemented controls. Note-off release, pedal CC64, expression CC11, ±2-semitone bend and panic CC120/123 retained.

12 voices; fixed string/delay buffers; no heap allocation, file access or locks in rendering. Three rack slots use approximately 1.5MiB of delay memory plus the string engine. State chunks save the amp, cabinet and full rack. Modeled guitar range E2–E6 recommended. Note articulations such as fret slides, hammer-ons and true palm-muted recordings are not sampled here. Natural held plucks decay; this is not an infinite sustaining bowed instrument.

Build: python3 plugins/tape-workshop-electric/build.py (also requires the sibling tape-workshop-guitar/guitar.cpp). Tests: clang++ -std=c++17 -O2 -Isrc tests/tw_electric/test.cpp -o build/tw-electric-test && build/tw-electric-test.
