# Tape Workshop for MPC Sample

Tape Workshop is an independent community build for the Akai MPC Sample, based on **Melodee v0.5.1 by Kerem Kiliç (Ellic Studio)**. The complete download includes **22 instruments and effects**, their resources, licenses and matching source.

## Tape Workshop v118

[Download the v118 release](https://github.com/bsauce42/tape-workshop-mpc-sample/releases/tag/v0.5.1-tapeworkshop-community.118)

**Tape-Workshop-v118-Complete.zip** is the recommended download: full OS + all 22 plug-ins, including **Melody Forge 1.0**. The smaller **Signed-Update.zip** is OS-only for the browser updater and does not install plug-in folders.

### New in v118

- Melody Forge: four-layer synthesizer with 80 factory presets across Trap, HipHop, R&B, EDM and DnB; 63 controls on 21 three-knob pages.
- Dedicated Melody Forge face with layer settings, a real output waveform, parameter-driven filter/envelope plots, Data-knob preset names and host tempo sync. Keyboard and held-note Melody modes are available. Acoustic-style sounds are synthesized approximations.
- The on-device instrument/effect browser shows a distinct UI preview for each of the 22 bundled plug-ins. Data browses; push Data to load; B1 goes back. Other plug-ins without bundled artwork show a type icon. Browsing does not load the plug-in or run its DSP.
- The current v0.5.1-based host is retained. The older v46 autorun from Melody Forge's standalone ZIP is excluded.

### Retained features

- Second Spin Crackle returns after zero when turned back up; the v116 fix was verified on the physical MPC.
- Distinct plug-in faces and parameter pages, live stereo waveforms, measured spectra and output meters; Afterglow Piano hot-green theme and audio-driven Second Spin cassette reels.
- Data directly browses exposed instrument presets; B2 opens the optional list. Effects use Shift + Data.
- Room Grand 1.3.1 softens release noise and defaults Release Noise to zero; saved project values are retained.
- Roll MIDI import, selection, Shift + K3 zoom and fine splitting; expansion bank/pad import, artwork and all-eight-bank builder; Live Screen/Tutorial Mode, stereo recording and logo/background options; themes, startup lighting, diagnostics and signed updates.

Sub Foundry, Contour Comp and the enlarged Prism EQ retain their established layouts. Browser DAW/Studio remains removed. The separate Melodee plug-in UI installer is a different product.

## Install

For MPC Sample firmware **1.3.0.12**:

1. Back up the SD card and matching plug-in folders; power off the MPC.
2. Extract **Complete.zip** and replace the root-level autorun folder with the included autorun.
3. Merge the included Melodee/Plugins folders onto the card. Preserve projects, samples, user presets and other third-party plug-ins.
4. Restart the MPC. Connect over USB-C and reopen the companion at http://192.168.155.1/ after updating.

Browser Manual Install uses **Signed-Update.zip**. That package updates the host UI but does not install Melody Forge: copy its complete plug-in folder from **Complete.zip** as well. Source archives are for your computer and do not need to go on the SD card.

## Verification

ARM64 host build, 22-bundle packaging checks and signed update verification passed. Melody Forge engine tests and all 80 VST presets passed finite/audible-output checks; an ARM64 source rebuild exactly matched the supplied library. All 21 Melody Forge UI pages and 22 browser artwork selections rendered successfully in host previews. Physical MPC playback, controls and CPU load for this new integration remain to be verified.

A short on-device v115 test with four earlier instruments had zero audio dropouts or overload voice cuts, about 35% peak render-budget use and 48% peak busiest-core use. This describes that workload, not Melody Forge or a full 16-track maximum load.

## Matching source and guides

Release assets include **Melodee-v118-source.tar.gz** (OS commit **65b65d785cca90688eaa6718638f5a080710a8b4**, with pinned base runtime), **Tape-Workshop-v118-Plugins-Source.tar.gz**, guides and SHA-256 files. This repository is the distribution hub. Its branch and GitHub-generated source archives are not synchronized with the attached OS source. Use the attached archives for matching source.

## Credits and licenses

- Tape Workshop additions and branding: **bsauce42**. Melodee and sample-base are by **Kerem Kiliç (Ellic Studio)**. [Melodee notices](https://github.com/keremimo/melodee-sample/blob/main/THIRD_PARTY.md)
- The Melodee fork and original Tape Workshop host code use AGPL-3.0-only; Melody Forge's engine retains its MIT license. Dex FM and Sloop FM6 retain GPL-3.0-only terms and Leo Kuroshita/Hügelton Instruments Sloop/Felucca and msfa attribution. [Sloop/Felucca source](https://github.com/isod89/sloop-fm1)
- The Open MPC wrapper retains MIT terms. Afterglow Piano and Room Grand adapt Alexander Holm's [Salamander Grand Piano](https://github.com/sfzinstruments/SalamanderGrandPiano) under CC BY 3.0. Velvet Choir uses Karoryfer Samples' [CC0 vocal tutorial recordings](https://github.com/sfzinstruments/legato_vocal_tutorial).

See the included source, build scripts and component notices for individual terms.
