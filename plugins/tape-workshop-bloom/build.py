#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
from pathlib import Path
import os,subprocess,json,shutil
root=Path(__file__).resolve().parents[2]
zig=os.environ.get('ZIG_BIN','/Users/buddysaucedo/Documents/Codex/2026-11-13/h/work/zig-aarch64-macos-0.14.1/zig')
dest=Path(os.environ.get('BLOOM_DEST',str(root/'build/tw-bloom-card/Melodee/Plugins/Tape Workshop - VST - Bloom Reverb')));dest.mkdir(parents=True,exist_ok=True)
subprocess.run([zig,'c++','-target','aarch64-linux-gnu.2.36','-mcpu=cortex_a55','-std=c++17','-O2','-pthread','-fPIC','-fvisibility=hidden','-shared','-s','-Isrc','plugins/tape-workshop-bloom/bloom.cpp','-o',str(dest/'bloom_reverb.so')],cwd=root,check=True)
(dest/'plugin-meta.xml').write_text('<PLUGIN name="Bloom Reverb" descriptiveName="Bloom Reverb" format="VST" category="Fx|Reverb" manufacturer="Tape Workshop" version="0.1.1" file="bloom_reverb.so" uid="54574252" isInstrument="0" numInputs="2" numOutputs="2" isShell="0"/>\n')
(dest/'version.xml').write_text('<version>0.1.1</version>\n')
(dest/'mpc-plugin.json').write_text(json.dumps(dict(schema=1,id='bloom-reverb',name='Bloom Reverb',version='0.1.1',kind='effect',uid='54574252',manufacturer='Tape Workshop',so='bloom_reverb.so',arch='aarch64',max_glibc='2.36',os_compat=['Melodee','Tape Workshop'],param_compat=1),indent=2)+'\n')
for name in ['COPYING','LICENSE']:
 if(root/name).is_file():shutil.copy2(root/name,dest/'LICENSE.txt');break
(dest/'NOTICE.txt').write_text('Original Tape Workshop Bloom Reverb DSP and UI, AGPL-3.0-only. Host ABI and OS based on Melodee by Kerem Kiliç (Ellic Studio). No FabFilter code, graphics, presets, IRs or Steinberg SDK included.\n')
skin=dest/'Plugin Skins';skin.mkdir(exist_ok=True)
rows=[{'Tab':n+1,'SubTab':1,'Bank Direction':'Column','Q-Links':{f'Q-Link {k}':n*3+k-1 if k<=3 else -1 for k in range(1,17)}} for n in range(8)]
for name in ['Q-Links.json','Q-Links - 8by1.json']:(skin/name).write_text(json.dumps({'version':4,'info':{'version':1,'type':'CompleteDescription'},'Screen Mode Q-Links':{'version':4,'map':rows}},indent=2))
ir=dest/'IRs';ir.mkdir(exist_ok=True);(ir/'README.txt').write_text('Copy up to eight user-owned/licensed mono/stereo WAV impulse responses here, then reload the plug-in. Lowercase .wav or uppercase .WAV accepted. Slots use alphabetical filename order. PCM16/24/32 or float32. Maximum 96,256 resampled frames (about 2 seconds at 48 kHz). Longer/invalid IRs are rejected; the algorithmic engine remains available. No IR audio is supplied.\n')
shutil.copy2(Path(__file__).with_name('README.md'),dest/'README.txt')
print(dest)
