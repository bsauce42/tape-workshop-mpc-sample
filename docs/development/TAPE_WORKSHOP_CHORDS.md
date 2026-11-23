# Tape Workshop v137 — Chord Generator

Based on Melodee by Kerem Kiliç (Ellic Studio). Original Tape Workshop chord
analysis, progression generation and UI; no Unison code, assets or model used.

Open a melodic track's Roll, open its existing Actions menu, and choose
MIDI GENERATOR. Existing Roll editing and shortcuts remain available after B1.

Progression mode (default): generates 4 or 8 bars using the project Key/Scale.
K1 selects a setting, K2 or the Data knob changes it, K3 selects a chord.
B2 GENERATE applies changed settings; B2 NEW CHORDS makes an alternative.
Use GENERATOR MODE to switch to FOLLOW MIDI: this analyses the current Roll
notes (only marked notes if a selection is present). KEY SOURCE can follow the
project or infer major/minor from the melody. Follow modes: Every Note, Beat,
Phrase (default), Bar and Auto. Follow Strength defaults to 85%; triads,
sevenths and ninths are independently enabled, with voice leading on.

Push the Data knob to audition the selected chord through the current instrument
while stopped. Inversion, start tick and duration edit the selected generated
chord. Pad playing remains available. Chord names and actual note names display.
Choose TARGET TRACK, then B3 ADD TO ROLL to add the generated sequence directly to the selected target
track (default: the current instrument track). If that Roll already contains MIDI
notes, a confirmation offers B1 CANCEL, B2 KEEP + ADD, or B3 REPLACE. Cancel leaves
it unchanged; Keep retains existing notes and adds chords; Replace removes its
notes before adding chords. Automation stays. One normal undo reverses the edit.
Drum/audio and frozen targets are refused. Set the target instrument first.
Normal MPC transport then plays the Roll.
Original melody events are changed only after you explicitly choose REPLACE.

Shift+B3 EXPORT MIDI remains an optional copy; it writes only generated chord notes, tempo and time signature as
Type 0 MIDI to SD/Melodee/MIDI/TW_CHORDS_<tempo>BPM.mid. Existing files gain a
numeric suffix rather than being overwritten. SD write/verification errors appear
as notices. Settings changes require B2 generation before export or Add to Roll.
The chord workspace is session-only; save MIDI or place chords on a track and save
the project to keep the result. The standalone melody players inside Strings and
Orchestra are audio-only; use Roll notes for Follow Melody input.

Implementation: bounded theory-based scoring/voice leading with deterministic
variations; no AI runtime, new audio engine or desktop SDK. At most 2,048 input
notes and 256 chord regions; MIDI generation and storage run outside the audio
callback. Storage uses exclusive file creation, fsync and byte verification and
works without hard links on FAT/exFAT. Existing background SD service handles export.

Validation: 4/8-bar progression generation, fixed/detected keys, five follow modes,
complexity combinations, variation, strength effects, source-note preservation,
exact exported-note/timing/velocity reimport with the existing SMF reader,
filename collisions, invalid paths/overflow and directory escape checks. Desktop
screenshots use actual UI/model. Hardware audio audition, SD removal/full-card
behavior and worst-case CPU remain untested in this revision. This is a testing
build, not a claim of parity with any commercial chord generator.

## Whole WAV (v137)

Load your WAV on a sampler/keygroup pad. On an instrument track, open Roll Actions
> MIDI GENERATOR. Set GENERATOR MODE to WHOLE WAV, WAV TRACK to the loaded
sample's track and WAV PAD to its pad (A01–H16). B2 ANALYSE WAV streams the entire
original file through background spectral/chroma analysis; progress shows a
percentage and B1 cancels. The audio file, pad region, and sample remain unchanged.
Analysis uses the original unpitched file, including audio outside pad trim points;
it does not include live effects or time-stretch processing.

