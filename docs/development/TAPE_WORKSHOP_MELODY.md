# Tape Workshop v135 — instrument melody players

Host based on Melodee by Kerem Kiliç (Ellic Studio).

Strings 0.4 and Orchestra 0.2 append 13 parameters without changing legacy indices.
Existing project chunks migrate with Melody Play off and Follow Project on.
The shared bounded scheduler in plugin/melody_pattern.h uses host PPQ/tempo;
owned generated voices have separate note-off handling from external MIDI.
No file I/O or allocation is added to the audio callback. This version generates
internal audio only, not Roll/MIDI events.

PluginRack copies project root/scale on load and sync when Follow Project is on.
Explicit local Key/Scale edits disable follow. The OS UI uses the same pattern
function as the plug-in. Browser previews are embedded PNGs, decoded off audio.

Shift+B3 on melody pages opens the existing sample browser in a read-only key
selection context. It reuses SampleLibrary's background key analyser, suppresses
auto-preview and pad loading, and applies only a matching ready selection after
B3 confirmation. Root-only results preserve scale. No new analyser or audio
engine is introduced.

Validation: deterministic variation, 13 scales/12 keys, swing/gate timing,
rest density, stop/seek release, buffer-size independence, generated/manual
voice isolation, panic, preset/state bounds and old chunk migration. Desktop
screens use real plug-in parameter metadata without invoking the render callback.
MPC audio, CPU limits and musical audition remain untested for this revision.
