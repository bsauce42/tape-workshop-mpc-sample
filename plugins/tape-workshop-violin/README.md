# Tape Workshop Strings — 0.4 plug-in build

Sample-based Linux ARM64 VST2 instrument for Tape Workshop OS/Melodee on MPC Sample.
Host ABI based on Melodee by Kerem Kiliç (Ellic Studio). Original code: AGPL-3.0-only.
Acoustic recordings: VSCO 2 Community Edition, CC0-1.0; exact provenance and license included.

Violin, viola section, cello section and solo contrabass, each with bowed vibrato and pizzicato.
Velocity layers preserve their relative recorded levels. Bowed samples have crossfaded sustain
loops: hold pads/MIDI keys to keep chords sounding; release keys for the adjustable release.
Sustain pedal CC64, modulation CC1, expression CC11 and ±2-semitone pitch bend are supported.
Pizzicato decays naturally rather than looping.

Synth-style controls: individual string-family levels, ensemble detune, stereo width, amplitude
ADSR, resonant low-pass filter, extra vibrato/rate, expression, reverb, tone and output.
Room Echo has its own amount, 40–800 ms time and bounded feedback controls, separate from reverb.
Echo starts off; earlier prototype presets load with echo off.
Movement: rate (0.05–10.05 Hz), depth, sine/triangle shape and volume/filter/pan/pitch destination.
Volume movement gives swells or tremolo; bowed vibrato and pizzicato use their own recordings.
The sample bank contains 200 zones and about 120 MiB of PCM shared across instances.
24 presets. Original ten: Solo Violin, Warm Viola, Cello Legato, Deep Contrabass, Chamber Strings,
Full Strings, Soft String Pad, Pizzicato Ensemble, Tremolo Strings and Breathing Strings. Added violin variations: Soft Solo, Bright Solo, Dark Solo, Warm Chamber, Wide Violins,
Expressive, Slow Swell, Tremolo, Pizzicato, Echo, Cinematic and Intimate Violin.
These use actual filter, envelope, layering, movement and room parameters; they are not
claims of separately sampled violin makes or performers. Original preset numbers stay unchanged. New Solo Legato Violin and Portamento Violin presets enable the connected-note mode.
Preset changes and user parameter state
use existing OS mechanisms. Data knob preset browsing remains controlled by the host.

Install: copy the supplied Melodee folder into the SD card root, merging with existing folders.
Do not delete existing plug-ins. Scan/restart the plug-in browser; choose Tape Workshop Strings.
This test ZIP contains only this instrument and its samples; it does not replace the OS.
Keep violin.bank beside tape_workshop_strings.so. A missing/invalid bank refuses loading.

Implementation: 16 bounded voices with up to eight sample readers per voice; immutable sample
bank shared across instances; bounded MIDI queue; all samples read before audio processing.
No allocations, locks or file operations in the render callback. CPU/polyphony must still be
measured on MPC hardware. The current OS source includes a custom violin face with sound-gated bow/string animation,
measured stereo waveforms and parameter diagrams. This requires a rebuilt Tape Workshop OS;
the standalone plug-in ZIP does not update the host interface. Sustains use generic crossfade loops requiring musical review.
Recorded vibrato remains present even with extra Vibrato set to zero. Ensemble detunes the
velocity-layer readers; it does not add separately recorded players or true legato transitions.
Upper/lower notes use nearest available samples, with no claim of a realistic extended range.

Build: python3 plugins/tape-workshop-violin/prepare.py, then build.py. Downloads and generated
samples stay under ignored build/. Source sample revision is pinned in build/violin-source/revision.txt
and recorded in the distributable SAMPLES-SOURCE.json. Code and source recipes are separate
from the SD-card installation ZIP.

Players page: four original string illustrations on a dark stage, shaded by their family mix.
PLAYERS / SOUND / LEGATO / FX are page-group indicators; existing Minus/Plus and navigation controls remain.
Solo Legato connects overlapping bowed notes on each MIDI channel, with last-held-note priority,
glide time (0–1000 ms) and maximum glide interval (0–24 semitones). It is a synthesized pitch
transition using the existing recordings, not separately recorded true-legato samples.
Legato is off in existing presets; normal polyphonic held chords remain available.
The new controls append to the parameter list; 0.1 and 0.2 chunks restore with Legato off.
No MPC install or audio test performed for these additions.

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
