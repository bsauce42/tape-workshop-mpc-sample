# Installing plug-ins on another Melodee build

Tape Workshop is based on Melodee by Kerem Kiliç (Ellic Studio).

The separate SD-card installer copies compatible Linux ARM plug-in files to
`Melodee/Plugins`. It never changes the card's autorun folder or operating system,
and refuses to replace an existing plug-in. It preserves resources inside complete
VST3, LV2 and Open MPC bundles. Use the plug-in author's complete distribution,
including presets and samples; a loose library alone may not contain its resources.

On the Mac, connect the SD card and check first:

```
python3 install_plugins.py --card /Volumes/YOUR_CARD --dry-run '/path/to/Your Plugin.clap'
```

Remove `--dry-run` to copy. Multiple plug-ins can be supplied in the same command.
Eject the card before using it in the MPC, then rescan plug-ins in Melodee.
This utility requires Python 3 and a mounted writable SD card. It does not include
third-party plug-ins. Compatible Melodee builds also have a plug-in upload control
on the browser's Device page.

A Linux ARM file is necessary but does not guarantee the target host supports its
format, required libraries, bridge or features. Mac AU/VST files and Windows DLLs
cannot be converted by copying. For 32-bit ARM plug-ins, the target build must ship
the bridge. Keep the host's normal validation and loading safeguards enabled.

Tape Workshop's Prism EQ and other custom animated screens are drawn by the host,
not installed as plug-in editor windows. Another Melodee fork needs the corresponding
screen, parameter and telemetry integration compiled into its host. Installing the
plug-in alone cannot add that interface. This installer does not patch an unknown
OS or replace its executable.