It estimates successive chord regions, including changes outside the first five
seconds and changes away from the initial key. This is chord recognition, not
random key-only progression generation and not exact extraction of every played
note. Mixed drums, melodies, detuning, sparse tones and complex voicings can give
ambiguous results. Correct inversions/timing manually and audition before keeping.
No commercial AI/model or proprietary library is bundled.

Set project BPM/time signature first. Actual audio seconds map to 96-PPQ Roll
positions at that tempo; this does not automatically detect BPM, warp the WAV,
or add an audio trigger to the Roll. Follow Every Bar uses bar windows; other
modes use beat/quarter-beat evidence with adaptive merging. The analyzer stores at
most 256 windows, coarsening their spacing for long files; rapid changes below
that spacing can be missed. It reads all samples through the end, never a five-
second excerpt. Files that cannot fit the existing 128-bar Roll at the selected
tempo are refused with a notice, not silently truncated. Choose a lower tempo or
trim a copy if needed. Maximum decoded file length is 24 hours, well beyond the
supported Roll duration; no full-file PCM buffer is allocated.

B3 ADD TO ROLL immediately opens editable MIDI notes (or asks Keep/Replace/Cancel
if notes exist). It does not require writing/importing an SD MIDI file. Pattern
length rounds up to a bar to preserve the complete detected ending. Save the
project to retain the result. SHIFT+B3 can still save an optional MIDI copy.

Implementation: one bounded load-pool job, fixed 4096-point FFT/ring at 6 kHz,
<=256 chroma windows, bounded candidate/Viterbi scoring and smooth voicing. No
card I/O, allocation or FFT is added to the audio callback. The main service
thread continues handling streaming/loads while a lower-priority pool thread
analyses. Cancellation, bad/short reads, analysis failures, pitch/timing bounds,
maximum note counts and duplicate-note protection are handled before mutation.

Checks: synthetic C/F/Am/G changes across a whole stream, tail coverage, silence,
window bounds, original-pattern preservation, keep/replace edits, duplicate
protection, length/invalid-data errors and earlier MIDI/progression regressions.
The selected Candlelit recording was also processed offline through its full
length. These checks do not prove exact transcription, musical accuracy for all
recordings, or MPC worst-case CPU limits. No live project notes were replaced as
part of these checks.

## Bass Line output (v139)

Open an instrument track's Roll Actions > MIDI GENERATOR. Use the same
Progression, Follow Melody or Whole WAV input as the chord generator. For a WAV,
choose WAV TRACK and WAV PAD, verify WAV FILE, and press B2 ANALYSE WAV. Then use
K1 to select OUTPUT and K2/Data to choose BASS LINE. Switching output or changing
bass controls reuses the existing chord analysis; no additional WAV scan is needed.

BASS STYLE offers Held Roots (one root per detected chord), Quarter Pulse,
Root + Fifth (alternating root and the chord's own fifth, including diminished/
augmented fifths), and Octave Pulse (eighth-note root/octave alternation).
BASS OCTAVE selects C1, C2 (default) or C3 as the root register. BASS GATE sets
10–100% of each note's available duration, default 85%. Chord changes reset the
pattern to its new root; generated notes do not cross chord boundaries or overlap.
Silent gaps in the detected chord sequence stay silent. This generates a new bass
accompaniment from estimated chords rather than transcribing a recorded bass player.

K3 selects a harmonic region. Push previews that region's bass root through the
selected target instrument; it does not audition the entire rhythm. Pick a bass sound on
the target instrument track, then B3 ADD TO ROLL. Existing MIDI notes prompt
CANCEL / KEEP + ADD / REPLACE exactly as for chords, with one normal undo.
The original WAV and chord-analysis result remain unchanged. Save the project to
retain the editable bass MIDI. Shift+B3 optionally exports only the bass notes to
Melodee/MIDI/TW_BASS_<tempo>BPM.mid; regular chord output/export remains available.

Bass generation runs outside the audio callback, with a 4,096-note bound. An
excessively dense result is refused before any Roll edit; use Held Roots to reduce
density. Direct Roll import shares validated note bounds, exclusive Keep/Replace
semantics, exact-duplicate protection and preserved automation with chord import.
Duplicate checks use a bounded note-signature set to avoid repeatedly searching
all existing notes. No additional audio analysis, synthesizer or DSP is introduced.

