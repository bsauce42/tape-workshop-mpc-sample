#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
from pathlib import Path
import os,json,shutil,subprocess,zipfile
root=Path(__file__).resolve().parents[2]
zig=os.environ.get('ZIG_BIN','/Users/buddysaucedo/Documents/Codex/2026-11-13/h/work/zig-aarch64-macos-0.14.1/zig')
bank=root/'build/orchestra-bank'
if not (bank/'orchestra.bank').is_file():raise SystemExit('Run prepare.py with numpy/scipy first.')
dest=root/'build/orchestra-card/Melodee/Plugins/Tape Workshop - VST - Orchestra';dest.mkdir(parents=True,exist_ok=True)
subprocess.run([zig,'c++','-target','aarch64-linux-gnu.2.36','-mcpu=cortex_a55','-std=c++17','-O2','-fPIC','-fvisibility=hidden','-shared','-s','-Isrc','plugins/tape-workshop-orchestra/orchestra.cpp','-o',str(dest/'tape_workshop_orchestra.so')],cwd=root,check=True)
for name in ['orchestra.bank','SAMPLES-LICENSE.txt','SAMPLES-SOURCE.json']:shutil.copy2(bank/name,dest/name)
(dest/'plugin-meta.xml').write_text('<PLUGIN name="Tape Workshop Orchestra" descriptiveName="Tape Workshop Orchestra" format="VST" category="Synth" manufacturer="Tape Workshop" version="0.2.1" file="tape_workshop_orchestra.so" uid="54574f31" isInstrument="1" numInputs="0" numOutputs="2" isShell="0"/>\n')
(dest/'version.xml').write_text('<version>0.2.1</version>\n')
(dest/'mpc-plugin.json').write_text(json.dumps(dict(schema=1,id='tape-workshop-orchestra',name='Tape Workshop Orchestra',version='0.2.1',kind='instrument',uid='54574f31',manufacturer='Tape Workshop',so='tape_workshop_orchestra.so',arch='aarch64',max_glibc='2.36',os_compat=['Melodee','Tape Workshop'],param_compat=1),indent=2))
for name in ['LICENSE','COPYING']:
    if (root/name).is_file():shutil.copy2(root/name,dest/'LICENSE.txt');break
(dest/'NOTICE.txt').write_text('Original Tape Workshop Orchestra DSP configuration and screen, AGPL-3.0-only. Host based on Melodee by Kerem Kiliç (Ellic Studio). Samples: VSCO 2 Community Edition by Versilian Studios, Sam Gossner and Simon Dalzell; cutting by Elan Hickler/Soundemote, CC0-1.0. Exact source revision, processing and SHA256s in SAMPLES-SOURCE.json. No commercial orchestral library code, recordings or interface art included.\n')
shutil.copy2(Path(__file__).with_name('README.md'),dest/'README.txt')
skin=dest/'Plugin Skins';skin.mkdir(exist_ok=True)
rows=[{'Tab':n+1,'SubTab':1,'Bank Direction':'Column','Q-Links':{f'Q-Link {k}':n*3+k-1 if k<=3 and n*3+k-1<48 else -1 for k in range(1,17)}} for n in range(16)]
for file in ['Q-Links.json','Q-Links - 8by1.json']:(skin/file).write_text(json.dumps({'version':4,'info':{'version':1,'type':'CompleteDescription'},'Screen Mode Q-Links':{'version':4,'map':rows}},indent=2))
out=root.parents[2]/'outputs/Tape-Workshop-Orchestra-v0.2.1-Plugin.zip'
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for file in sorted(dest.rglob('*')):
        if file.is_file():z.write(file,file.relative_to(root/'build/orchestra-card'))
print(out)
