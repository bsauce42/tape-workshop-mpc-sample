#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Prepare a pinned CC0 solo violin bank; downloads stay in ignored build/."""
from pathlib import Path
import array, concurrent.futures, hashlib, json, re, struct, sys, urllib.request, urllib.parse, wave
ROOT = Path(__file__).resolve().parents[2]
CACHE = ROOT / 'build/violin-source'
OUT = ROOT / 'build/violin-bank'
CACHE.mkdir(parents=True, exist_ok=True)
OUT.mkdir(parents=True, exist_ok=True)
REVISION = '440300901dfe9275fd84e0b7763af1f8443ae62e'
pin = CACHE / 'revision.txt'
if not pin.exists():
    pin.write_text(REVISION)
revision = REVISION
base = f'https://raw.githubusercontent.com/sgossner/VSCO-2-CE/{revision}/'
def get(path):
    dest = CACHE / (hashlib.sha256(path.encode()).hexdigest()[:12] + '-' + Path(path).name)
    if not dest.exists():
        data = urllib.request.urlopen(base + urllib.parse.quote(path), timeout=90).read()
        dest.write_bytes(data)
    return dest
tree = json.load(urllib.request.urlopen(f'https://api.github.com/repos/sgossner/VSCO-2-CE/git/trees/{revision}?recursive=1'))['tree']
jobs = []
groups = [('Solo Violin','Arco Vib','Pizz'), ('Viola Section','susvib','pizz'), ('Cello Section','susvib','pizzT'), ('Solo Contrabass','SusVib','Pizz')]
for instrument,(group,bow,pluck) in enumerate(groups):
    for articulation,folder in enumerate([bow,pluck]):
        candidates = []
        for item in tree:
            path = item['path']
            if not path.startswith(f'Strings/{group}/{folder}/') or not path.endswith('.wav'): continue
            match = re.search(r'_([A-G]#?)(-?\d)_',Path(path).name)
            if not match: continue
            key = {'C':0,'D':2,'E':4,'F':5,'G':7,'A':9,'B':11}[match[1][0]] + ('#' in match[1]) + 12*(int(match[2])+1)
            velocity = re.search(r'_(p|f|v[123])(?:_|\.)',Path(path).name)
            layer = 0 if velocity and velocity[1] in ('p','v1') else 1
            candidates.append((key,layer,path))
        for key in sorted(set(c[0] for c in candidates)):
            for layer in range(2):
                choices = sorted(c[2] for c in candidates if c[:2] == (key,layer))
                if choices: jobs.append((choices[0],instrument*2+articulation,key,layer))
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    files = list(pool.map(lambda j:get(j[0]), jobs))
MAPPING_REVISION = '6dd651d55dde97fd4028699be9d4481f26917891'
MAPPING_FILES = ['SViolinVib.sfz', 'SViolinPizz.sfz', 'ViolaEnsSusVib.sfz', 'ViolaEnsPizz.sfz', 'CelloEnsSusVib.sfz', 'CelloEnsPizz.sfz', 'ContrabassSusVB.sfz', 'ContrabassPizz.sfz']
mapping = {}
for file in MAPPING_FILES:
    data = urllib.request.urlopen(f'https://raw.githubusercontent.com/sgossner/VSCO-2-CE/{MAPPING_REVISION}/{file}').read().decode()
    for region in data.split('<region>')[1:]:
        name = re.search(r'sample=([^\r\n]+)',region)
        key = re.search(r'pitch_keycenter=(\d+)',region)
        if not name or not key: continue
        tune = re.search(r'(?:^|\s)tune=(-?\d+)',region)
        gain = re.search(r'(?:^|\s)volume=(-?[\d.]+)',region)
        mapping[name[1].strip()] = (int(key[1]), int(tune[1]) if tune else 0, 10**(float(gain[1])/20) if gain else 1)

records, pcm, sources = [], bytearray(), []
for (path, art, key, layer), file in zip(jobs, files):
    with wave.open(str(file),'rb') as w:
        channels,width,rate,n = w.getnchannels(),w.getsampwidth(),w.getframerate(),w.getnframes()
        raw = w.readframes(n)
    assert width in (2,3,4) and channels in (1,2)
    values = array.array('h')
    for at in range(0,len(raw),width*channels):
        total = sum(int.from_bytes(raw[at+c*width:at+(c+1)*width], 'little', signed=True) for c in range(channels))
        value = max(-32768,min(32767, round(total/channels/(2**(width*8-16)))))
        values.append(value)
    # Preserve recorded attacks and natural tails. Do not normalize velocity layers independently.
    if sys.byteorder != 'little': values.byteswap()
    key,tune,gain = mapping[Path(path).name]
    records.append((art,key,layer,rate,len(pcm)//2,len(values),tune,gain))
    pcm.extend(values.tobytes())
    sources.append(dict(path=path,sha256=hashlib.sha256(file.read_bytes()).hexdigest()))
header = b'TWV2' + struct.pack('<II',len(records),len(pcm)//2)
(OUT/'violin.bank').write_bytes(header+b''.join(struct.pack('<6Iif',*r) for r in records)+pcm)
license_file=get('LICENSE')
(OUT/'SAMPLES-LICENSE.txt').write_bytes(license_file.read_bytes())
(OUT/'SAMPLES-SOURCE.json').write_text(json.dumps(dict(repository='https://github.com/sgossner/VSCO-2-CE',revision=revision,mapping_revision=MAPPING_REVISION,mapping_files=MAPPING_FILES,license='CC0-1.0',changes='Stereo downmix to mono, 16-bit PCM; original sample rates and velocity-layer levels retained.',samples=sources),indent=2))
print(f'{len(records)} sample zones, {len(pcm)/1048576:.1f} MiB PCM: {OUT}')
