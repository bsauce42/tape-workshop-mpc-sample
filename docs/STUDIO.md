# Tape Workshop Studio (v97)

Studio is a browser control surface for the project currently running on the MPC. Connect by USB/network and open `/studio` on the MPC's companion website. It has no browser synthesizer or duplicate project file. The MPC's engines, loaded plug-ins, audio tracks and MIDI routing remain responsible for sound. A MIDI-out track still needs its normal connected instrument. Changes are sent over the local connection and synchronized into MPC playback; network delay and sequencer timing apply.

## MIDI Grid

Select a track and pattern, and set the pattern length in bars. Draw adds notes on the selected snap grid. Select lets you drag notes, resize their right edges, or Shift-click a group. Erase deletes a note. Velocity stems can be dragged vertically. The inspector edits start tick, duration, key and velocity precisely; Apply sends the edit to the MPC. Select visible selects up to 128 visible notes, Delete removes them, and Duplicate places a copy after the selected span. Changes preserve per-note chance, ratchets, conditions and pitch data not explicitly edited. Undo/Redo use the MPC's shared edit history.

Preview on MPC or tap a piano/pad-row label to audition that note on the unit. The note is released automatically after about 220 ms even if the browser disconnects.

The view can zoom from one to sixteen bars with Previous/Next pages, plus octave navigation. Drum banks A–H expose 128 pad rows. A view returns at most 2,048 notes; zoom in if it is too dense. Audio-track waveforms and audio sample slicing are not edited in this MIDI view.

## Sequencer and Song Arrange

Sequencer shows all 16 tracks against a bank of 16 patterns, with eight pattern banks. Select a cell for editing; Launch pattern queues it on the MPC. Play selected sequence launches that pattern number across tracks using the MPC's native scene behavior.

Song Arrange uses the existing MPC song clips and track lanes, not a separate browser song. Place pattern puts the selected pattern at the chosen bar for the chosen duration. Select a clip to move, resize, duplicate, mute or delete it. Touch users can use the same clip fields for precise updates. Overlapping clips are rejected instead of cutting or replacing the other clip. Clips reference patterns: editing a pattern changes all its instances. Existing audio-track patterns can be arranged, but browser audio-file editing is outside this version.

## Playback, saving and export

Play and Stop operate the MPC transport. Song Arrange Play starts the arrangement at First bar; the other views play patterns. Stop before changing transport mode. Record must be disarmed before browser edits. Mute and Solo control MPC tracks. New drum track creates an empty drum track; configure other instrument types, plug-ins and audio inputs on the MPC.

Save on MPC uses the native save operation. If the project has no filename, finish the naming dialog on the unit. Export on MPC opens the native Song export screen; choose the format/name and finish on the unit. It does not render audio in the browser or silently download a mix. Files remain on the MPC SD card and can be downloaded later with Files.

## Desktop, phone and fullscreen

Desktop has a track sidebar, large roll and inspector. On mobile, tap Edit controls to reveal track, pattern and editing tools; the sidebar is replaced by the Track selector; timelines scroll inside the editor while controls wrap to the phone width. Tap Fullscreen for native fullscreen when supported, or an in-page focus layout otherwise. Exit fullscreen returns to the normal companion website. Audio is still played by the MPC.

## Concurrency and performance

The HTTP thread queues bounded requests; project operations run on the existing MIDI/control thread under the normal Shell lock, never in the audio callback. Notes and clips are staged and validated before committing. A changed project, changed notes, changed arrangement, frozen note target, recording, or blocking MPC dialog can reject an edit. Conflicts require a refreshed view and an explicit retry. Do not repeat a timed-out command blindly; check the project first.

Studio refreshes at roughly 2–3 snapshots per second, only while visible and idle, with one outstanding command per browser. It pauses refresh during gestures and field editing. It is intended for editing, not sample-accurate performance over a network. Existing DSP/voice limits and overload protection are unchanged. Multiple independent browser editors can overwrite shared transport/mute choices; note/clip revision checks protect structural edits.

Built on Melodee by Kerem Kiliç (Ellic Studio). This is an independent Tape Workshop interface inspired by the supplied workflow references; no Akai software or graphics are embedded.
