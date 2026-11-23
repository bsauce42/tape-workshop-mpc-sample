#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
from pathlib import Path
import json,os,shutil,subprocess,zipfile
root=Path(__file__).resolve().parents[2]
zig=os.environ.get('ZIG_BIN','/Users/buddysaucedo/Documents/Codex/2026-11-13/h/work/zig-aarch64-macos-0.14.1/zig')
bank=root/'build/violin-bank'
if not (bank/'violin.bank').exists():subprocess.run(['python3',str(Path(__file__).with_name('prepare.py'))],check=True)
dest=root/'build/strings-card/Melodee/Plugins/Tape Workshop - VST - Strings'
dest.mkdir(parents=True,exist_ok=True)
subprocess.run([zig,'c++','-target','aarch64-linux-gnu.2.36','-mcpu=cortex_a55','-std=c++17','-O2','-fPIC','-fvisibility=hidden','-shared','-s','-Isrc','plugins/tape-workshop-violin/strings.cpp','-o',str(dest/'tape_workshop_strings.so')],cwd=root,check=True)
for name in ['violin.bank','SAMPLES-LICENSE.txt','SAMPLES-SOURCE.json']:shutil.copy2(bank/name,dest/name)
(dest/'plugin-meta.xml').write_text('<PLUGIN name="Tape Workshop Strings" descriptiveName="Tape Workshop Strings" format="VST" category="Synth" manufacturer="Tape Workshop" version="0.4.1" file="tape_workshop_strings.so" uid="54575631" isInstrument="1" numInputs="0" numOutputs="2" isShell="0"/>\n')
(dest/'version.xml').write_text('<version>0.4.1</version>\n')
(dest/'mpc-plugin.json').write_text(json.dumps(dict(schema=1,id='tape-workshop-strings',name='Tape Workshop Strings',version='0.4.1',kind='instrument',uid='54575631',manufacturer='Tape Workshop',so='tape_workshop_strings.so',arch='aarch64',max_glibc='2.36',os_compat=['Melodee','Tape Workshop'],param_compat=1),indent=2))
for name in ['COPYING','LICENSE']:
    if (root/name).exists():shutil.copy2(root/name,dest/'LICENSE.txt');break
(dest/'NOTICE.txt').write_text('Original Tape Workshop Strings code, AGPL-3.0-only. Host ABI based on Melodee by Kerem Kiliç (Ellic Studio). Samples: VSCO 2 Community Edition by Versilian Studios, Sam Gossner and Simon Dalzell; cutting by Elan Hickler/Soundemote. CC0-1.0. See SAMPLES-SOURCE.json for exact files and pinned source revision. Stereo downmixed to mono, converted to 16-bit; original sample rates retained. Not endorsed by sample creators.\n')
shutil.copy2(Path(__file__).with_name('README.md'),dest/'README.txt')
skin=dest/'Plugin Skins';skin.mkdir(exist_ok=True)
rows=[{'Tab':n+1,'SubTab':1,'Bank Direction':'Column','Q-Links':{f'Q-Link {k}':n*3+k-1 if k<=3 and n*3+k-1<43 else -1 for k in range(1,17)}} for n in range(15)]
for file in ['Q-Links.json','Q-Links - 8by1.json']:(skin/file).write_text(json.dumps({'version':4,'info':{'version':1,'type':'CompleteDescription'},'Screen Mode Q-Links':{'version':4,'map':rows}},indent=2))
out=root.parents[2]/'outputs/Tape-Workshop-Strings-v0.4.1-Plugin.zip'
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for p in sorted(dest.rglob('*')):
        if p.is_file():z.write(p,p.relative_to(root/'build/strings-card'))
print(out)
