// SPDX-License-Identifier: AGPL-3.0-only
(function(root){
 'use strict';
 function decode(bytes,session){
  if(!(bytes instanceof ArrayBuffer)||bytes.byteLength<32)throw Error('Invalid MPC audio packet');const view=new DataView(bytes);
  if(view.getUint32(0,false)!==0x54574143)throw Error('Invalid MPC audio packet');
  const rate=view.getUint32(4,true),first=view.getUint32(8,true)+view.getUint32(12,true)*4294967296,count=view.getUint32(16,true),drops=view.getUint32(20,true),id=view.getUint32(24,true);
  if(id!==session||count>24576||bytes.byteLength!==32+count*4||count&&(rate<8000||rate>192000)||!Number.isSafeInteger(first))throw Error('Invalid or expired MPC audio packet');
  const left=new Float32Array(count),right=new Float32Array(count);for(let i=0;i<count;++i){left[i]=view.getInt16(32+i*4,true)/32768;right[i]=view.getInt16(34+i*4,true)/32768;}
  return{rate,first,count,drops,left,right};
 }
 function position(packet,meta){
  if(!packet||!meta||!Array.isArray(packet.positions)||!Array.isArray(packet.patterns))return null;
  const t=meta.track,barTicks=meta.barTicks,beatTicks=meta.beatTicks;if(!Number.isInteger(t)||t<0||t>15||!(barTicks>0)||!(beatTicks>0))return null;
  const song=!!packet.songPlaying,raw=song?packet.songBar*barTicks+packet.ticks%barTicks:packet.positions[t],length=song?meta.songLength*barTicks:meta.length;
  if(!Number.isFinite(raw)||raw<0||!(length>0)||!song&&packet.patterns[t]!==meta.pattern)return null;
  const tick=Math.max(0,Math.min(raw,length));return{percent:Math.min(100,tick/length*100),bar:Math.floor(tick/barTicks)+1,beat:Math.floor(tick%barTicks/beatTicks)+1,bars:Math.ceil(length/barTicks),playing:!!packet.playing,song};
 }
 function aligned(history,meta,audibleSample){
  if(!meta||!Number.isFinite(audibleSample))return null;let anchor=null;for(const item of history)if(Number.isFinite(item.audioSample)&&item.audioSample<=audibleSample&&(!anchor||item.audioSample>anchor.audioSample))anchor=item;
  if(!anchor||!(anchor.audioRate>0))return null;const seconds=(audibleSample-anchor.audioSample)/anchor.audioRate;if(seconds>.25)return null;
  const out={...anchor,positions:anchor.positions.slice(),patterns:anchor.patterns.slice()};if(!out.playing)return out;
  const delta=seconds*(meta.tempo/10)*96/60;if(!Number.isFinite(delta))return null;
  if(out.songPlaying){out.songBar+=Math.floor((out.ticks%meta.barTicks+delta)/meta.barTicks);out.ticks+=delta;}
  else{out.positions[meta.track]=(out.positions[meta.track]+delta)%meta.length;out.ticks+=delta;}
  return out;
 }
 function monitorWindow(first,count,rate,mode='fast'){const keep=Math.min(count,Math.ceil(rate*(mode==='stable'?.08:.03))),offset=count-keep;return{first:first+offset,count:keep,offset,target:mode==='stable'?.10:.035,ceiling:mode==='stable'?.22:.11};}
 const api={decode,position,aligned,monitorWindow};if(typeof module!=='undefined'&&module.exports)module.exports=api;else root.WorkshopMediaCore=api;
})(typeof window!=='undefined'?window:this);
