# Tape Workshop Orchestra — 0.2.1 MPC-native prototype

Playable Linux ARM64 VST2 for Tape Workshop OS on Akai MPC Sample. Based on the original Tape Workshop
sampler and Melodee's host ABI by Kerem Kiliç (Ellic Studio). Original code is AGPL-3.0-only. Samples are
VSCO 2 Community Edition by Versilian Studios (CC0-1.0), with exact pinned sources and notices included.
Commercial orchestra libraries informed the workflow goals; their recordings, code and graphics are not used.

17 players across four independently mixed sections:
- Strings: violin section, solo violin, viola, cello and contrabass.
- Brass: French horn, trumpet, trombone and tuba.
- Woodwinds: flute, oboe, clarinet and bassoon.
- Percussion: timpani, marimba, glockenspiel and tubular bells.

32 actual parameter presets: orchestral blends, section/solo instruments, short articulations, held swells,
legato solos, motion textures and echo orchestra. Existing Data-knob preset browsing and B2 menu remain.
Articulation switches recorded sustains/shorts; solo violin's short sample is pizzicato. Percussion is one-shot.
Each family has one selected instrument at a time; blend four families. Instrument changes take effect on new notes.
Use the pad octave controls to reach each instrument's natural sampled range; notes beyond its mapped range are silent.

Hold bowed/wind/brass notes for looping sustain; note-off gives the chosen release. Polyphonic chord mode is the
default. Solo Legato is an optional per-channel last-held-note-priority mono mode. Glide is synthesized sample
pitch movement, not recorded true-legato transitions. ADSR, filter/resonance, extra vibrato, expression, tone,
stereo width, ensemble detune, movement rate/depth/shape/target, reverb and room echo remain editable.
Dynamics continuously crossfades soft/loud recordings and adjusts level; MIDI CC1 moves Dynamics, CC11 Expression.
CC64 sustain, ±2-semitone bend and MIDI panic are supported. Host project/preset state uses a separate plug-in ID.

UI: original four-section Players view, instrument selectors, sound diagrams, measured stereo waveforms/meters,
Dynamics crossfade and Solo Legato page. It requires the matching OS UI build. The standalone ZIP only installs
the instrument; merge Melodee into the SD root and keep orchestra.bank beside tape_workshop_orchestra.so.
Normal K1–K3 and Minus/Plus navigation remain; group tabs are indicators. No existing shortcuts are overwritten.

Bounded prototype: 12 voices, eight sample readers per voice, fixed event queue/effect buffers. Bank size is about
63 MiB PCM (428 zones); all file reading/allocation happens at load, outside the audio callback. Samples are mono
32 kHz/16-bit, at most 4.5 seconds each, with reduced root density and two velocity layers. Sampled room/stereo
complexity, round-robin diversity and realism are intentionally more limited than desktop orchestral libraries.
The current host may load separate module copies for separate tracks; shared-bank memory is not guaranteed
across host staging paths. CPU limits and maximum safe instance count still require MPC measurements.
No on-device installation or audio test has been run for this build. No full-orchestra compatibility/performance
claim is made from compilation or the illustrative desktop UI previews. Prior Strings remains separate.

Rebuild: create a Python virtual environment with numpy<2.1 and scipy<1.14; run prepare.py, then build.py.
The original Tape Workshop sampler source at ../tape-workshop-violin/strings.cpp and
src/melodee/plugin/{vst2_abi.h,orchestra_info.h} are also required. Downloads stay in ignored build/.
Bank provenance records the exact sample/mapping revisions and processing. No proprietary SDK is bundled.

Melody player (requires matching Tape Workshop v135 OS)

The appended melody pages add Melody Play, New Melody, Key, Scale, Note Speed,
Octave, Note Range, Pattern Steps, Note Gate, Swing, Density, Melody Velocity and
Follow Project. Melody Play starts Off in every factory preset. Enable it, keep
B3 TRANSPORT enabled, then press the MPC Play button. Stop releases the generated
notes; manually played notes remain separate. Normal pads, Data-knob presets and
B2 PRESETS remain available. Use the existing parameter-page navigation.

Follow Project starts On: the generator uses the project's Key/Scale. Editing its
local Key or Scale turns Follow Project off; turn it back On to relink. New Melody
selects one of 1,000 repeatable scale-based variations. Speed, octave, range,
8/16/32 steps, gate, swing, density and velocity change the generated performance.
The melody grid shows those settings and transport position. This is internal
instrument playback: it does not write notes into the Roll or export MIDI.

From a melody page, Shift+B3 opens a read-only SD audio-sample picker. Select a
sample, wait for its estimate, then B3 confirms the project key. No sample is
loaded into or replaces the instrument, and no audio auto-preview runs here.
A stable single-note sample supplies only a root; the existing scale stays.
A tonal phrase can supply estimated major/minor. Unclear audio leaves the project
unchanged. Analysis uses the existing background sample-key service (up to the
first five seconds), so percussion, mixed/changing keys or detuned recordings can
be ambiguous. Review the estimate before applying it. B1 cancels/backtracks.

Both instrument thumbnails are included in the OS plug-in browser. Pure scheduler,
metadata and state checks plus desktop UI previews were run without rendering audio.
The new melody feature has not yet been auditioned or performance-tested on MPC.

Output-level update: shared post-mix gain raised by 9.54 dB before the existing soft ceiling. Output knob, expression/dynamics, MIDI controls, plug-in IDs and state format are retained. Existing saved sounds will play louder; use Output to balance a mix. No new per-sample processing stage was added.
