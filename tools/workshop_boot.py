#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Fit the supplied logo without redrawing it, then compile its black splash.
Regeneration uses macOS Swift/CoreGraphics; generated assets build on all hosts.
"""
from pathlib import Path
import shutil, subprocess
from boot_art import main
root=Path(__file__).resolve().parents[1]
out=root/'build/brand-assets';out.mkdir(parents=True,exist_ok=True)
subprocess.run(['swift','-module-cache-path',str(out/'ModuleCache'),str(root/'tools/brand_assets.swift'),str(root/'docs/images/tape-workshop-logo-source.png'),str(out)],check=True)
shutil.copy2(out/'wordmark.png',root/'docs/images/tape-workshop-wordmark.png')
shutil.copy2(out/'splash.png',root/'docs/images/tape-workshop-splash.png')
for size in (180,192,512):shutil.copy2(out/f'icon-{size}.png',root/f'src/melodee/web/static/app-icon-{size}.png')
main(out/'splash.png',out/'black.png')
