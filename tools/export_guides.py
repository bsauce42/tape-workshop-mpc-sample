#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Export the bundled manual/quickstart as linked offline HTML and image assets."""
import argparse
from pathlib import Path
import os
import re
import struct
from urllib.parse import urlsplit


def unpack(path):
    data = path.read_bytes()
    if data[:4] != b'MLWB':
        raise ValueError('Expected a Melodee web bundle')
    at, files = 4, {}
    while at < len(data):
        parts = []
        for _ in range(3):
            n = struct.unpack_from('<I', data, at)[0]
            at += 4
            parts.append(data[at:at+n])
            at += n
        files[parts[0].decode()] = parts[2]
    return files


def export(bundle, output):
    files = unpack(bundle)
    pages = {p: p.lstrip('/') + '.html' for p in files if p.startswith('/docs/')}
    assets = {p: p.lstrip('/') for p in files if p.startswith('/img/') or p in
              ('/app.css', '/modern.css', '/logo.png', '/app-icon-180.png', '/app-icon-192.png')}
    names = dict(assets, **pages)
    output.mkdir(parents=True, exist_ok=True)
    for path, name in names.items():
        dest = output/name
        dest.parent.mkdir(parents=True, exist_ok=True)
        if path not in pages:
            dest.write_bytes(files[path])
            continue
        text = files[path].decode()
        text = re.sub(r'<script\b[^>]*>.*?</script>', '', text, flags=re.S)
        text = re.sub(r'<link\b[^>]*rel="manifest"[^>]*>', '', text)
        text = re.sub(r'<link\b[^>]*href="/browser-media.css"[^>]*>', '', text)
        text = re.sub(r'<nav class="tabs".*?</nav>', '<nav class="tabs" aria-label="Guides"><a href="/docs/quickstart">Quickstart</a><a href="/docs/manual">Manual</a></nav>', text, flags=re.S)
        text = re.sub(r'<div class="link".*?</div>', '<div class="link">Offline guide</div>', text, flags=re.S)
        def local(match):
            attr, target = match.group(1), match.group(2)
            u = urlsplit(target)
            if not target.startswith('/'):
                return match.group(0)
            route = '/docs/quickstart' if u.path == '/' else u.path
            if route in names:
                href = os.path.relpath(output/names[route], dest.parent).replace(os.sep, '/')
                if u.fragment:
                    href += '#' + u.fragment
                return attr + '="' + href + '"'
            return attr + '="http://192.168.155.1' + target + '"'
        text = re.sub(r'(href|src)="([^"]+)"', local, text)
        dest.write_text(text)
    (output/'index.html').write_text('<!doctype html><meta charset="utf-8"><meta http-equiv="refresh" content="0;url=docs/quickstart.html"><a href="docs/quickstart.html">Open the Tape Workshop Quickstart</a>')
    (output/'README.txt').write_text('Tape Workshop illustrated guides. Open index.html.\nBased on Melodee by Kerem Kiliç (Ellic Studio).\nThe HTML manual works without an MPC connection. Device actions still require the connected MPC.\nUI images use example data rendered by the current native UI; they are not physical-device captures.\n')
    print('Exported', len(pages), 'guide pages and', len(assets), 'assets to', output)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('bundle', type=Path)
    p.add_argument('output', type=Path)
    a = p.parse_args()
    export(a.bundle, a.output)
