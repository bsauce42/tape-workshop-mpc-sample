# Tape Workshop v124

Based on Melodee by Kerem Kiliç (Ellic Studio).

Framing → Large screen now has a 3D screen angle from −35° to +35°, with Left back, Straight, and Right back shortcuts. It rotates only the large screen and its tablet frame in perspective. The MPC stays upright. Independent crop/pan, smooth transitions, file search and all 22 plug-ins are retained. Angle changes work during recording. The browser uses CSS perspective; exported video uses the same perspective projection with GPU texture mapping and a Canvas fallback. Straight view keeps the previous direct rendering path.

## Final Cut layers and editable backgrounds

Enable **Final Cut layers** before recording in Tutorial Mode. The normal combined take remains available. **Save Final Cut files** contains two synchronized MP4 source angles (full MPC and clean screen), grayscale alpha masks, and a separate background PNG. Browser-only MP4 cannot retain these transparent edges by itself.

Download **Mac Final Cut Prep** from the page, extract both ZIPs, and open the helper on macOS 13 or later. Choose the recording folder. It writes transparent HEVC MOV layers and `Tutorial.fcpxml` into a new **Final Cut Ready** folder, leaving originals untouched. Import the XML into Final Cut Pro. It provides separate-layer and multicam projects on the same 30 fps timeline. Replace the **Background** clip with a color generator, image or movie; adjust framing and cut video angles afterward. No green-screen color removal is used, so green LEDs and pad/screen colors are preserved. Framing in source-angle exports stays full and upright for later editing; the combined take retains live framing and perspective.

Extra capture is optional, runs in the browser, and shares the existing stereo audio and screen feeds. The 20-minute/768 MiB limit counts all recordings. Device style is fixed during a layered take so its mask stays aligned. Browser export requires MP4 MediaRecorder support. The Mac helper is universal for Apple Silicon and Intel; its source and AGPL license are included in its ZIP.

Release builds on macOS run `python3 tools/build_final_cut_prep.py` before generating the web bundle. The generated helper ZIP is embedded in the web app; the standalone helper stays outside the SD-card folder layout.

Validation: ARM OS build; perspective/crop/transition math; GPU export rendering; continuous local 1080p recording with angle changes; actual layered browser ZIP extraction and CRCs; native conversion of both 1080p test videos; transparent edge and opaque center pixel checks; Apple FCPXML 1.7 DTD validation. Final Cut Pro is not installed on the test Mac, so actual Final Cut import and on-device MPC recording still require testing. Synthetic local stereo audio was used, so hardware timing accuracy has not been measured.
