# Hip-hop / trap MIDI melody composer

Tape Workshop implementation; host Melodee by Kerem Kiliç (Ellic Studio).

## Research references

- [Native Instruments: How to make a trap beat](https://www.native-instruments.com/blogs/blog/how-to-make-a-trap-beat), Tim Cant, November 22, 2022. Reference for sparse accompaniment, a small number of melodic parts, and leaving room around the rhythm section.
- [Attack Magazine: Dance Music Melody & Composition Techniques, Volume 1](https://www.attackmagazine.com/technique/passing-notes/dance-music-melody-composition-techniques-volume-1/). Reference for common tones, pentatonic thinking, arpeggio-derived material and varying note lengths/rhythms.
- [Kyle Adams: Harmonic, Syntactic, and Motivic Parameters of Phrase in Hip-Hop](https://mtosmt.org/issues/mto.20.26.2/mto.20.26.2.adams.html), Music Theory Online 26.2 (2020). Reference for considering motivic repetition and phrase structure alongside harmony.

These are technique references, not imported musical data. All onset templates, contour generation, scoring and MIDI output in `rap_phrase.h` are original. No melodies, commercial MIDI packs, audio samples or website code are copied or bundled.

## Implementation choices

Hip Hop and Trap now use a dedicated motif composer for Melody, Pluck and the melody part of Hybrid. With Genre Auto rhythm, an original two-bar onset design repeats across the timeline. Variation chooses another design and contour; the second phrase adds a restrained answering contour and cadence. Empty space is intentional. This is a deterministic music-theory heuristic, not a trained AI model or a promise of hit-quality output.

Candidate notes favor a repeated target contour, nearby pitches, pentatonic color and chord compatibility. Strong beats and endings use chord tones. Short weak-beat Melody notes may connect through the selected scale. Pluck and explicitly selected rhythms retain chord-tone-only pitch choices. Rate changes sparse/dense/pickup/triplet treatment; octave, gate and variation still work.

The existing scale mask travels with the generated harmony result, including WAV and MIDI analysis. Actual selected key/scale wins where supplied. Chord note data and source WAV/MIDI are not rewritten. Custom/borrowed chord tones remain available even when outside the global scale.

Manual rhythm selections keep their exact onsets. Chords, Bass Line, Original and the other genre profiles retain their prior paths. Hybrid still combines chords and a higher-register melody, without adding bass. Processing is bounded by the existing 4,096-note limit and happens outside real-time audio rendering.

## Validation

Tests cover every root, major/minor/pentatonic scales, three meters, all melodic outputs and rates, deterministic repetition, regenerated variants, short passing tones, monophonic bounds, a partial 26th bar, explicit rhythms, source preservation and MIDI round trips. Existing genre and rhythm regression suites remain required. Musical taste still needs audition on the MPC with suitable sounds.

## User-supplied audio reference study (v194)

Nineteen locally supplied commercial WAV loops were read for timing characteristics;
source audio was not modified, uploaded or bundled. Filename tempos were used as
reference grids. A spectral-flux analysis estimated attack locations and envelope
self-similarity. The trap group showed stronger median two-bar envelope recurrence
than one-bar recurrence. The slower hip-hop group had more offbeat attacks and
more detected attacks per named-tempo bar. These estimates include layers, chords,
reverb and delays: they are not note transcriptions or measured melodic note counts.
No pitch sequence, audio recording or extracted MIDI was copied into the generator.

The resulting design changes are original phrase families: Hook (small repeating
contour), Rolling (more continuous movement), Dark/Soulful (genre-specific syncopated
motif), and Space (fewer, longer gestures). The existing Pattern control selects
these for Hip Hop/Trap; other genres keep their previous Pattern labels/behavior.
Genre Auto exposes their rhythmic differences. Explicit rhythms retain exact onsets.
