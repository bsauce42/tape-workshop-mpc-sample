# Graphical controls for Melodee v0.5.1

Based on Melodee by Kerem Kilic (Ellic Studio), commit f11466589c7d.

This patch adds a host-rendered graphical parameter editor to the instrument
EDIT page and imported plug-in effect chains. It reads the existing PluginPanel
parameter names, text, values and page count. K1-K3 and page navigation use the
existing bindings and pickup; no plug-in ABI or DSP changes. A bounded lock-free audio tap now captures
actual stereo output for the selected plug-in only; no allocation, locks, file
I/O or drawing occur on the audio thread. The UI shows packed min/max waveforms
and peak dB readings. Idle/stopped/bypassed output clears instead of freezing.
Shift retains the original list / chain view, including menus and error states.

Apply with `git apply tools/plugin_ui/melodee-v0.5.1-controls.patch` in an
unmodified v0.5.1 source checkout (copy the patch there first), then build with
that checkout's supported toolchain. Other Melodee versions require adapting
and recompiling the host; copying a .so or editing metadata cannot patch its UI.

This does not port RHYTHMER Sound Rack's designs or vendor desktop editors.
Plug-ins exposing no parameters cannot acquire controls by this patch. The
layout preview uses fixture parameters; actual Frodhes hardware remains untested.
The separately supplied SD build is a fork with Tape Workshop's public signing
identity, and must be installed manually rather than through official updates.
