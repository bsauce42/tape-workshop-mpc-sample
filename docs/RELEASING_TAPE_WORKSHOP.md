# Tape Workshop GitHub releases

Tape Workshop by bsauce42, based on Melodee by Kerem Kiliç (Ellic Studio).

The owner now requires the main download to be an **SD Card ZIP with all Tape
Workshop plug-ins**, their resources and licenses. Its only top-level folders
are `autorun/`, the required firmware 1.4 root `autorun.sh`, and `Melodee/Plugins/`. No source archives or standalone guide ZIPs
belong inside this download. Preserving already-installed plug-ins is not a
substitute for including all of them. Keep matching source as a separate download
(or link unchanged plug-in source from the matching earlier release).

Use `tools/workshop_complete.py` after building and verifying the signed OS ZIP.
The builder checks every bundle in `tools/workshop_plugins.json`, its metadata,
ARM64 libraries and licenses. It records hashes of every bundled resource and
preserves the exact original OS bytes. Update the inventory deliberately when
the library changes; never silently publish with a missing bundle.

Filter the verified Complete ZIP to `autorun/`, root `autorun.sh` and `Melodee/Plugins/`, preserving
all bytes and ZIP permissions; verify every runtime and plug-in resource hash.
Publish this as the recommended SD Card ZIP. Keep corresponding source and the
signed web-updater ZIP separate, clearly marked as unnecessary for SD copying.
Record the SHA-256 in the release notes; separate checksum files are optional. Keep the smaller signed OS ZIP for the existing web
updater; that updater does not replace plug-in folders.

The standalone Melodee plug-in-UI build and Mac plug-in installer are separate
products. Do not substitute either for the full Tape Workshop OS.

Build example (paths are explicit release inputs):

```sh
python3 tools/workshop_complete.py \
  --runtime Tape-Workshop-vVERSION-Signed-Update.zip \
  --plugins PATH/TO/Melodee/Plugins \
  --plugin-source Tape-Workshop-Plugins-Source.tar.gz \
  --os-source Melodee-vVERSION-source.tar.gz \
  --guides Tape-Workshop-vVERSION-Guides.zip \
  --output Tape-Workshop-vVERSION-Complete.zip
```

Copy `autorun` and root `autorun.sh`, and merge `Melodee/Plugins` onto the SD card. Back up matching
plug-in folders before replacing them; retain user presets, projects, samples
and other third-party plug-ins. Source archives need not be copied to the card.

For firmware 1.4 packages pass `--launcher base/install/top-autorun.sh` to the complete builder. The signed browser updater remains autorun-folder-only; a first firmware-1.4 SD install must also copy the root launcher.
