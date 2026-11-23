> Historical v154–v156 notes. Chop Generator was removed in v159.
> Song Generator was removed in v168. Normal Song Mode and Chop Mode remain available.
> These are historical development notes; saved arrangements, generated notes and pads still play.

# Chop and Song Generators (v154)

Tape Workshop, based on Melodee by Kerem Kiliç (Ellic Studio).

## Chop Generator

Open a sampler track's Roll, press Data, and choose **Chop Generator**.
Use K1 to pick one setting, K2/Data to edit it, B3 Next, B1 Back.

1. **Source:** choose WAV track/pad, start bar, source length, and the WAV's
   original BPM. A source window is separate from generated loop length.
2. **Slice:** quarter, eighth, sixteenth, half, whole, transients, or saved manual cuts.
   K3 selects a chop; Start/End adjust it in 10 ms steps. Manual cuts are read
   from the source's existing Chop Mode data. Changes here do not rewrite it.
3. **Groove:** style, loop length, density, swing, pitch, repeats, reverse
   chance, gate, sequence BPM, variation. Up to 16 generated bars.
4. **Preview:** B2 Listen/Stop. K3 selects a hit; Data changes its chop.
   K1 selects Chop or Lock. Push on Lock protects that sixteenth-note slot
   (including its repeats) from regeneration. Pads audition or swap the
   selected hit; the slice selected on Slice chooses the A/B audition bank.
   Hold Shift+B2 to regenerate while previewing. Changes queue for next bar.
5. **Add to Roll:** choose Cancel, Keep + Add, or Replace Roll. One Undo
   restores both new pads and Roll notes. Existing sounds are never replaced.

Styles: Dusty Shaolin, Boom Bap, Soul Flip, Jazz Chop, Lo-Fi, Trap, Drill,
House, UK Garage, Breakbeat/Jungle, Experimental. These use original
rule-based rhythm/motif profiles; no commercial generator code is used.

Preview plays on the stopped sampler track with engine-only temporary pad
assignments; leaving, cancelling, starting the sequencer, or changing tracks
stops preview and restores those assignments. No project pad changes until
Add. The source WAV is shared, never copied or overwritten. After Add, use
normal Pad/Trim/Chop and Roll tools to refine regions, tuning and notes.

Limits: 4/4, sampler with 16 Levels off, at most 32 slices, and two empty,
unreferenced pads per slice (forward/reverse). Choose a shorter source window
or larger slice size when needed. The current track's effects/mix are heard.
Timing follows sequence BPM; slice cuts follow the source BPM. Pitch changes
playback rate; this is not independent pitch-preserving time-stretch. Gate and
silence between hits are intentional style choices. BPM changes affect the
preview clock immediately; pattern/chop changes switch at the next bar.

## Song Generator

Press **SHIFT + Pad 12** to open Song, then **SHIFT + B1 (SONG GEN)**.

1. **Genre:** choose one of 28 genres. Section order and lengths are automatic.
   Choose Genre default, Hook first, Verse first or Intro first for the opening.
   No named-song presets or manual length controls.
2. **Tracks:** K3 or pads selects a track. Data chooses Auto, Drums, Bass,
   Melody, Chords, Texture, or Off. Auto shows the inferred role; check it.
   Each track uses its current pattern. Empty tracks are omitted.
3. **Layout:** a visual section contour shows the arrangement. K3/Data browses
   section names, start bars and durations; total bars and estimated time are shown.
   Generate refreshes the arrangement without changing the project.
4. **Build Song:** stop playback, then choose Append or Replace. Existing
   arrangements are never silently overwritten. Append checks clip, marker
   and bar limits first. Replace replaces Song Mode arrangement/tempo/punches,
   not the patterns, samples, plug-ins or sounds. One Undo restores Song Mode.

The result opens in native Song Mode with editable clips and markers. Play
there to audition the arrangement. This arranges existing material; it does
not invent new melodies, lyrics, audio or independently separate a mixed WAV
into instruments. Long sample triggers follow normal sampler/clip behavior;
review them at section boundaries as with a manually made arrangement.

## Long chops (v155)

Groove now separates **Note span** (1/16, 1/8, 1/4, 1/2, 1 bar) from
**Gate length** (10–100%). Longer spans space new hits apart accordingly;
repeats subdivide that span. For held phrases, choose half/whole-note slices,
Note span 1/2 or 1 bar, Gate length 100%, Repeats 0%.
**Sustain loop ON** repeats the slice with a short crossfade until note-off;
OFF plays only to the audio's natural end. It does not time-stretch audio.
Both preview and added pads use the same loop/gate settings. Pads still stop
on release; hardware Stop ends preview. Source errors no longer trap you
on the Source page: proceed to Slice to choose a larger slice size. Failed
re-slicing clears stale slice counts. A labeled filename BPM is used as the
initial source tempo when available and the pad has no own tempo.

## Chop types (v156)

On Slice, K1 selects **Chop type**; K2/Data chooses Auto, Threshold, Regions,
Beats or Manual. B3 advances to that type's own amount setting: Auto target
8/16/32; Threshold 0–100% (higher keeps only stronger hits); Regions 2–32;
Beats slice size; Manual cut count. Auto, Threshold and Regions reuse native
Melodee chop functions and the already loaded sample analysis.

K3 selects a cut, and Start/End stay on separate screens. Manual starts from
saved cuts in the selected region, or one whole region if none exist. In
Manual on the Slice page, Shift+B2 splits the selected cut in half; Shift+B3
merges it with the previous one. Pads audition the cuts. Generate a new chop
pattern after editing. Changing type/amount re-slices and stops the temporary
preview; it does not write to the source WAV or existing Roll. All types keep
the 32-chop limit and refuse excess cuts with a message instead of silently
truncating the source. Auto's count is a target, not a guarantee for every
kind of sample (separate shots can produce a different detected count).
