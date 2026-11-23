// SPDX-License-Identifier: AGPL-3.0-only
const assert=require('node:assert/strict');
const core=require('../src/melodee/web/static/browser-media-core.js');
function run(){
 const bytes=new ArrayBuffer(40),v=new DataView(bytes);v.setUint32(0,0x54574143,false);v.setUint32(4,48000,true);v.setUint32(8,256,true);v.setUint32(16,2,true);v.setUint32(24,7,true);v.setInt16(32,32767,true);v.setInt16(34,-32768,true);v.setInt16(36,8192,true);v.setInt16(38,-4096,true);
 const audio=core.decode(bytes,7);assert.equal(audio.rate,48000);assert.equal(audio.first,256);assert.equal(audio.left[0],32767/32768);assert.equal(audio.right[0],-1);assert.equal(audio.left[1],.25);assert.equal(audio.right[1],-.125);
 assert.throws(()=>core.decode(bytes,8));assert.throws(()=>core.decode(bytes.slice(0,36),7));v.setUint32(4,0,true);assert.throws(()=>core.decode(bytes,7));v.setUint32(4,48000,true);
 const meta={track:0,pattern:2,length:1536,barTicks:384,beatTicks:96,songLength:16},packet={positions:[768],patterns:[2],ticks:864,songBar:5,playing:true,songPlaying:false};
 let pos=core.position(packet,meta);assert.equal(pos.percent,50);assert.equal(pos.bar,3);assert.equal(pos.beat,1);assert.equal(pos.bars,4);
 packet.songPlaying=true;pos=core.position(packet,meta);assert.equal(pos.bar,6);assert.equal(pos.beat,2);assert.equal(pos.percent,32.8125);
 packet.songPlaying=false;packet.playing=false;pos=core.position(packet,meta);assert.equal(pos.playing,false);assert.equal(pos.percent,50);
 assert.equal(core.position({...packet,patterns:[3]},meta),null);assert.equal(core.position(packet,{...meta,length:0}),null);
 const sixEight={...meta,barTicks:288,beatTicks:48,length:576};pos=core.position({...packet,positions:[432]},sixEight);assert.equal(pos.bar,2);assert.equal(pos.beat,4);assert.equal(pos.percent,75);
 const metaAudio={...meta,tempo:1200};const anchor={...packet,playing:true,audioSample:1000,audioRate:48000,positions:[768]};
 let heard=core.aligned([anchor,{...anchor,audioSample:5000,positions:[1000]}],metaAudio,3400);assert.ok(Math.abs(heard.positions[0]-777.6)<.001);assert.equal(anchor.positions[0],768);assert.equal(core.aligned([anchor],metaAudio,999),null);assert.equal(core.aligned([anchor],metaAudio,20000),null);
 const fast=core.monitorWindow(0,24576,48000);assert.equal(fast.count,1440);assert.equal(fast.first,23136);assert.equal(fast.target,.035);assert.equal(fast.ceiling,.11);assert.equal(core.monitorWindow(0,24576,48000,'stable').count,3840);
 return 'PASS: independent stereo channels, session/rate/length validation, pattern/song progress, stopped positions and 6/8 timing';
}
if(require.main===module)console.log(run());module.exports=run;
