#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Add Tape Workshop's voice resources to its runtime without editing base/."""
from pathlib import Path
import argparse

def prepare(folder):
    folder=Path(folder)
    if not (folder/'voice/decoder.so').is_file():
        raise ValueError('Missing voice decoder in release')
    script=folder/'sd-worker.sh'
    content=script.read_text()
    marker='# Tape Workshop voice runtime resources'
    if marker not in content:
        anchor='chmod 755 "$RELEASE/sample-base"'
        if anchor not in content: raise ValueError('Unknown startup worker layout')
        block='''# Tape Workshop voice runtime resources
# VoiceSearch uses /run/sample-base/voice. Copy and verify before starting.
if [ -d "$DIR/voice" ]; then
    cp -r "$DIR/voice" "$RELEASE/"
    (cd "$DIR" && find voice -type f -exec sha256sum {} \\;) | (cd "$RELEASE" && sha256sum -c -)
fi
'''
        content=content.replace(anchor,block+anchor)
        script.write_text(content)
    return script

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('folder');args=p.parse_args();print(prepare(args.folder))