Validation: all four styles/three octaves, detected-root changes, fifths/octaves,
non-overlap and note boundaries, gate settings, exact bass MIDI reimport,
Keep/Replace source preservation, density/invalid-data refusal, and existing chord,
audio-change and Roll-edit regression checks. Desktop UI previews use the actual
model/rendering. On-device bass playback and musical listening review remain
unverified until the update is installed and tested.

## Pluck, Melody and Hybrid outputs (v140)

The Roll Actions entry is now MIDI GENERATOR. OUTPUT has Chords, Bass Line,
Pluck, Melody and Hybrid. All five reuse the same Progression, Follow Melody,
or Whole WAV harmony input, and share B3 Add to Roll, Keep/Replace/Cancel, undo,
and optional Shift+B3 MIDI export.

PLUCK makes a short arpeggio from each detected chord's tones. PATTERN chooses
Up, Down, Bounce or Shuffle. MELODY creates a deterministic lead phrase with
chord-tone emphasis, contour, held notes and rests. Its PATTERN choices are Rise,
Fall, Wave and Explore. TOP RATE (1/4, 1/8, 1/16, 1/8 triplet), TOP OCTAVE (C3-C6),
TOP GATE (10-100%) and NEW PHRASE (0-999) edit the selected output. Pluck and Melody
have independent settings. New Phrase changes notes/rhythm without another WAV
scan and without changing the detected chords. Rates count MIDI quarter-note
values consistently across time signatures.

HYBRID combines CHORDS + MELODY ONLY, as requested. It adds no bass or pluck part.
Chord pitch classes occupy C3-B3; the melody is kept at C4 or higher so the two
parts do not compete for the same MIDI pitch. Selecting Hybrid can raise a low
melody octave to maintain this separation; the displayed setting updates. Its
TOP controls edit the melody. Standalone Chords retains the original voicings.
All Hybrid notes go to one target Roll and use that track's instrument. This is
not a separate-instrument multi-track arrangement.

On the destination track, choose a pluck/lead or suitable layered sound first.
Then open Roll Actions > MIDI GENERATOR, analyse the WAV or generate harmony,
select OUTPUT, adjust TOP controls and press B3 ADD TO ROLL. Push previews a
representative note/voicing for the selected region, not the entire phrase.
The generated notes are editable in the existing Roll. Save the project to keep
them; SD export is optional (TW_PLUCK, TW_MELODY and TW_HYBRID prefixes).

Bounded, theory-based generation runs outside the audio callback. Combined
outputs above 4096 notes are refused before editing; lower Top Rate if needed.
No audio effects, source WAV, plug-in sounds or recording engines are changed.
These are new MIDI accompaniments following estimated harmony; they do not
extract the recording's original lead or pluck performance.

Tests cover five outputs, pure two-part Hybrid (no bass/pluck), separated pitch
registers, source-harmony preservation, chord tones, rhythmic boundaries,
monophonic Pluck/Melody, meaningful deterministic variations, independent
settings, representative preview bounds, exact MIDI reimport, density refusal,
and Keep/Replace edits. Prior bass/chord/audio-analysis checks are retained.
Hardware audition and musical quality still need listening review.

## MIDI source on another track/pad (v140)

Import a MIDI file into a source Roll, then open MIDI GENERATOR on the destination
instrument track. Choose GENERATOR MODE > FOLLOW MIDI. The latest successful MIDI
import is remembered during this session: the next generator opening automatically
chooses FOLLOW MIDI, with MIDI TRACK and MIDI PATTERN preselected.
You can choose any other stored track/pattern manually, including a nonplaying
pattern. A small footer confirms source track, pattern and note count after generation.
All five outputs use that MIDI source; TARGET TRACK remains a separate destination.
B2 generates, B3 adds to the destination Roll with Keep/Replace/Cancel protection.
The source is not modified unless you deliberately make it the destination and
choose Replace. Save the project to keep both source and generated notes.

