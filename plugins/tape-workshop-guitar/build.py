#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Build the ARM64 loadable plug-in without a withdrawn VST SDK."""
from pathlib import Path
import os,subprocess,json,shutil
root=Path(__file__).resolve().parents[2]
zig=os.environ.get('ZIG_BIN','/Users/buddysaucedo/Documents/Codex/2026-11-13/h/work/zig-aarch64-macos-0.14.1/zig')
dest=Path(os.environ.get('GUITAR_DEST',str(root/'build/tw-guitar-card/Melodee/Plugins/Tape Workshop - VST - Guitar')));dest.mkdir(parents=True,exist_ok=True)
subprocess.run([zig,'c++','-target','aarch64-linux-gnu.2.36','-mcpu=cortex_a55','-std=c++17','-O2','-fPIC','-fvisibility=hidden','-shared','-s','-Isrc','plugins/tape-workshop-guitar/guitar.cpp','-o',str(dest/'tape_workshop_guitar.so')],cwd=root,check=True)
(dest/'plugin-meta.xml').write_text('<PLUGIN name="Tape Workshop Guitar" descriptiveName="Tape Workshop Guitar" format="VST" category="Synth" manufacturer="Tape Workshop" version="0.1.0" file="tape_workshop_guitar.so" uid="54574731" isInstrument="1" numInputs="0" numOutputs="2" isShell="0"/>\n')
(dest/'version.xml').write_text('<version>0.1.0</version>\n')
(dest/'mpc-plugin.json').write_text(json.dumps(dict(schema=1,id='tape-workshop-guitar',name='Tape Workshop Guitar',version='0.1.0',kind='instrument',uid='54574731',manufacturer='Tape Workshop',so='tape_workshop_guitar.so',arch='aarch64',max_glibc='2.36',os_compat=['Melodee','Tape Workshop'],param_compat=1),indent=2)+'\n')
for name in ['COPYING','LICENSE']:
 if(root/name).is_file():shutil.copy2(root/name,dest/'LICENSE.txt');break
(dest/'NOTICE.txt').write_text('Original Tape Workshop Guitar DSP, AGPL-3.0-only. Host ABI and operating-system integration based on Melodee by Kerem Kiliç (Ellic Studio). No Steinberg SDK is included.\n')
skin=dest/'Plugin Skins';skin.mkdir(exist_ok=True)
rows=[{'Tab':n+1,'SubTab':1,'Bank Direction':'Column','Q-Links':{f'Q-Link {k}':n*3+k-1 if k<=3 and n*3+k-1<18 else -1 for k in range(1,17)}} for n in range(6)]
for file in ['Q-Links.json','Q-Links - 8by1.json']:(skin/file).write_text(json.dumps({'version':4,'info':{'version':1,'type':'CompleteDescription'},'Screen Mode Q-Links':{'version':4,'map':rows}},indent=2))
shutil.copy2(Path(__file__).with_name('README.md'),dest/'README.txt')
print(dest)
