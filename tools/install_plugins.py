#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Install Linux ARM plug-ins on a mounted Melodee SD card, without changing its OS."""
import argparse
from pathlib import Path
import shutil
import struct
import tempfile


def arm_elf(path):
    with path.open('rb') as f:
        h = f.read(20)
    if len(h) < 20 or h[:4] != b'\x7fELF' or h[5] != 1:
        return False
    if struct.unpack_from('<H', h, 16)[0] != 3:  # ET_DYN: shared library
        return False
    machine = struct.unpack_from('<H', h, 18)[0]
    return (h[4], machine) in ((2, 183), (1, 40))


def inspect(path):
    if path.is_symlink() or not path.exists():
        raise ValueError('Missing plug-in or symbolic link: ' + str(path))
    if path.is_file():
        if path.suffix.lower() not in ('.so', '.clap') or not arm_elf(path):
            raise ValueError('Expected a Linux ARM .so/.clap, not a Mac/Windows/PC build: ' + str(path))
        return
    if path.suffix.lower() not in ('.vst3', '.lv2') and not (path/'plugin-meta.xml').is_file():
        raise ValueError('Select a .vst3/.lv2 bundle or an Open MPC folder containing plugin-meta.xml')
    files = list(path.rglob('*'))
    if any(f.is_symlink() for f in files):
        raise ValueError('Bundle contains symbolic links; use a self-contained Linux ARM distribution')
    if not any(f.is_file() and f.suffix.lower() in ('.so', '.clap') and arm_elf(f) for f in files):
        raise ValueError('Bundle has no Linux ARM plug-in library: ' + str(path))


def install(card, sources, dry_run=False):
    card = card.expanduser().resolve(strict=True)
    if not card.is_dir():
        raise ValueError('Card must be an existing mounted SD-card directory')
    # Resolve each destination component independently to avoid escaping through a link.
    folder = card/'Melodee'/'Plugins'
    for p in (card/'Melodee', folder):
        if p.is_symlink() or (p.exists() and not p.is_dir()):
            raise ValueError('Unsafe plug-in destination: ' + str(p))
    names = set()
    for source in sources:
        inspect(source)
        if source.name in names or (folder/source.name).exists() or (folder/source.name).is_symlink():
            raise ValueError('Already exists; no files replaced: ' + source.name)
        names.add(source.name)
    size = sum(f.stat().st_size for s in sources for f in ([s] if s.is_file() else s.rglob('*')) if f.is_file())
    if shutil.disk_usage(card).free < size + (4 << 20):
        raise ValueError('Not enough free space on the SD card')
    if not dry_run:
        folder.mkdir(parents=True, exist_ok=True)
        # Copy all payloads first. Existing files and autorun are never touched.
        with tempfile.TemporaryDirectory(prefix='.plugin-install-', dir=folder) as tmp:
            stage = Path(tmp)
            for s in sources:
                if s.is_dir():
                    shutil.copytree(s, stage/s.name)
                else:
                    shutil.copy2(s, stage/s.name)
            for s in sources:
                target = folder/s.name
                if target.exists() or target.is_symlink():
                    raise ValueError('Destination appeared during copy: ' + str(target))
                (stage/s.name).rename(target)
    return folder


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--card', type=Path, required=True, help='Mounted SD card, e.g. /Volumes/MPC')
    parser.add_argument('--dry-run', action='store_true', help='Check compatibility and paths without copying')
    parser.add_argument('plugins', nargs='+', type=Path, help='Plug-in files or complete bundle folders')
    args = parser.parse_args()
    try:
        folder = install(args.card, args.plugins, args.dry_run)
    except (ValueError, OSError) as error:
        parser.exit(1, str(error) + '\n')
    print(('Checked: ' if args.dry_run else 'Installed into: ') + str(folder))
    print('Eject the card, start Melodee and rescan plug-ins. Compatibility still depends on the host and dependencies.')
    print('Tape Workshop live screens require host integration; this installer cannot add them to another OS.')


if __name__ == '__main__':
    main()
