#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Build the required complete GitHub download without changing the signed OS ZIP."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile


def digest(data):
    return hashlib.sha256(data).hexdigest()


def complete(runtime, plugins, plugin_source, os_source, guides, output, launcher=None):
    inventory = json.loads(Path(__file__).with_name('workshop_plugins.json').read_text())
    files = {}
    with zipfile.ZipFile(runtime) as z:
        if z.testzip() is not None:
            raise ValueError('Damaged OS archive')
        for name in z.namelist():
            if not name.startswith('autorun/') or '..' in Path(name).parts:
                raise ValueError('Expected a signed autorun-only OS ZIP')
            if not name.endswith('/'):
                files[name] = z.read(name)
    if launcher is not None:
        files['autorun.sh'] = launcher.read_bytes()
    version = files['autorun/melodee/VERSION'].decode().split()[0]
    bundles = []
    for entry in inventory['bundles']:
        folder = plugins / entry['folder']
        if not (folder / 'plugin-meta.xml').is_file():
            raise ValueError('Missing plug-in metadata: ' + entry['folder'])
        if not any('LICENSE' in p.name.upper() for p in folder.iterdir()):
            raise ValueError('Missing plug-in license: ' + entry['folder'])
        for binary in entry['libraries']:
            data = (folder / binary).read_bytes()
            if len(data) < 20 or data[:6] != b'\x7fELF\x02\x01' or struct.unpack_from('<H', data, 18)[0] != 183:
                raise ValueError('Expected Linux ARM64 plug-in: ' + binary)
        members = {}
        for p in sorted(folder.rglob('*')):
            if p.is_symlink():
                raise ValueError('Symlink in plug-in bundle: ' + str(p))
            if p.is_file() and p.name != '.DS_Store':
                name = 'Melodee/Plugins/' + entry['folder'] + '/' + p.relative_to(folder).as_posix()
                data = p.read_bytes()
                files[name] = data
                members[p.relative_to(folder).as_posix()] = digest(data)
        bundles.append({'folder': entry['folder'], 'files': members})
    for source in (plugin_source, os_source, guides):
        if not source.is_file():
            raise ValueError('Missing matching source / guides archive: ' + str(source))
        files['Source/' + source.name] = source.read_bytes()
    manifest = {'version': version, 'bundles': bundles, 'runtime_sha256': digest(runtime.read_bytes()),
                'plugin_source_sha256': digest(plugin_source.read_bytes())}
    files['PLUGIN-INVENTORY.json'] = (json.dumps(manifest, indent=2) + '\n').encode()
    names = '\n'.join('- ' + item['folder'].replace('Tape Workshop - VST - ', '') for item in bundles)
    files['START-HERE.txt'] = (f'Tape Workshop {version} — complete OS + {len(bundles)} Tape Workshop plug-ins\n\n'
        'Power off the MPC and back up the SD card. Replace autorun with the included autorun folder.\n'
        'Merge Melodee/Plugins into the card library; back up matching plug-in folders before replacing them.\n'
        'Keep projects, samples, presets and other third-party plug-ins. Source is for your computer, not required on the card.\n'
        'This complete ZIP is for SD-card installation. For the web updater use the separate Signed-Update.zip.\n'
        'The standalone Melodee UI installer build is separate.\n\n'
        'Tape Workshop additions and branding: bsauce42.\n'
        'Based on Melodee by Kerem Kiliç (Ellic Studio). Original component licenses are retained.\n\n'
        'Included Tape Workshop plug-ins:\n' + names + '\n').encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name)
            info.create_system = 3
            info.compress_type = zipfile.ZIP_DEFLATED
            mode = 0o755 if name.endswith('.sh') or name.endswith('/sample-base') else 0o644
            info.external_attr = (0o100000 | mode) << 16
            z.writestr(info, data)
    with zipfile.ZipFile(output) as z:
        if z.testzip() is not None:
            raise ValueError('Complete ZIP verification failed')
        for name, data in files.items():
            if digest(z.read(name)) != digest(data):
                raise ValueError('Archive content mismatch: ' + name)
    output.with_suffix('.zip.sha256').write_text(digest(output.read_bytes()) + '  ' + output.name + '\n')
    return len(bundles), len(files)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for arg in ('runtime', 'plugins', 'plugin-source', 'os-source', 'guides', 'output'):
        parser.add_argument('--' + arg, required=True, type=Path)
    parser.add_argument('--launcher', type=Path, help='Firmware 1.4 card-root autorun.sh')
    args = parser.parse_args()
    count, files = complete(args.runtime, args.plugins, args.plugin_source, args.os_source, args.guides, args.output, args.launcher)
    print(f'Verified complete package: {count} plug-ins, {files} files: {args.output}')
