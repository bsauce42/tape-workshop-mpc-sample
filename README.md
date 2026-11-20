# Tape Workshop for MPC Sample

Tape Workshop is an independent community build based on **Melodee v0.5.1 by Kerem Kiliç (Ellic Studio)** for the Akai MPC Sample. All **22 instruments and effects**, including Melody Forge 1.0, are included.

## Download v120 for the SD card

[Download Tape-Workshop-v120-SD-Card.zip](https://github.com/bsauce42/tape-workshop-mpc-sample/releases/download/v0.5.1-tapeworkshop-community.120/Tape-Workshop-v120-SD-Card.zip)

**This is the only download needed for SD installation.** About 138 MB, containing only:

- **autorun/** — the current Tape Workshop OS.
- **Melodee/Plugins/** — all 22 plug-ins, samples/resources, skins and required license notices.

No source archives, standalone guide ZIPs or development files are inside the SD download. [Release notes and other download methods](https://github.com/bsauce42/tape-workshop-mpc-sample/releases/tag/v0.5.1-tapeworkshop-community.120).

### Install

For MPC Sample firmware **1.3.0.12**:

1. Back up the SD card and matching plug-in folders; power off the MPC.
2. Extract the SD Card ZIP and replace the card-root **autorun** folder.
3. Merge the included **Melodee/Plugins** folders onto the card. Preserve projects, samples, user presets and other third-party plug-ins.
4. Eject the card, restart the MPC and reopen the companion at http://192.168.155.1/ after updating.

SD users do not need the separate **Signed-Update.zip** or source downloads. Signed Update remains for the existing browser updater/Manual Install; it changes the OS only and does not install plug-in folders.

## Live typed search

Open typed Sample Select search with **Shift + B1**. Results update from the first letter. Partial words and phrases work: **pia** finds Piano; **warm pia** finds Warm Grand Piano.

Names beginning with the first word you type come first. For **kick**, **Kick 01** and **Kick Deep** appear before **Kings Dog Kicks** and **Deep Kick**. Other matches remain below. **DONE** or **BACK** closes typing and keeps the filter; erasing all characters restores the normal list. Voice matching is unchanged.

## Other included features

- Melody Forge: four-layer synthesizer, 80 presets across Trap, HipHop, R&B, EDM and DnB, with a dedicated live UI and 21 control pages.
- Distinct plug-in faces, parameter pages, live stereo waveforms, spectra and meters; browser artwork for all 22 plug-ins; Data-knob presets and optional B2 preset menu.
- Second Spin idle-wake/Crackle fix and audio-driven cassette reels; Afterglow Piano hot-green theme; Room Grand 1.3.1 release-noise defaults.
- Roll MIDI import, selection, Shift + K3 zoom and fine splitting; expansion bank/pad import, cover art and eight-bank builder; Live Screen/Tutorial Mode, stereo recording, themes, startup lighting and signed updates.

Sub Foundry, Contour Comp and the enlarged Prism EQ retain their established layouts. Browser DAW/Studio remains removed. The separate Melodee plug-in UI installer is a different product.

## Verification and source

ARM64 build, typed/partial search, prefix ordering, late arrivals, result cap and selection-preservation checks passed. The SD ZIP was checked against every file/resource in the 22-plug-in inventory and the signed OS runtime. MPC testing of the new search remains pending.

[Matching v120 OS source](https://github.com/bsauce42/tape-workshop-mpc-sample/releases/download/v0.5.1-tapeworkshop-community.120/Melodee-v120-source.tar.gz), host commit **9f241cc2572f1e5e22d4c4724f4f352da8f9be26**. [Unchanged matching source for all 22 plug-ins](https://github.com/bsauce42/tape-workshop-mpc-sample/releases/download/v0.5.1-tapeworkshop-community.118/Tape-Workshop-v118-Plugins-Source.tar.gz). Source is separate and not needed on the SD card. This repository is the distribution hub; its branch and GitHub-generated Source code downloads are not the matching OS source.

SD ZIP SHA-256: 52362bcec60617bdf4a5b71b4e13a0fdf93eb29786f4405ba9091e0be0cd5e10

## Credits and licenses

- Tape Workshop additions and branding: **bsauce42**. Melodee and sample-base are by **Kerem Kiliç (Ellic Studio)**. [Melodee notices](https://github.com/keremimo/melodee-sample/blob/main/THIRD_PARTY.md)
- The Melodee fork and original Tape Workshop host code use AGPL-3.0-only; Melody Forge's engine retains its MIT license. Dex FM and Sloop FM6 retain GPL-3.0-only terms and Leo Kuroshita/Hügelton Instruments Sloop/Felucca and msfa attribution. [Sloop/Felucca source](https://github.com/isod89/sloop-fm1)
- The Open MPC wrapper retains MIT terms. Afterglow Piano and Room Grand adapt Alexander Holm's [Salamander Grand Piano](https://github.com/sfzinstruments/SalamanderGrandPiano) under CC BY 3.0. Velvet Choir uses Karoryfer Samples' [CC0 vocal tutorial recordings](https://github.com/sfzinstruments/legato_vocal_tutorial).

See the component notices and corresponding source for individual terms.
