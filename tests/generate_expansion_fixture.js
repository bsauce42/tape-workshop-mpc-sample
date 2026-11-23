// SPDX-License-Identifier: AGPL-3.0-only
// node tests/generate_expansion_fixture.js OUTPUT_DIR COVER.jpg
// The web archive is generated with the unchanged production builder.
const fs=require('fs'),path=require('path'),zlib=require('zlib');
const api=require('../src/melodee/web/static/expansion-core.js');
const [out,coverFile]=process.argv.slice(2);if(!out||!coverFile)throw Error('OUTPUT_DIR and COVER.jpg required');
fs.mkdirSync(out,{recursive:true});const cover=fs.readFileSync(coverFile);fs.writeFileSync(path.join(out,'Cover.jpg'),cover);
let pads=Array(16).fill(null);
for(const i of [0,7,15]){const audio=Float32Array.from({length:4410},(_,n)=>.2*Math.sin(n*2*Math.PI*(220+i*30)/44100));pads[i]={bytes:api.wav({length:4410,numberOfChannels:1,sampleRate:44100,getChannelData:()=>audio}),frames:4410};}
fs.writeFileSync(path.join(out,'Web-Builder.xpn'),api.expansion('Bridge Kit','Tape Workshop',pads,cover));
const dir=path.join(out,'MPC3'),data=path.join(dir,'Test_[TrackData]');fs.mkdirSync(data,{recursive:true});
for(const name of ['Pad-01.wav','Pad-18.wav'])fs.writeFileSync(path.join(data,name),pads[0].bytes);
let instruments=Array.from({length:32},()=>({layersv:[]}));
for(const [n,file]of [[0,'Pad-01.wav'],[17,'Pad-18.wav']])instruments[n]={mixable:{volume:.7079,pan:.5},layersv:[{active:true,sampleFile:file,velocityStart:0,velocityEnd:127,sliceInfo:{Start:0,End:4409}}]};
const document={data:{program:{name:'Desktop Test',type:0,drum:{instruments}}}};
fs.writeFileSync(path.join(dir,'Test.xty'),zlib.gzipSync('ACVS\n3.5\nSerialisableTrackData\njson\nmacOS\n'+JSON.stringify(document)));
fs.writeFileSync(path.join(dir,'Cover.jpg'),cover);
fs.writeFileSync(path.join(dir,'Expansion.xml'),'<expansion><title>Desktop Test</title><manufacturer>Desktop Maker</manufacturer><image>Cover.jpg</image></expansion>');
