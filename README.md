# Tape Workshop for MPC Sample
Sixteen instruments and effects for the Akai MPC Sample, bundled with a
community build of Melodee v0.2.2 by Kerem Kiliç (Ellic Studio). The build
includes Tape Workshop's cassette boot artwork and custom plug-in screens.

## Community release 5

Dex FM has a new screen with the preset name, live output waveform, peak
meter and labeled K1–K3 values. Envelope and LFO pages show response
diagrams. Operator shape pages show signed Peak, Body and Sustain offsets.
Turn the Data knob from Keys or the controls screen to browse presets. Use
− / + on the controls screen to change between its 28 pages.

Sloop FM6, Stillwater and Velvet Choir now show the preset name, live output
and a peak meter, with labeled K1–K3 values. Use the Data knob from Keys or
the plug-in controls to change presets. Use − / + on the controls screen to
change pages. FM envelope and LFO pages, Stillwater envelope and drift,
and Velvet Choir's section blend have parameter-driven diagrams.

For Layer House, turn the **Data knob** on the Keys or plug-in controls screen
to browse presets. The preset loads after you stop turning. Push the knob on
Keys to open the controls screen, where the selected preset appears below the
plug-in name. Use **− / +** to change control pages.

Second Spin 2 adds animated reels, a live VU needle, and seven labeled
K1–K3 pages. Choose 7.5, 15, or 30 IPS and four tape stocks. Try **15 IPS Mix
Glue** for a first listen. The Cassette path restores the original sound and
old project settings. The updated host in the install ZIP drives the new screen.

## Downloads

- **Tape-Workshop-Hub-MPC-Sample.zip** contains the SD install, all 16 ARM64 plug-ins, skins and piano sample banks.
- **Tape-Workshop-Hub-Source.zip** contains the corresponding host and plug-in source, dependencies, assets, build scripts and license notices.
- **SHA256SUMS.txt** contains checksums for both downloads.

## Instruments and effects

| Instrument | Sound |
| --- | --- |
| Afterglow Piano | Sampled concert grand and layered tones |
| Astral Loom | Four-layer cinematic synth and organic textures |
| Dex FM | Six-operator FM, electric pianos, bells and plucks |
| Layer House | Four-layer wavetable synthesizer |
| Room Grand | Grand pianos and keyboard tones |
| Sloop FM6 | Six-operator FM with ambient pads |
| Stillwater | Evolving ambient pads |
| Sub Foundry | 808s, synth bass and sub layers |
| Velvet Choir | Vowel morphing and full choir mode |

| Effect | Controls |
| --- | --- |
| Break 26 | Sampler-style grit and drums |
| Contour Comp | Compressor with a live transfer curve and history |
| Gate Keeper | Gate and expander with a live response curve |
| Prism EQ | Color-coded EQ with live spectrum |
| Rack 76 | FET-style compressor with a needle meter and history |
| Second Spin 2 | Reel-to-reel saturation, live VU and classic cassette mode |
| Vocal Tune | Pitch correction with live pitch feedback |

## Install

1. Use an Akai MPC Sample running firmware **1.3.0.12**. Back up the microSD card.
2. Unzip the SD install. Move any existing root-level `autorun` folder to your backup, then copy the included `autorun` to the top level of the card.
3. Merge the included `Melodee/Plugins` into the card's `Melodee/Plugins` folder. Replace older copies of the same plug-ins.
4. Restart the MPC Sample. Open the track sound picker for instruments or an FX slot's plug-in picker for effects.

The included host provides the custom screens for the bundle. This build uses manual SD installation and omits the optional 32-bit bridge runtime, GUI stubs and supplemental Linux libraries. Removing the installed `autorun` from the card lets the stock app start at the next boot.

The binaries and archives were checked on the development machine. Booting, audio playback and hardware controls still need validation on an MPC Sample.

## Credits and licenses

- Melodee and sample-base are by Kerem Kiliç (Ellic Studio). This release is an independent community build. [Melodee source and notices](https://github.com/keremimo/melodee-sample/blob/main/THIRD_PARTY.md)
- The Melodee fork and original Tape Workshop code in this release use AGPL-3.0-only, with Dex FM and Sloop FM6 under GPL-3.0-only. The latter use Leo Kuroshita and Hügelton Instruments' Sloop/Felucca FM6 port of the Apache-licensed msfa core. [Sloop/Felucca source](https://github.com/isod89/sloop-fm1)
- The Open MPC wrapper retains its MIT terms. Afterglow Piano and Room Grand adapt Alexander Holm's [Salamander Grand Piano](https://github.com/sfzinstruments/SalamanderGrandPiano) under CC BY 3.0. Velvet Choir uses Karoryfer Samples' [CC0 vocal tutorial recordings](https://github.com/sfzinstruments/legato_vocal_tutorial).

See `BUILD.md` inside the source archive for build instructions and the included notices for individual terms.
