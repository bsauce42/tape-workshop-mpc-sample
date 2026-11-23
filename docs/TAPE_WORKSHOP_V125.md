# Tape Workshop v125

Based on Melodee by Kerem Kiliç (Ellic Studio).

Tutorial mode keeps the live preview beside a compact control dock. Framing, Appearance and Recording tabs organize the controls. On narrow screens the preview stays above the controls. Independent MPC/screen framing, 3D screen angle and smooth changes remain available.

The browser MPC view shows A–H bank labels, highlights the current track bank, and displays stereo meters from incoming audio when MPC audio is enabled. No audio engine processing was changed.

Save recording and Final Cut files use Save As on browsers with File System Access support. Other browsers use their download settings. The Mac Final Cut Prep helper asks for the recording source folder and the output destination. It displays frame-based percentage progress and includes a Tape Workshop app icon.

Two local Final Cut crash reports identify the XML AssetClipImport path. The helper now writes explicit video/audio components instead of nested asset clips, with a single layered project and separate background. Export passes Apple FCPXML 1.7 DTD validation; actual Final Cut reimport and MPC hardware playback require a user test. Re-run the updated helper on the original recording folder to regenerate the XML. Previous prepared folders are preserved.