MIDI READ > MIDI NOTES reads stored note keys directly (plug-in, keygroup, MIDI
and imported raw MIDI-key Rolls). Key/Scale can follow the project or use DETECT
MIDI to estimate major/minor from the source. A selected subset is honoured only
when the source is the same track/pattern as the currently displayed Roll; a
selection in the destination never filters another source track accidentally.
Muted/zero-velocity notes are excluded. More than 2048 source notes is refused
rather than silently dropping the ending.

For one sampler pad's pitch Roll, select MIDI READ > PAD PITCHES and MIDI PAD
A01-H16. This filters that pad's lane and decodes its actual Roll pitch offsets,
not the pad's trigger number. Importing MIDI while a sampler's 16 Levels pitch
Roll is active now places the melody pitches on that selected pad. The existing
pitched-pad range is MIDI 12-108; imports outside that range are refused before
replacing anything. Use a plugin/keygroup/MIDI track for the full 0-127 range.
Normal drum-layout MIDI imports retain the existing 36-and-up pad mapping. Ordinary
drum trigger numbers are not a reliable melodic source; choose the actual MIDI
note Roll or a pitched-pad lane. Existing source patterns from older builds can
be selected without reimporting if their note data is already intact.

Input source settings and last-import selection are session-only, while the
actual MIDI notes persist with the project. Across restart choose the stored
source track/pattern again. Source note ticks/durations are reused at 96 PPQ;
the target/project tempo controls playback, not the original MIDI file's tempo
metadata. Tests verify exact imported pitch/timing/velocity, pad filtering,
source immutability, all five downstream outputs, muted-note handling, range
and note-count refusal, and no partial edit on a bad pitched-pad import.

## Apply the WAV's detected BPM (v140)

In Sample Select, turn AUTO BPM on before loading a rhythmic sample. Once the
import dialog shows DETECTED BPM, hold Shift and press B2 USE WAV BPM. This
sets the current project/sequence tempo through the normal tempo controls and
keeps the sample at its original speed. It does not alter the source file.
B2 without Shift still applies TIME WARP; B3 and Shift+B3 still preview original
and warped audio. If this same file is already loaded/selected, toggling Auto BPM
OFF then ON reopens its detected-tempo choices without another import.

The estimate comes from the existing audio beat detector, not a filename guess.
A one-shot or ambiguous unpitched sound may have no steady BPM; no tempo is then
applied. Tempo must fit the existing 30-300 BPM control range. Review possible
half/double-time estimates before applying. Normal global/per-sequence tempo
mode and external clock behaviour are preserved. Reanalyse Whole WAV after
changing project tempo so generated ticks align with the intended tempo.


## Guided navigation (v141)

The default generator is a five-step wizard: Source, Output, Target, Preview,
Add. It reuses the v140 harmony model, MIDI source selectors, phrase outputs,
background WAV analysis, protected Roll edits and MIDI writer. The new UI does
not generate until explicitly requested. Data adjusts the highlighted field;
Push/B2 advances through Source fields then steps. Preview Next refuses empty
or stale results. Adding is confined to the final step and retains the original
occupied-Roll confirmation. B3 Options exposes the prior parameter list with
Done/Generate/Listen buttons. Shift+B3 saves MIDI in Options or the final step.

Native tests cover source branches, five-step forward/back navigation, field
selection and bounded option rows. Screen previews are rendered from the actual
UI, with a text-fit audit at the device's 320x240 size. Hardware validation is
separate; do not treat preview captures as on-device screenshots.

## Bass rhythm expansion (v143)

`BassSettings::rhythm` adds 24 choices independently of the four original pitch
styles. Zero retains their legacy timing. Explicit divisions use the fixed
quarter-note PPQ. Named patterns are original bounded hit/duration/accent tables,
locked to project measures; two-bar patterns retain phase across chord changes.
A new root at each chord boundary follows harmony even in a groove rest; note
lengths clip at subsequent hits and chord ends. Gate is applied afterward.

