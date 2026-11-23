# Tape Workshop Strings — local v133 build

Based on Melodee by Kerem Kiliç (Ellic Studio). Tape Workshop additions by bsauce42.

This build adds the ARM64 VST2 Tape Workshop Strings 0.3 instrument and its host-rendered violin screen.
It retains the 22 existing plug-in bundles. The installation ZIP contains only autorun and Melodee/Plugins.
Bowed notes loop while held, with adjustable release. Pizzicato decays naturally. Four string families,
24 factory presets, synth-style controls, movement modulation, reverb and separate room echo are included.

The screen uses current host parameter snapshots and measured output waveforms/meters. Bow and string
motion is a level-gated illustration; it is not a measurement of physical bow position or individual strings.
Parameter pages display section mix, stereo ensemble, envelope, filter, movement and room diagrams.
The waveform in the supplied desktop preview uses illustrative display data; it is not an MPC capture.

Copy the included autorun to the SD root and merge Melodee/Plugins, retaining projects/samples.
The signed updater ZIP contains only the OS; install the Strings plug-in folder separately if using it.
The standalone Strings ZIP alone cannot update the host-drawn violin screen.

No MPC installation/audio test was performed for this local build. The earlier desktop DSP checks predate
room echo and the additional presets. Latest changes are compile-checked and previewed; embedded CPU,
loop quality and live display behavior still require hardware verification. Do not describe these as measured.

The separate unfinished Tape Workshop Synth/chord-generation work is not part of this package.
Sample source/licensing and rebuild instructions are in plugins/tape-workshop-violin/README.md.

The Players overview now shows all four families on an original dark stage. Group tabs are
page indicators, not new physical-button assignments. The appended Solo Legato page offers
last-held-note-priority mono connection, 0–1000 ms linear pitch glide and a 0–24 semitone
maximum glide interval. Normal polyphonic chord mode remains the default. These are pitch
transitions through the existing samples, not separately recorded true-legato transitions.
Existing presets keep their numbering; two legato presets are appended. State chunks from
0.1 and 0.2 restore with legato off. No MPC audio or installation test was performed.
