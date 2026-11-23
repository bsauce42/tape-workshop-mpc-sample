# Tape Workshop Orchestra — local v134 prototype

Based on Melodee by Kerem Kiliç (Ellic Studio). Original Tape Workshop sampler, UI and configuration.

Separate ARM64 Linux VST2 instrument: 17 sampled players, four section mixes and 32 factory presets.
Strings, brass, woodwinds and pitched orchestral percussion share a bounded 12-voice sampler.
Continuous Dynamics crossfades recorded velocity layers; CC1 controls Dynamics and CC11 Expression.
Sustain/short recordings, ADSR/filter, vibrato, synthesized solo legato/glide, movement, reverb and
room echo are editable. Percussion is one-shot. Each section chooses one player, applied on new notes.

The original Players screen and parameter diagrams run in the host. Waveforms and meters read
measured output; instrument animation is illustrative. Factory samples are VSCO 2 Community Edition,
CC0-1.0, with pinned sample and SFZ mapping revisions. No commercial library code/audio/art is included.
The bank has 428 zones, about 62.5 MiB PCM: mono 32 kHz/16-bit, two velocity layers, reduced root
density and bounded 4.5-second recordings. These constraints limit realism compared with desktop libraries.

Normal polyphonic held chords remain the default. Solo Legato is optional, with last-held-note priority
per MIDI channel. It uses synthesized pitch transitions, not separately recorded true legato.
The player range follows the available sample roots; use pad octaves. Notes outside a player's range
are silent. Keep orchestra.bank next to tape_workshop_orchestra.so.

v134 SD ZIP includes all prior 23 plug-ins unchanged, plus Orchestra. The OS-only signed update
requires separately copying the Orchestra plug-in folder. The standalone plug-in ZIP cannot update
the host screen. Installation guide and source recipes are in plugins/tape-workshop-orchestra/README.md.

Validation so far: ARM64 compilation, 32 distinct normalized parameter presets, section selection
bounds, state roundtrips/rejection, source-bank structure and desktop UI preview. No audio rendered
for this new instrument and no MPC installation/audio/CPU test performed, per the user's request to
hold testing. Embedded CPU, safe instance count, loop quality and audible realism remain unverified.
The host can stage separate library copies, so cross-track sharing of bank memory is not guaranteed.

The existing unfinished synth/chord tool is separate and excluded from this build.