Bass MIDI output now receives actual project bar ticks in both Roll insertion
and MIDI export. The guided Output step asks for rhythm when Bass is selected.
Bass audition schedules up to two bars of generated notes on the UI/control
thread, using a fixed 128-note queue and latched tempo. It skips expired notes
on delayed ticks and sends note-offs on finish, cancellation, parameter edits,
leaving the tool, recording or starting transport. No DSP/audio callback work
was added. The preview is not a sample-based 808 engine; the target instrument
supplies the sound. Rhythm settings, like existing generator settings, last for
the session; save generated notes with the project.

Tests cover every rhythm/pitch-style combination, exact note timing/durations,
MIDI reimport, roots at chord changes, monophony, partial tails, 3/4 bar timing,
count limits and source immutability. Earlier output and Roll tests remain.

## Selected-bar editing and octave controls (v144)

Bar candidates strictly reject every chord containing a pitch outside the project
root/scale mask, rather than merely scoring it down. Replacements split crossing
notes to preserve portions outside the chosen interval, retain event metadata,
refuse overflow/collisions and apply in one undo edit. A fingerprint, track/pattern
owner and key check reject stale previews. Bar selection uses a range predicate,
so it is not limited to the ordinary 128 manual marks. Octave edits validate the
whole operation before mutation, including pitched sampler lanes. Plain drum
transposition/chord generation is not offered. Bass C0-C3 numbering extends below
the former C1 lower bound while keeping the default at C2.

The visible tempo chooser reuses the existing detector and set_tempo controls.
`tools/workshop_runtime.py` patches the packaged startup worker to copy/verify the
voice resources into the runtime; it deliberately does not edit the base submodule.

## Rhythm for every output (v146)

The guided Output step now asks for Rhythm for Chords, Bass, Pluck, Melody and
Hybrid. All 24 patterns are available; each output remembers its own selection
for the session. Style Default preserves the old behavior. Explicit patterns
replace the onset rhythm while retaining chord tones and the chosen contour.
Hybrid applies its rhythm to chords and melody together, with no bass layer.
Rate applies only to Style Default; Gate still shapes pluck/melody durations.
Listen to Rhythm auditions up to two bars from the selected chord using the
current instrument, with bounded polyphonic note-offs and cancellation.
Generated notes still use the existing safe Roll Keep/Replace and MIDI export.
Native checks cover all five outputs and 24 patterns in 4/4 and 3/4, MIDI
roundtrips, source preservation, count refusal and audition cancellation.

## Sample Select tempo accuracy (v146)

Audio tempo now follows measured beat spacing without forcing the result to
fit the whole file duration, which may include a release tail. Auto BPM first
uses a clear filename label such as `93 BPM`, `128bpm` or `BPM: 87.5` (30–300
BPM); conflicting labels and ordinary unlabelled numbers are ignored. Only the
filename is read, not parent folders. The chooser identifies Filename BPM or
Audio BPM. Filename metadata affects import tempo; it does not fabricate or
replace audio beat/chop markers. Labels may themselves be wrong, so review the
value before Apply. Audio estimates can remain ambiguous on free-time material.
The reported WAV reproduced 90.9 BPM before the fix and 93.1 from audio after;
its explicit 93 BPM label supplies exactly 93.0. Synthetic pulse tests with and
without tails cover 60, 93, 120 and 155 BPM. Physical-device checks are pending.

## Shared musical genres (v149)

The guided Output page now selects Output > Genre > Rhythm. Original (ID 0)
retains the previous generator behavior; IDs 1–28 use the same names/order as
the drum generator. Genre is available on project-key, MIDI-follow and whole-WAV
sources. It never changes project tempo, key or scale. Candidate quality weights
and progression paths affect new harmony selection; whole-WAV scoring uses a
small quality bias so audio evidence stays dominant. Enabled complexity flags
and existing voice-leading/follow controls remain authoritative.

