# Tape Workshop Drum Generator

Based on Melodee by Kerem Kiliç (Ellic Studio).

v147 adds control-thread rule-based drum generation, not a new audio engine.
`model/drum_generator` owns 28 original genre templates, bounded 1–16-bar
4/4 generation, deterministic xorshift variation, accented velocities,
percentage density/fills, off-sixteenth swing, and per-role locked hit reuse.
Eight logical roles are mapped separately to sampler pads A01–H16, with Off.
Auto-map uses names in the existing kit; unknown names remain unassigned.
It never replaces samples, pad parameters, kit routing or track engines.

`drum_tool` guards owner, pattern, recording/rendering/freeze state, 4/4 and a
fingerprint before commit. Keep/Replace constructs the entire proposed Pattern
before one undo transaction. Keep preserves events and deduplicates at pad/time;
invalid mapping, empty mapped pads and note overflow refuse without mutation.
A fixed 512-note control-thread preview covers at most two bars and sends
note-offs before new onsets. Stop/Play, cancellation, leaving Roll or changing
owner stop the audition. Delayed ticks skip expired notes. Sample tails retain
normal kit playback behavior. Project/sequence tempo remains untouched.
The same existing preview clock is generalized over its state type; no behavior
change for the chord generator. Screen rendering remains off the audio callback.

Native tests cover 840 genre/length/density/swing combinations, deterministic
seeds, locked lanes, A-H pad mapping, duplicate mapping refusal, disabled roles,
Keep/Replace/source preservation and bounds. Existing polyphonic preview tests
cover cancellation and delayed ticks. UI preview uses actual native rendering.
On v147 hardware, the kit auto-map, lane lock, generation, two-bar audition
and Keep + Add into a separate demo Roll were verified. v148 suppresses the
underlying Roll knob cards and Shift strip while the generator owns its controls.

Deliberate initial limits: sampler drum Roll, 4/4, 1–16 bars. Eight roles;
unknown/unmapped samples are silent. Settings are session-local; generated MIDI
notes save normally with the project. No separate built-in sound bank or
standalone audio/MIDI export is added by this tool; normal project/sequence
export and Roll editing remain available after adding. This is an original
implementation inspired by the requested workflow, not copied commercial UI,
assets, code or learned pattern data.

## Focused UI and sound selection (v151)

A shared `generator_focus` renderer uses large labels and 30-point values, with a
bounded clipped marquee for long values instead of shrinking them. MIDI guided
and advanced settings and Drum Style/Pad/Lock show one field. K1/value controls
retain their jobs; Drum Next/Back walks individual fields before changing groups.
Drum Preview draws two enlarged bars for its K3-selected role.

The role sound picker uses the existing Sample Select/load transaction. It holds
owner/role/pad, chooses an assigned pad or an empty unmapped pad, and validates
that asynchronous completion belongs to that request. Kit/bank/MIDI/instrument
imports are blocked in this scoped picker. Browser load retains normal existing
pad settings/Undo. Success maps the role and returns to Roll while retaining the
generator state; ordinary browser cancellation resumes the same state without
assigning a new role. Drum loads bypass Auto BPM's import dialog without changing
its global setting. Track/page changes still stop previews. No audio engine or
real-time audio callback changes.

Native checks cover field traversal, destination selection and completion-owner
guards. All advanced MIDI fields render through text audits; physical picker and
return/undo checks remain pending until installed on hardware.
