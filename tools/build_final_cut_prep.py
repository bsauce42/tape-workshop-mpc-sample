#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Build the optional Mac helper before web_bundle.py on a macOS release host."""
from pathlib import Path
import subprocess,plistlib,zipfile,shutil
root=Path(__file__).resolve().parents[1];stage=root/'build/final-cut-prep';stage.mkdir(parents=True,exist_ok=True)
app=stage/'Tape Workshop Final Cut Prep.app';mac=app/'Contents/MacOS';resources=app/'Contents/Resources';mac.mkdir(parents=True,exist_ok=True);resources.mkdir(parents=True,exist_ok=True)
iconset=stage/'FinalCutPrep.iconset';iconset.mkdir(exist_ok=True)
subprocess.run(['xcrun','swift',str(root/'tools/final_cut_icon.swift'),str(stage/'icon.png')],check=True)
for size in [16,32,128,256,512]:
 for scale in [1,2]:
  subprocess.run(['sips','-z',str(size*scale),str(size*scale),str(stage/'icon.png'),'--out',str(iconset/('icon_'+str(size)+'x'+str(size)+('@2x' if scale==2 else '')+'.png'))],check=True,stdout=subprocess.DEVNULL)
subprocess.run(['iconutil','-c','icns',str(iconset),'-o',str(resources/'FinalCutPrep.icns')],check=True)
for arch in ['arm64','x86_64']:
 subprocess.run(['xcrun','swiftc','-swift-version','5','-O','-target',arch+'-apple-macosx13.0',str(root/'tools/final_cut_prep.swift'),'-o',str(stage/arch),'-framework','AppKit','-framework','AVFoundation','-framework','CoreImage'],check=True,stdout=subprocess.DEVNULL,stderr=(stage/(arch+'.log')).open('w'))
subprocess.run(['lipo','-create',str(stage/'arm64'),str(stage/'x86_64'),'-output',str(mac/'FinalCutPrep')],check=True)
(app/'Contents/Info.plist').write_bytes(plistlib.dumps({'CFBundleExecutable':'FinalCutPrep','CFBundleIdentifier':'com.tapeworkshop.finalcutprep','CFBundleName':'Tape Workshop Final Cut Prep','CFBundlePackageType':'APPL','CFBundleShortVersionString':'1.3','CFBundleVersion':'4','CFBundleIconFile':'FinalCutPrep.icns','LSMinimumSystemVersion':'13.0','NSHighResolutionCapable':True}))
subprocess.run(['codesign','--force','--sign','-',str(app)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
archive=root/'build/final-cut-prep.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in app.rglob('*'):
  if p.is_file():z.write(p,p.relative_to(stage))
 z.write(root/'tools/final_cut_prep.swift','Source/final_cut_prep.swift')
 z.write(root/'tools/final_cut_icon.swift','Source/final_cut_icon.swift')
 z.writestr('README.txt','Tape Workshop Final Cut Prep — macOS 13 or later, Apple Silicon and Intel.\n\nExtract the browser Final Cut recording ZIP, then open this app and select that folder. It creates transparent HEVC MOVs and Tutorial.fcpxml in a new Final Cut Ready folder. Import that XML into Final Cut Pro. Background is a separate clip. The originals remain untouched.\n\nSource is included under AGPL-3.0-only. Based on Melodee by Kerem Kiliç (Ellic Studio).\n')
 for name in ['COPYING','LICENSE']:
  if (root/name).is_file():z.write(root/name,'Source/'+name);break
print(archive)
