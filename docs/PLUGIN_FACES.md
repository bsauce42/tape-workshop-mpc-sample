# Tape Workshop plug-in faces

Tape Workshop by bsauce42, based on Melodee by Kerem Kiliç (Ellic Studio).

The host's instrument faces and Shift listening views use measured stereo
output waveforms and peak levels in dBFS. The display reads the existing bounded
output snapshot; it does not change DSP, routing, presets or knob assignments.
Silence stays flat. Bypassed listening views suppress the signal display.

The visual identity follows the instrument: warm piano panels and held-note
keys, digital FM control racks, analogue wave/spectrum panels, atmospheric stereo
panels, drum transient displays, tape styling, dynamics meters and vocal pitch
readouts. Each named instrument has its own palette. Spectra are measured output,
not filter response estimates. Peak meters are not loudness/LUFS meters.

Sub Foundry, Contour Comp and Prism EQ retain their existing interfaces,
including their Shift displays. No other plug-in binaries need rebuilding.

Validation for v107: ARM64 host builds; 36 native rendered instrument/listening
previews; direct pixel comparison of the three protected listening views against
the previous renderer; unchanged Sub Foundry view source; silence is independent
of the clock; stereo output-capture and signed/complete-package tests pass.
Previews use controlled signal fixtures, not a live MPC capture. Physical-unit
verification remains required before describing this version as hardware-tested.

## v108: Prism EQ readability

Prism EQ keeps its band colors, curves, spectrum, balance feedback and physical
controls. The automatic overlap source-name banner is removed from the main
plot; Shift still opens the detailed overlap list. Removing the banner and
compacting the surrounding title spacing gives the standard EQ plot 25 extra
vertical pixels. A stable zero-gain mark replaces the changing source caption.
No filter coefficients, gain scaling, analysis or preset data change.

## v109: parameter banks and vintage Second Spin

Parameter banks after the overview now select a control-specific layout from
the loaded names: envelope stages, filter/spectrum, modulation dials,
pitch/selector, space, mixer faders, timing rulers, and voice controls. Each
bank retains the plug-in palette and uses the live host parameter values;
related banks reuse common widgets. Preset browsing keeps its own view, and
Shift keeps the larger instrument listening view. These are host layouts,
not embedded vendor interfaces or invented DSP response curves.

Second Spin uses a vintage compact-cassette silhouette at approximately
100:64 proportions. Its tape window, hubs, label stripes and screw details
stay distinct from the output needle and measured waveform in the side bay.
The needle is a stereo output-peak meter in dBFS; reel motion follows playback
or signal activity and stops when bypassed. Parameter banks retain the tape
styling, with blend faders, transport selectors and tone/control banks.

Sub Foundry and Contour Comp are unchanged; Prism EQ retains the v108 enlarged
plot without automatic overlap-name banners. DSP, physical knob bindings and
preset serialization are unchanged. Native render checks cover all eight bank
kinds and protected listening views; hardware verification is still pending.

## v110: analog output meter

The redesigned Tape Workshop plug-ins now append one editable METER bank after
their existing parameter banks. Turn Data or use the usual page navigation to
reach it. Existing shown-parameter indices are unchanged. The new bank aliases existing
controls: Second Spin maps K1-K3 to Input Gain, Output Trim and Mix; the other
redesigned plug-ins expose their first three controls beside the meter. Sub Foundry, Contour Comp and Prism EQ are excluded, as
requested. Third-party plug-ins are not given an extra bank.

The analog face has a recessed dark housing, graduated cream dial, fixed paper
grain, glass edging and a red-tipped needle. Its number and needle both show the
actual recent stereo output peak in dBFS (-60 to 0 on the dial), not gain/clip
reduction or calibrated analog VU. Silence displays -inf; bypass suppresses the
reading. The design is original, inspired by vintage meters rather than copied
vendor artwork or branding.

Second Spin's first page uses this same meter face beside a correctly
proportioned cassette marked TAPE WORKSHOP. Its Shift listening view retains
the stereo waveform. The additional meter bank shows the analog face by itself.
The source-level meter navigation test checks clamping, unchanged parameter
indices, reordered skin mappings and protected plug-ins; native render checks verify -6.0206 dBFS for
half-scale left-only/right-only output and bypass behavior.

## v111 — individual panels and audio-driven cassette

