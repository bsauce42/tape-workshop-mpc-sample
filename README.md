# Tape Workshop for MPC Sample

Tape Workshop is an independent community build based on **Melodee 0.7 by Kerem Kiliç (Ellic Studio)** for the Akai MPC Sample. This OS update retains support for the existing **28 Tape Workshop instruments and effects**; keep your installed plug-in folders.

## Download v196 — signed OS update

[Download Tape-Workshop-v196-Signed-Update.zip](https://github.com/bsauce42/tape-workshop-mpc-sample/releases/download/v0.7.0-tapeworkshop-community.196/Tape-Workshop-v196-Signed-Update.zip) · [Release notes and supporting files](https://github.com/bsauce42/tape-workshop-mpc-sample/releases/tag/v0.7.0-tapeworkshop-community.196)

**This release is the OS-only signed update. Keep your existing Melodee folder, plug-ins, projects, samples and presets.** Plug-in binaries are not included or replaced.

### Install

Supported stock firmware: **1.3.0.12 and 1.4.0.7**. Firmware 1.4 requires **Developer Mode** and the separate [autorun.sh launcher](https://github.com/bsauce42/tape-workshop-mpc-sample/releases/download/v0.7.0-tapeworkshop-community.196/autorun.sh) copied beside the autorun folder at the SD-card root.

1. Save your project and back up the SD card.
2. Connect the MPC over USB and open the Tape Workshop browser companion at http://192.168.155.1/.
3. In Manual Install / update, select **Tape-Workshop-v196-Signed-Update.zip**.
4. Physically press **B3 / INSTALL** when prompted on the MPC and let it restart.
5. Refresh the browser companion.

For manual SD installation, power off the MPC, extract the signed ZIP and replace the card-root **autorun/** folder. Preserve **Melodee/**. On firmware 1.4, also copy the root **autorun.sh** launcher. The browser updater does not install that root launcher. The launcher alone does not contain the OS. Sources and guides are separate computer downloads, not SD installation files.

## Optional plug-in library — all 28

[Download Tape-Workshop-v196-Plugins.zip](https://github.com/bsauce42/tape-workshop-mpc-sample/releases/download/v0.7.0-tapeworkshop-community.196/Tape-Workshop-v196-Plugins.zip)

This separate ZIP includes all 28 plug-ins with their samples/resources, artwork, Bloom IRs and license notices. Power off the MPC and back up matching folders, extract the ZIP, then merge **Melodee/Plugins/** into the SD-card library. Preserve projects, samples, user presets and unrelated plug-ins. Eject safely and restart. Use the v196 OS for the matching plug-in screens. Do not select this ZIP in the browser OS updater.

Plug-in ZIP SHA-256: 2d39283a6788667bb1faae6068ad0ec81c23e0aff3159010bacf0896345f5d70

## Main feature: WAV → MIDI Generator

Generate editable **Chords, Bass Line, Pluck, Melody or Hybrid** directly on the MPC. Choose a WAV sample, existing MIDI Roll, or the project key as the source. Hybrid combines chords and melody.

1. Load a source WAV on a sampler pad and set the correct project BPM.
2. Select a melodic instrument track. Open **Roll → Data Push → MIDI GENERATOR**.
3. Follow **Source → Output → Target → Preview → Add**. For WAV input, select the actual source track and bank/pad; these can differ from the destination.
4. Confirm the WAV pad to start background key estimation. Scale can show **Auto / D minor**; the project key stays unchanged.
5. Choose the output, genre, scale and rhythm. Generate and preview, then **Add to Roll**. If notes already exist, choose **Cancel / Keep + Add / Replace**. Save your project.

**Data/K2** changes values, **Push/B2 Next** confirms, **B1 Back** returns, **K1** selects a setting and **B3 Options** opens advanced controls. **Generate Again** remembers the setup; **Generate From Roll** uses existing MIDI.

For simple chords enable **Triads**, then disable **Sevenths/Ninths**. Whole Notes changes rhythm, not chord size. Bass Line defaults to sustained classic 808 roots. Saved generator setups retain their settings. WAV harmony analysis is an estimate, not exact transcription of every audible note.

[Illustrated MIDI Generator instructions](docs/guide/wav-to-midi.md) · [Drum Generator instructions](docs/guide/drum-generator.md) · [Offline Quick Start and Manual](https://github.com/bsauce42/tape-workshop-mpc-sample/releases/download/v0.7.0-tapeworkshop-community.196/Tape-Workshop-v196-Guides.zip)

![Automatic WAV key preview](docs/images/generators/midi-scale.png)

## Included features

- Melodee 0.7 base, with the Tape Workshop generators, themes and browser tools retained.
- Drum Generator with genre/groove controls and guided kick, snare and hi-hat sound selection.
- Fullscreen Live Screen for a larger display, separate from Tutorial recording.
- Tutorial recording with Both, MPC only and Screen only views, independent framing, smooth transitions, backgrounds, logos and direct stereo audio.
- Readable dark plum browser file and plug-in rows, expansion artwork, bank/pad import, MIDI import and sample search.
- Typed Sample Select search: **Shift + B1**. Partial words and phrases work; DONE/BACK keeps the filter and clearing text restores the list.
- Plug-in faces, parameter pages, stereo waveforms, spectra, meters and browser artwork. Data browses exposed instrument presets; B2 retains the preset menu.
- Melody Forge, Strings, Orchestra, Guitar, Electric, Bloom Reverb with IR resources, and Prism FM Light in the existing 28-plug-in library (installed separately).
- Existing Second Spin idle-wake fix, Room Grand release-noise defaults, Sub Foundry, Contour Comp and enlarged Prism EQ layouts.

Manual Song Mode and native Chop remain. Automatic Song/Chop Generators, browser DAW/Studio, external browser DSP/Mac VST hosting and Final Cut/Logic export remain removed. The separate Melodee plug-in UI installer is a different product.

## Verification and source

ARM64 build, native sequencing/core tests, MIDI generator regressions, complete plug-in packaging and signed update verification passed. **Physical MPC playback and firmware-1.4 boot still require on-device verification.** Linux-only integration checks were skipped on macOS.

Signed updater SHA-256: f7abe3d2eb9d4c8d61089d99ed6cf892e8a82e0a23b9f71d8b7b073e83cbc31e

Use the release assets **Melodee-v196-source.tar.gz** and **Tape-Workshop-v196-Plugins-Source.tar.gz** for corresponding source, rather than GitHub’s automatic archive of this distribution/documentation repository. The OS source commit is **3f30783**; the source archive includes the populated base and custom core. Required component licenses and resource notices are included.

Tape Workshop additions and branding: **bsauce42**. Based on **Melodee by Kerem Kiliç (Ellic Studio)**. Original copyright and license notices are retained.
