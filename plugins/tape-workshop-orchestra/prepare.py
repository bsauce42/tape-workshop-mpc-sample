#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Build bounded, pinned CC0 orchestra samples. Requires numpy and scipy."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import hashlib,json,math,re,struct,urllib.request,urllib.parse,wave,time
import numpy as np
from scipy.signal import resample_poly
ROOT=Path(__file__).resolve().parents[2]
CACHE=ROOT/'build/orchestra-source';OUT=ROOT/'build/orchestra-bank'
CACHE.mkdir(parents=True,exist_ok=True);OUT.mkdir(parents=True,exist_ok=True)
REVISION='440300901dfe9275fd84e0b7763af1f8443ae62e'
MAPPING='6dd651d55dde97fd4028699be9d4481f26917891'
INSTRUMENTS=json.loads(Path(__file__).with_name('instruments.json').read_text())
def get(path,revision=REVISION):
    dest=CACHE/(hashlib.sha256((revision+path).encode()).hexdigest()[:16]+'-'+Path(path).name)
    if not dest.exists():
        for attempt in range(3):
            try:
                data=urllib.request.urlopen(f'https://raw.githubusercontent.com/sgossner/VSCO-2-CE/{revision}/'+urllib.parse.quote(path),timeout=60).read()
                temporary=dest.with_suffix('.new');temporary.write_bytes(data);temporary.replace(dest);break
            except Exception:
                if attempt==2:raise
                time.sleep(attempt+1)
    return dest
jobs=[]
for instrument,(name,sustain,short) in enumerate(INSTRUMENTS):
    for art,sfz in enumerate([sustain,short]):
        text=get(sfz,MAPPING).read_text();prefix=re.search(r'default_path=([^\r\n]+)',text)
        if not prefix:raise ValueError('Missing sample directory: '+sfz)
        prefix=prefix[1].strip().replace('\\','/')
        candidates={}
        for region in text.split('<region>')[1:]:
            def field(name,default=None):
                match=re.search(r'(?:^|\s)'+name+r'=([^\s]+)',region)
                return match[1] if match else default
            sample=re.search(r'sample=([^\r\n]+)',region);key=field('pitch_keycenter')
            if not sample or key is None:continue
            filename=sample[1].strip().split('//')[0].strip().replace('\\','/')
            key=int(key);lo=int(field('lovel','0'));hi=int(field('hivel','127'))
            candidates.setdefault(key,[]).append((lo,hi,prefix+filename,int(field('tune','0')),float(field('volume','0'))))
        keys=sorted(candidates)
        # Retain range endpoints and at most roughly one sampled root per five semitones.
        chosen=[]
        for key in keys:
            if not chosen or key-chosen[-1]>=5:chosen.append(key)
        if keys and keys[-1] not in chosen:chosen.append(keys[-1])
        for key in chosen:
            choices=sorted(candidates[key],key=lambda c:(c[0],c[2]))
            for layer,target in enumerate([32,110]):
                choice=min(choices,key=lambda c:(0 if c[0]<=target<=c[1] else min(abs(c[0]-target),abs(c[1]-target)),c[2]))
                lo,hi,path,tune,gain=choice;jobs.append((instrument*2+art,key,layer,path,tune,gain,sfz))
print('Fetching',len(set(j[3] for j in jobs)),'unique recordings for',len(jobs),'zones',flush=True)
with ThreadPoolExecutor(max_workers=4) as pool:
    fetched=dict(zip(sorted(set(j[3] for j in jobs)),pool.map(get,sorted(set(j[3] for j in jobs)))))
records=[];pcm=bytearray();sources=[];converted={}
for group,key,layer,path,tune,gain,sfz in jobs:
    if path not in converted:
        file=fetched[path]
        with wave.open(str(file),'rb') as w:
            channels,width,rate,frames=w.getnchannels(),w.getsampwidth(),w.getframerate(),w.getnframes();raw=w.readframes(frames)
        if width==3:
            b=np.frombuffer(raw,dtype=np.uint8).reshape(-1,3).astype(np.int32)
            data=b[:,0]|(b[:,1]<<8)|(b[:,2]<<16);data=(data^(1<<23))-(1<<23);data=data.astype(np.float32)/(1<<23)
        elif width in (2,4):data=np.frombuffer(raw,dtype='<i2' if width==2 else '<i4').astype(np.float32)/(1<<(width*8-1))
        else:raise ValueError('Unsupported PCM width')
        data=data.reshape(-1,channels).mean(axis=1)
        # Preserve attacks; trim long silence and cap sustained recordings for bounded memory.
        magnitude=np.abs(data);threshold=max(float(magnitude.max())*.003,.000001)
        nonzero=np.flatnonzero(magnitude>threshold)
        start=max(0,int(nonzero[0])-int(rate*.02)) if len(nonzero) else 0
        data=data[start:start+int(rate*4.5)]
        factor=math.gcd(rate,32000);data=resample_poly(data,32000//factor,rate//factor)
        samples=np.clip(np.rint(data*32767),-32768,32767).astype('<i2')
        if len(samples)<8:raise ValueError('Empty recording: '+path)
        # Short and percussion tails fade before the bounded recording ends.
        fade=min(len(samples)//4,640);samples[-fade:]=(samples[-fade:].astype(np.float32)*np.linspace(1,0,fade)).astype('<i2')
        offset=len(pcm)//2;pcm.extend(samples.tobytes());converted[path]=(offset,len(samples))
        sources.append(dict(path=path,sha256=hashlib.sha256(file.read_bytes()).hexdigest(),trim_start_frames=start))
    offset,frames=converted[path]
    records.append((group,key,layer,32000,offset,frames,tune,10**(gain/20)))
assert len(pcm)<=160*1024*1024 and len(records)<=768
(OUT/'orchestra.bank').write_bytes(b'TWV2'+struct.pack('<II',len(records),len(pcm)//2)+b''.join(struct.pack('<6Iif',*r) for r in records)+pcm)
(OUT/'SAMPLES-LICENSE.txt').write_bytes(get('LICENSE').read_bytes())
(OUT/'SAMPLES-SOURCE.json').write_text(json.dumps(dict(repository='https://github.com/sgossner/VSCO-2-CE',revision=REVISION,mapping_revision=MAPPING,license='CC0-1.0',instruments=INSTRUMENTS,changes='Nearest sampled roots thinned to approximately five semitones; two velocity layers; mono 32 kHz/16-bit via polyphase resampling; leading silence trimmed, up to 4.5 seconds retained, 20 ms end fade. Original mapping pitch, tuning and gain retained.',zones=len(records),pcm_bytes=len(pcm),samples=sources),indent=2))
print('Prepared',len(records),'zones;',round(len(pcm)/1048576,1),'MiB PCM',flush=True)