Seventeen instruments/effects now have explicit bank layouts: piano cabinets,
choir stage, FM operator offsets, dual wavetable views, spectral history,
cream modular panels, pitch display and dynamics racks. Second Spin retains
its dedicated cassette. K1–K3 edit the actual named parameters on each page;
the meter bank retains its editable aliases. These are original native panels,
not ports of the desktop products shown as references.

Second Spin advances its reels from fresh measured output audio, including
pad audio when transport is stopped. Silence, bypass and stale audio snapshots
hold the reel position. Existing audio taps supply the visual data; no extra
allocation, locking or I/O was added to audio processing.

Output waveforms, spectra and phase plots use measured audio. Envelope/effect
control diagrams are labeled as control shapes, not measurements of internal
DSP signals. FM level bars show factory offsets rather than invented routing.
Sub Foundry, Contour Comp and the enlarged Prism EQ remain unchanged.

Native fixtures render every parameter bank and check parameter editing and
protected live-view pixel parity. Screenshots use controlled test audio, not
an MPC capture. Hardware playback and performance testing remain outstanding.

Based on Melodee by Kerem Kiliç (Ellic Studio); Tape Workshop by bsauce42.

## v112 — live source displays and neon Afterglow

All voice/source-mix graphic panels now show measured stereo output waveforms,
including Astral Loom's first page, focused listening view and preset preview.
Source and mix controls retain their original assignments. Afterglow Piano
uses a black/green cabinet, hot-green accents and a glowing green frame.
The complete distribution includes Room Grand 1.3.1, with softened key-release
noise and Release Noise set to zero throughout its factory presets.

## v114 — direct Data preset browsing with names

Turn Data directly on the plug-in UI to browse sounds; the panel displays
the selected preset name while scrolling, including pending selections.
The original B2 PRESETS menu remains available as an optional full list.
The ordinary plug-in panel and K1–K3 assignments are restored to v112.
On the preset list, Data auditions sounds while staying in the list;
Push enters folders. Minus/Plus do not change the list selection.

## v116 — Second Spin Crackle returns after zero

Reproduced on the MPC with the effect enabled: Crackle at zero becomes silent;
turning it back to 50% still produces silence until a new note wakes the effect.
The live display shows 50% and the chain remains enabled. The idle gate was
only waking for MIDI/input audio, not parameter edits.

Parameter writes now post a lock-free wake request. The audio thread consumes
it and resets its own quiet countdown. No allocation, audio-thread locks or
forced bypass changes are added. A control at zero may still sleep to save CPU;
turning it up wakes it without requiring a new note. Tests cover the real
Second Spin engine across repeated zero/positive cycles and the normal idle
sleep countdown. `tests/plugin_parameter_wake.cpp` links against `tape.c` and
Open MPC's `wrapper/engine.h` from the matching plug-in source bundle.

## v117: browser artwork

The on-device instrument and effect pickers now show a static UI preview beside
six navigable name rows. All 21 bundled Tape Workshop plug-ins have distinct
artwork taken from their own host UI (the protected Sub Foundry, Contour Comp
and Prism EQ views are unchanged). Turn Data to change the selection and preview;
push Data to load, and B1 still goes back. Long list names retain scrolling.
Folders and third-party plug-ins without artwork show a folder or type icon.
Artwork is embedded in the OS and only the selected image is decoded/cached on
the screen thread. Browsing does not instantiate a plug-in or run its DSP.
The source PNGs live in `src/melodee/ui/plugin_previews`; regenerate their embedded
header with `python3 tools/plugin_preview_art.py` after updating an image.

## v118: Melody Forge

Melody Forge 1.0 joins the complete library as plug-in 22. The current host
includes its four-layer face, real output waveform, parameter-driven filter and
envelope plots, 21 three-knob pages, and Data-knob preset browsing. Its 80
factory presets are grouped into Trap, HipHop, R&B, EDM and DnB. The picker also
shows Melody Forge artwork. Keyboard mode plays directly; Melody mode plays a
phrase while a note is held. Host Sync follows project tempo; Manual uses its BPM
parameter. Acoustic-style sounds are synthesized approximations.

Only the Melody Forge plug-in, matching source and custom host UI are imported
from its distribution; the older v46 autorun is excluded. The v116 idle-wake fix
and v117 browser artwork remain in this build. Physical MPC playback and load
for the newly integrated Melody Forge still require hardware verification.
