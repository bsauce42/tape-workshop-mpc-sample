# Tape Workshop for MPC Sample

Tape Workshop is an independent community build for the Akai MPC Sample, based on **Melodee v0.5.1 by Kerem Kiliç (Ellic Studio)**. The complete download includes **21 instruments and effects**, their resources, licenses and matching source.

## Tape Workshop v116

[Download the v116 release](https://github.com/bsauce42/tape-workshop-mpc-sample/releases/tag/v0.5.1-tapeworkshop-community.116)

**Tape-Workshop-v116-Complete.zip** is the recommended download: full OS + all 21 plug-ins. The smaller **Signed-Update.zip** is OS-only for the browser updater and does not replace plug-in folders.

### Changes

- Second Spin Crackle now returns after zero when turned back up, without requiring a new note. Verified on the physical MPC running v116.
- Distinct plug-in panels and parameter pages, live stereo waveforms, measured spectra and output meters. Voice/source-mix graphics use live audio waveforms.
- Afterglow Piano hot-green theme; Second Spin cassette reels follow audio and stop in silence.
- Turn **Data** directly on an instrument plug-in UI to browse exposed presets and see the selected name while scrolling. **B2** still opens the full list. Effects use **Shift + Data** for exposed presets.
- Room Grand 1.3.1 softens release noise and defaults Release Noise to zero in the engine and all factory presets. Saved project values are retained.
- Read-only performance diagnostics expose existing load and audio-dropout counters.
- Roll MIDI import, selection, Shift + K3 zoom and fine splitting; expansion bank/pad import, artwork and all-eight-bank builder; Live Screen/Tutorial Mode, stereo recording and logo/background options; themes, startup lighting and signed updates are retained.

Sub Foundry, Contour Comp and the enlarged Prism EQ retain their established layouts. The browser DAW/Studio remains removed. The separate Melodee plug-in UI installer is a different product.

## Install

For MPC Sample firmware **1.3.0.12**:

1. Back up the SD card and matching plug-in folders; power off the MPC.
2. Extract **Complete.zip**. Replace the root-level `autorun` folder with the included `autorun`.
3. Merge the included `Melodee/Plugins` folders onto the card. Preserve projects, samples, user presets and other third-party plug-ins.
4. Restart the MPC. Connect over USB-C and reopen the companion at `http://192.168.155.1/` after updating.

The browser Manual Install uses **Signed-Update.zip**. To receive the updated Room Grand binary as well, copy its plug-in folder from **Complete.zip**. Source archives do not need to go on the SD card.

## Verification

ARM64 build, native engine/model/binding tests, repeated real-DSP idle-wake regression, signed update, ZIP integrity and the 21-bundle inventory passed. The Second Spin zero/back-up fix was tested on the physical MPC.

A short on-device v115 test with Afterglow Piano, Astral Loom, Room Grand and Layer House playing together, live visuals and browser audio had zero audio dropouts or overload voice cuts. Peak audio render-budget use was about 35%; the busiest CPU core peaked at 48%. This describes that workload, not every preset or a full 16-track maximum load.

## Matching source and guides

Release assets include `Melodee-v116-source.tar.gz` (OS commit `1b1c044c5050d4288f6781defcdf0ec69c7be749`, with pinned base runtime), `Tape-Workshop-v116-Plugins-Source.tar.gz`, guides, revision notes and SHA-256 files.

This repository is the distribution hub. Its branch and GitHub-generated source archives are not synchronized with the attached OS source. Use the attached source archives for this build.

## Credits and licenses

- Tape Workshop additions and branding: **bsauce42**. Melodee and sample-base are by **Kerem Kiliç (Ellic Studio)**. [Melodee notices](https://github.com/keremimo/melodee-sample/blob/main/THIRD_PARTY.md)
- The Melodee fork and original Tape Workshop code use AGPL-3.0-only; Dex FM and Sloop FM6 retain GPL-3.0-only terms and Leo Kuroshita/Hügelton Instruments Sloop/Felucca and msfa attribution. [Sloop/Felucca source](https://github.com/isod89/sloop-fm1)
- The Open MPC wrapper retains MIT terms. Afterglow Piano and Room Grand adapt Alexander Holm’s [Salamander Grand Piano](https://github.com/sfzinstruments/SalamanderGrandPiano) under CC BY 3.0. Velvet Choir uses Karoryfer Samples’ [CC0 vocal tutorial recordings](https://github.com/sfzinstruments/legato_vocal_tutorial).

See the included source, build scripts and component notices for individual terms.
