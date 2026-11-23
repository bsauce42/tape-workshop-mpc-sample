#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Sign a staged Tape Workshop autorun folder for the existing SD updater.

Requires the `cryptography` Python package. The private key stays outside Git
at .local/tape-workshop-release-signing.pem; only the public key in update.h
ships. Upload the resulting *-Signed-Update.zip and its .sha256 to the matching
GitHub release. Mirror that ZIP on the repository's `updates` branch for
browser-readable automatic transfer; no credentials belong in the webpage.
"""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import zipfile
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
import make_release
import workshop_install
import workshop_runtime

ROOT = Path(__file__).resolve().parents[1]
LIST = 'autorun/melodee/RELEASE.sha256'
KEY = ROOT / '.local/tape-workshop-release-signing.pem'
PATH = re.compile(r'autorun/[A-Za-z0-9._+/-]+')
VERSION = re.compile(r'v\d+\.\d+\.\d+-tapeworkshop-community\.(\d+)')


def safe(name):
    return bool(PATH.fullmatch(name)) and all(p not in ('', '.', '..') for p in name.split('/'))


def public_key():
    return serialization.load_pem_public_key(make_release.public_key().encode())


def verify(path):
    with zipfile.ZipFile(path) as archive:
        if len(archive.namelist()) != len(set(archive.namelist())):
            raise ValueError('Duplicate ZIP entries')
        listing = archive.read(LIST)
        public_key().verify(archive.read(LIST+'.sig'), listing)
        entries = {}
        for line in listing.decode().splitlines():
            digest, name = line.split('  ', 1)
            if not re.fullmatch('[0-9a-f]{64}', digest) or not safe(name) or name in entries:
                raise ValueError('Invalid signed file list')
            entries[name] = digest
        actual = {n for n in archive.namelist() if n.startswith('autorun/') and not n.endswith('/') and n not in (LIST, LIST+'.sig')}
        if set(entries) != actual:
            raise ValueError('Signed list must cover every autorun file')
        for name, digest in entries.items():
            if hashlib.sha256(archive.read(name)).hexdigest() != digest:
                raise ValueError('Damaged file: '+name)
        if archive.read('autorun/melodee/app').strip() != b'melodee':
            raise ValueError('Not the Tape Workshop application package')
        version = archive.read('autorun/melodee/VERSION').decode().split()[0]
        if not VERSION.fullmatch(version):
            raise ValueError('Not a Tape Workshop release version')
        return version


def sign(package, output, key=KEY):
    package, output, key = Path(package), Path(output), Path(key)
    version = (package/'autorun/melodee/VERSION').read_text().split()[0]
    if not VERSION.fullmatch(version):
        raise ValueError('Expected vX.Y.Z-tapeworkshop-community.N')
    private = serialization.load_pem_private_key(key.read_bytes(), password=None)
    if not isinstance(private, Ed25519PrivateKey):
        raise ValueError('Expected an Ed25519 signing key')
    der = lambda p: p.public_bytes(serialization.Encoding.DER, serialization.PublicFormat.SubjectPublicKeyInfo)
    if der(private.public_key()) != der(public_key()):
        raise ValueError('Signing key does not match this Tape Workshop build')
    # Every release carries the fork-specific updater, including staged builds.
    folder = package/'autorun/melodee'
    (folder/'sd-worker.sh').write_text(workshop_install.worker())
    (folder/'sd-update.sh').write_text(workshop_install.updater(public_key().public_bytes(serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo).decode()))
    workshop_runtime.prepare(folder)
    sums = folder/'SHA256SUMS'
    names = []
    for line in sums.read_text().splitlines():
        fields = line.split('  ', 1)
        if len(fields) != 2 or not safe('autorun/melodee/' + fields[1]):
            raise ValueError('Invalid startup checksum path')
        names.append(fields[1])
    for name in ('sample-base', 'restore-stock.sh', 'sd-worker.sh', 'sd-update.sh'):
        if name not in names:
            raise ValueError('Startup checksums must cover ' + name)
    sums.write_text(''.join(hashlib.sha256((folder/name).read_bytes()).hexdigest()
                           + '  ' + name + '\n' for name in names))
    files = {}
    for path in sorted((package/'autorun').rglob('*')):
        if not path.is_file():
            continue
        name = path.relative_to(package).as_posix()
        if name in (LIST, LIST+'.sig') or path.name == '.DS_Store':
            continue
        if not safe(name):
            raise ValueError('Unsafe update path: '+name)
        files[name] = path.read_bytes()
    for name in ('sample-base','restore-stock.sh','sd-boot.sh','sd-worker.sh','sd-update.sh','SHA256SUMS','app','VERSION'):
        if not files.get('autorun/melodee/'+name):
            raise ValueError('Missing required update file: '+name)
    for name in ('autorun.sh','autorun.json','autorun.check'):
        if not files.get('autorun/'+name):
            raise ValueError('Missing autorun file: '+name)
    if files["autorun/melodee/app"].strip() != b"melodee":
        raise ValueError("Not the Tape Workshop application package")
    listing = make_release.release_list(files)
    files[LIST] = listing
    files[LIST+'.sig'] = private.sign(listing)
    timestamp = int(subprocess.check_output(['git','log','-1','--format=%ct'],cwd=ROOT,text=True).strip())
    make_release.write_zip(files, output, timestamp)
    verify(output)
    output.with_suffix('.zip.sha256').write_text(hashlib.sha256(output.read_bytes()).hexdigest()+'  '+output.name+'\n')
    # Include the same signed files in the full manual-install bundle.
    (package/LIST).write_bytes(listing)
    (package/(LIST+'.sig')).write_bytes(files[LIST+'.sig'])
    return version


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--key', type=Path, default=KEY)
    parser.add_argument('--verify', type=Path)
    args = parser.parse_args()
    if args.verify:
        print('Verified signed release:', verify(args.verify))
    else:
        if not args.package or not args.output:
            parser.error('--package and --output are required')
        print('Signed release:', sign(args.package,args.output,args.key))