`music_genres.h` provides original profiles: chord/phrase rhythms, progression
paths, chord-extension preferences, arpeggio rotation, melodic movement,
velocity offsets and swung off-eighths for appropriate genres. Genre Auto uses
these profiles for Chords, Pluck, Melody and Hybrid. Explicit rhythm selections
retain their onset grid (including Hybrid's shared manual rhythm). Bass retains
its own existing rhythm choices. The optional swing delays only exact off-eighth
onsets and shortens their durations by the same amount; notes remain inside
source chord regions. Octave, gate and contour controls remain available. Rate
is shown for Original/Style Default only, since genre timing supersedes it.
Hybrid remains chords + melody with separate registers and no bass layer.

All generation remains off the audio callback. The 4096-note bounds, source
preservation, preview clock, MIDI export and safe Keep/Replace edits remain.
Native coverage includes 224 genre/output/meter cases, round-trip MIDI timing,
chord-tone membership, 27 distinct melody profiles for the fixture, and explicit
rhythm override; legacy Original rhythm/phrase checks remain. Drum templates
share vocabulary only and retain their v148 behavior. Physical v149 checks are
pending. Generated notes save normally; genre settings remain session-local.

### Direct Roll generation

`HarmonySetup` stores only settings/source selectors (no result or note buffers),
in a bounded 16-entry UiState array keyed by Roll track. Successful generation
remembers the setup for both the source/current Roll and the chosen target.
Normal opening restores that track's setup, except the explicit MIDI import
flow. Generate Again restores it, advances bounded variations, and starts at
Preview; Generate From Roll forces the current Roll/selected notes as its MIDI
source. Both use the existing worker/model, preview and Add confirmation, never
writing as part of generation. Project reset/restart clears this session cache.
The card-sheet capacity grows from 12 to 16 to accommodate the two Roll actions;
visible layout/scrolling remains unchanged. Native snapshot/restore/variation
checks cover setup isolation and direct Preview entry.

## Explicit generator scales (v150)

Scale choice zero retains project/legacy behavior (labelled Project, Auto/MIDI
or Auto/WAV to match the actual source workflow). Choices 1–13 select the existing
scale masks with the project root; neither root nor scale on Project is modified.
The new `strict_scale` model setting defaults false. Explicit selection enables
candidate filtering in progression, MIDI harmonization and audio recognition.
The progression searches nearby valid chord roots when its preferred degree
has no compatible enabled chord. Empty candidate sets return no result safely.
Pluck/melody/hybrid retain chord-tone generation, so all generated notes obey
that explicit scale. Unsupported scale/complexity combinations refuse rather
than snapping notes and displaying an incorrect chord name. Setup snapshots
retain scale selection; project key changes invalidate pending results.

The guided Output step and Options both expose Scale. Row 33 is distinct from
Preview/Add action rows 31–32; setting bounds and card labels handle it directly.
The duplicated Genre entry in advanced Options was removed while adding Scale.
Native tests cover all 13 scales and 12 roots across the four requested outputs
(624 checks), MIDI/audio recognition membership, impossible masks, remembered
setup and guided navigation. Project Scale defaults remain compatible with the
prior genre/rhythm tests. Device/UI checks pending.

## Whole-WAV final bar (v153)

A quiet or silent ending no longer drops the final generated bar. After a
pitched harmony is detected, the last chord continues to the WAV's exact end,
including a partial final beat/bar. This is a continuation of the last detected
harmony, not a newly detected chord in silence. Rests inside the recording stay
unchanged, and an entirely silent recording still produces no chords. Generated
outputs use this full-duration harmony; their selected rhythm and gate still
control note spacing and release. Re-generate an older result to apply the fix.

WAV duration is quantized to the nearest MIDI tick before rounding the Roll
to bars. Audio frame rounding just beyond a bar boundary no longer creates an
empty extra bar. Verified example: Cymatics Business, 1,840,696 frames at
44,100 Hz and 138 BPM, is 9,216 MIDI ticks / 24 bars, not 25. Real partial
beats and bars remain included; the source WAV is never changed.
