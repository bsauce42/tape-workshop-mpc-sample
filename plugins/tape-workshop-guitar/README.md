# Tape Workshop Guitar — 0.1.0

Original physically modeled acoustic guitar for MPC Sample running Tape Workshop OS / compatible Melodee. Linux ARM64 VST2 ABI; not a macOS/Windows desktop VST. Host interface based on Melodee by Kerem Kiliç (Ellic Studio). Code licensed AGPL-3.0-only.

This is a plucked-string waveguide prototype, not a sampled guitar or a claim of recorded acoustic realism. Steel and nylon timbres share an original string model with different damping. Pick position comb excitation, brightness, decay, palm mute, resonant body, velocity, stereo width, detune, vibrato, room and echo shape the sound. Harmonics is a timbral preset, not separately recorded harmonic articulations. No proprietary recordings or SDK are included.

Install: merge the ZIP's Melodee folder into the SD-card root. Restart/rescan the plug-in browser, create a Plugin track and choose Tape Workshop Guitar. Existing OS and plug-ins stay in place. Use pads/MIDI to play notes and chords; ordinary sequencer/Roll recording applies. The standard host parameter pages expose three controls per page, and Data-knob presets follow your existing OS behavior. No OS update or custom graphics are required.

Pages:
1. String material / Pick position / Brightness
2. Decay / Palm mute / Body resonance
3. Attack / Note-off release / Velocity sensitivity
4. Strum time per note / Up-down direction / Detune
5. Stereo width / Vibrato depth / Vibrato rate
6. Room / Echo / Output

Eight presets: Steel Fingerstyle, Nylon Warm, Steel Picked, Palm Muted, Wide Strum, Dark Nylon, Bright Harmonics, Ambient Guitar. Strum spreads simultaneous notes by pitch in the selected direction (up to 60ms per note). Notes arriving in different audio blocks are separate gestures. A guitar pluck naturally fades even if held; Release damps the string after note-off. Sustain pedal holds note releases. Pitch bend is ±2 semitones; MIDI expression CC11 and CC120/123 panic supported.

12 voices, voice stealing, fixed buffers, sample-accurate bounded MIDI queue, state chunks, shared controls via atomics. No allocation, locks or files in audio processing. Modeled range is limited by the delay-line bounds; use the normal guitar range E2–E6. No fret selection, true recorded slides, hammer-ons, or automatic guitar chord fingering yet. Use moderate Room/Echo for natural sound. Host tests do not establish actual MPC CPU performance or acoustic realism; hardware audition still required.

Build: python3 plugins/tape-workshop-guitar/build.py. Standalone ZIP contains only this plug-in; source recipe is supplied separately. Do not replace the complete OS build with this ZIP.
