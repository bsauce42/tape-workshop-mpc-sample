# Tutorial Mode and Expansion Builder

Tape Workshop’s local USB companion builds on Melodee by Kerem Kiliç (Ellic Studio). The browser DAW is removed from v103; edit MIDI and arrange songs on the MPC.

## Companion address

Use http://192.168.155.1/ while the MPC is connected by USB. The unreliable named-address link has been removed from the companion instructions. Add this numeric address to your phone’s home screen.

## Tutorial Mode

Choose Realistic or Pixel Art and a background color. Import your own logo (PNG, JPEG or WebP, up to 8 MB) to put it at the top left. The logo and background are included in the composed tutorial video. The logo is remembered only in this browser; Remove logo clears it. It is rasterized locally and is not sent to a third-party service.

Product photo uses the owner’s supplied MPC photo, retouched to remove glare, background and branding. It now uses a single map in the image’s native 1199×1312 coordinates for the LCD, pads, knobs and button highlights. Normal view, fullscreen and composed recordings use the same geometry. The live display fits inside the photographed LCD without stretching. Previous freehand photo-offset settings are ignored, and arbitrary device-photo imports/offset controls have been removed so mismatched geometry cannot be saved. Logo import and expansion cover photos remain available. Realistic illustration and Pixel Art are still options.

Tap pads/buttons and drag knobs or scroll over them to control the connected MPC. Physical actions appear in the illustration. Remote taps release automatically if the browser disconnects. Use physical Shift for held combinations. The MPC owns the native project; playback and export remain on the MPC.

## Phone home screen

The Device page has Add to Home Screen instructions and an app-icon download. On iPhone, open the companion in Safari, choose Share → Add to Home Screen → Add. Safari reads the supplied Apple touch icon. The website cannot add an icon to your phone without your action. Where the browser offers an install prompt, the button uses it; otherwise it explains the browser-menu steps. A local HTTP companion may not qualify for an install prompt. The shortcut still needs the MPC connection; it is not an offline firmware emulator.

## Expansion cover photos

In Expansion Builder, name the pack, add WAV samples, and optionally choose Import cover photo. The photo is center-cropped to a 768×768 JPEG, previewed, and included as Artwork.jpg in the .xpn. Expansion.xml references it, and TapeWorkshopExpansion.json records the cover path, dimensions, author and title for a future OS gallery. This release does not add that OS gallery. Remove photo omits the artwork and its metadata from the export.

The .xpn includes Expansion.xml, Kit.xpm and its samples. Upload it through Files or copy it to SD, open it in Sample Select, then load Kit.xpm. Samples are converted to 44.1 kHz, 16-bit mono/stereo. The builder offers banks A–H, 128 pad slots total, with a 256 MB archive cap. Sample-loading limits can stop an export earlier. No plugin binaries are bundled.

## Tutorial recording duration

Live Screen and Tutorial Mode support **up to 2 hours per take** when browser
disk buffering is available, with a separate limit of just under **4 GiB**.
The page reports the active limit. The fallback without disk buffering is
**20 minutes or 768 MiB**, whichever comes first. Browser codec, selected
quality and free storage can shorten a take; a two-hour recording is not a
guarantee on every device. Temporary recording storage is on the computer or
phone running the browser, not on the MPC SD card.

Use **Pause recording** to omit a section; **Resume** continues the same video
and excludes paused time. The separate live-view pause freezes the display.
Stop the recording, then **Save recording**. A destination picker is used where
the browser supports it; other browsers use their download-location settings.
Keep the page visible, the device unlocked and USB connected. Save a finished
take before starting another.

## Framing and backgrounds

The inspector groups independent MPC and large-screen zoom/pan controls.
**Full unit, Knobs, Pads**, view switches and screen-angle controls are alongside
the preview so you can adjust framing while watching it. Tablet is a frame
option; smooth screen rendering softens enlarged source pixels, but the native
MPC display remains 320×240, rather than gaining new detail.

Choose a background color, an original animated scene, or **Upload background**
for a local PNG/JPEG/WebP image. Audio-reactive scenes and the live waveform
need **Enable MPC audio**. Logo/background imports are stored in this browser;
recording includes the composed scene. Final Cut/Logic project export is removed.

## Browser style

The companion now uses solid black with purple, magenta and cyan accents from
the Tape Workshop logo, bright text and dark sample pads. The OS Workshop theme
setting is independent of this browser style. After an update, reload the page
to get the new styling; an already installed home-screen icon may need removing
and adding again to refresh the icon.

## Expansion Builder

Use banks A–H to assign up to 128 pad slots. Click/tap to audition, or enable MPC
pad triggering to audition the bank selected in the browser. **Save to MPC** writes
the pack to **Expansion Packs** on the SD card; **Download a copy** keeps another
copy on the phone/computer. See [Expansions, banks and pads](workshop-expansions.md)
for the complete workflow and the OS import steps.

## Live Screen and stereo audio

Live Screen shows the MPC display; Tutorial Mode composes a large display with
the illustrated/photo device and control activity. Use **Enable MPC audio** to
monitor the unit's direct stereo stream. **Record Tutorial**, stop, then **Save
recording** keeps the composed take. The browser asks for recording-related
permissions it needs; direct MPC audio does not require recording your phone's
microphone. Supported output formats depend on the browser.

Keep the page visible, the device unlocked and the USB cable connected. Listen
with headphones on the MPC for the most immediate playing feedback. Browser
monitoring includes network and output-device buffering. The firmware's direct
recording stream and the speaker/headphone output are separate paths; test both
stereo channels in a short recording before a long tutorial.

The configured limits are safeguards, not a claim that every phone/browser
combination has completed a full two-hour take. Record a short test first.
