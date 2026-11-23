# Expansions, banks and pads

Tape Workshop extends Melodee by Kerem Kiliç (Ellic Studio).

## Build a pack in the browser

Open **http://192.168.155.1/** over USB and select **Expansion Builder**.

1. Name the pack and enter its creator.
2. Choose **bank A–H**, then a pad. Add WAV samples; multiple files fill successive
   slots across banks. There are **16 pads per bank, 128 slots total**.
3. Tap/click an assigned pad to hear it in the browser. To use the MPC pads for
   browser audition, enable the MPC-pad control offered by the builder. Physical
   pads address the bank selected in the browser. Browser audition is not a
   recording into the MPC project.
4. Optionally choose **Import cover photo**. The image becomes a square 768×768
   JPEG in the pack. Remove it to export without cover art.
5. **Save to MPC** writes the export to the SD card's **Expansion Packs** folder.
   **Download a copy** saves another copy to your phone or computer.

The exported `.xpn` contains `Expansion.xml`, `Kit.xpm`, WAV samples and optional
`Artwork.jpg`. Samples are converted to 44.1 kHz, 16-bit mono/stereo. The builder
caps the archive at 256 MB; its sample-loading limits can stop a pack earlier.
A pack contains samples/program data, not desktop plug-in binaries.

## Open a pack on the MPC

Hold the encoder to open the Menu, choose **HERE → EXPANSIONS**, and browse the
pack. You can also open an `.xpn` in Sample Select. Installing an expansion places
its contents in **Samples/Expansions**. Open the installed folder and choose its
kit/program or bank. Recognized cover art is shown in the OS expansion browser.

The export folder **Expansion Packs** stores the pack files you create; the
installed-content folder **Samples/Expansions** is where unpacked kits/samples
are browsed. They serve different steps of the workflow.

## Import a bank or a single pad

Use the expansion browser's **Import Bank** or **Import Pad** action for the item
you want to load. The source-bank and source-pad controls choose material from
within the kit. Check the destination bank/pad shown on screen before loading.
Keep a saved project copy before replacing sounds you want to retain.

On an empty track's type chooser, **Import Pad** sits beside Drum, Keygroup,
Plugin, MIDI and Audio. Choose it to browse expansion material. Its **Empty Pad**
choice creates a blank pad without loading a sample; load a sample later when
ready. The Import Pad track type is available in both Basic and Full.

## MPC desktop packs

Supported sample-based MPC kits/programs can be imported when the required WAV
samples and supported program data are included. An MPC desktop instrument that
requires a desktop plug-in is not made portable by packing its MIDI. Render that
instrument to audio first if the MPC cannot run it. Check the imported pad map,
samples and playback on the MPC before relying on a pack in a performance.
