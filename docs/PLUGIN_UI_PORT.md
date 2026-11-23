# Plug-in UI integration

Melodee by Kerem Kilic (Ellic Studio) is the origin.

`tools/plugin_ui/` contains a source patch for the exact official v0.5.1 commit,
adding graphical parameter controls for instruments and imported effects.
Tape Workshop retains its specialized screens and uses this portable editor for
Frodhes on EDIT. Other Tape Workshop instruments retain their existing views.
The implementation draws existing parameter snapshots and does not alter sound
processing, ABI loading or parameter routing. UI Edition 2 adds a fixed-size,
lock-free stereo output tap to the plain Melodee fork. Only the selected plug-in
publishes; the UI polls at its existing 20 Hz cadence. Scope tests cover stereo,
audio preservation, silence, rolling capture, nonfinite samples and concurrent
reads. Hardware validation of live capture is pending.
