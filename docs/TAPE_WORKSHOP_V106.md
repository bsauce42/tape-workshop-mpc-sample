# Tape Workshop v106 — Melodee v0.5.1 base

Based on Melodee by Kerem Kiliç (Ellic Studio).

## Source and merge

- Supplied archive: `Melodee-MPC-Sample-v0.5.1.zip`.
- Archive SHA256: `3cf366842082af7ffddef054fc346ca4af168bf7c1290323417aad64ccdb41eb`.
- Upstream source: `f11466589c7d28a3dd386c15a30743a6abc23fa3`.
- Runtime base: `f18e620104fe579727c5f13f796016ba08e3a394`.
- Previous Tape Workshop: v105, `35e3d2b18135e55f2ca90ab6f682391c17ee8d3d`.

The original Melodee v0.3.0 snapshot imported at `2c38157` was used as the
three-way content baseline. This avoids treating that snapshot's imported files
as independently added files. The resulting merge records the official v0.5.1
commit as a parent for subsequent updates.

## Preserved Tape Workshop features

Roll MIDI import, selection, Shift + K3 zoom and persistent fine splitting;
Prism EQ and native plug-in visuals; overlap readings; sample search/voice search,
custom sample-folder and auto-BPM import; expansion browser, cover art, bank/pad
import and Empty Pad; all eight browser builder banks; direct stereo browser audio
and tutorial recording; black browser styling; all Workshop themes; logo/icons,
all-pad startup animation and track-color lighting; fork-specific signed updates.
Studio/the browser DAW stays removed. Desktop companion/plug-in-builder apps are
separate applications and are not replaced by this OS update.

## Base changes

Includes Melodee v0.5.1's sequencing/transport fixes, updated chopping and slicing,
sample-note metadata, kit-bank selection, auto sampling, USB-volume setting and
stem helper. The stem helper is taken from the checked supplied v0.5.1 archive;
its source matches the merged `tools/stems` source. Other existing runtime support
files match the supplied archive or are Tape Workshop's own updater components.

Rec Quantize follows the pattern's Q as in v0.5.1. This is separate from Tape
Workshop's split-cursor tick increments, which remain available. New tracks show
all six types (including Import Pad) in Basic and Full. New sample writes default
to Melodee/Samples; legacy Samples paths remain readable. Existing explicit
settings, project sample paths and Tape Workshop's custom-folder choice are kept.

## Validation

The ARM application builds. Native combined engine/model tests pass, including
both upstream and Tape Workshop test cases: project serialization, extended sample
paths, sample roots, MIDI, Roll splitting, expansion-pad import, bank import,
USB-volume routing and themes. Native direct-stream and stem-service tests pass.
Web feed/layout tests and plug-in-installer tests pass. Signed-update sandbox
checks pass using the existing Mac test adapters for Ed25519 and checksum tools.
The packaged manual's internal links/images and all 11 theme images are checked;
Studio routes/assets remain absent. Signed release verification checks the final
ZIP's manifest/signature, archive integrity and exact built executable.

Physical MPC playback and hardware controls have not been exercised for this
merged build. Save a backup of projects and the current autorun before installing.
The package is not automatically installed or publicly published.
