// SPDX-License-Identifier: AGPL-3.0-only
// A lossless tagged-file adapter: untouched Melodee sections remain byte-for-byte intact.
(function(root){
'use strict';
const enc=new TextEncoder(), dec=new TextDecoder();
class Reader {
 constructor(bytes){this.b=bytes;this.v=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength);this.p=0;}
 need(n){if(this.p+n>this.b.length)throw Error('Truncated Melodee project');}
 u8(){this.need(1);return this.b[this.p++];} u16(){this.need(2);let n=this.v.getUint16(this.p,true);this.p+=2;return n;}
 u32(){this.need(4);let n=this.v.getUint32(this.p,true);this.p+=4;return n;} i16(){let n=this.u16();return n>32767?n-65536:n;}
 text(){let n=this.u8();this.need(n);let s=dec.decode(this.b.slice(this.p,this.p+n));this.p+=n;return s;}
}
class Writer {
 constructor(){this.b=[];} u8(n){this.b.push(n&255);return this;} u16(n){return this.u8(n).u8(n>>8);} u32(n){return this.u16(n).u16(n>>>16);}
 bytes(a){for(let b of a)this.b.push(b);return this;} text(s){let b=enc.encode(s);return this.u8(b.length).bytes(b);} out(){return Uint8Array.from(this.b);}
}
function join(parts){let out=new Uint8Array(parts.reduce((n,p)=>n+p.length,0)),at=0;for(let p of parts){out.set(p,at);at+=p.length;}return out;}
function section(tag,b){return join([enc.encode(tag),new Writer().u32(b.length).out(),b]);}
function crc32(bytes){let c=0xffffffff;for(let b of bytes){c^=b;for(let k=0;k<8;k++)c=(c>>>1)^((c&1)?0xedb88320:0);}return (c^0xffffffff)>>>0;}
function zip(entries){
 let parts=[],central=[],offset=0,total=0;for(let {name,data} of entries){if(!name||name.startsWith('/')||name.split('/').some(p=>!p||p==='.'||p==='..')||name.includes('\\'))throw Error('Unsafe archive path');data=typeof data==='string'?enc.encode(data):data;total+=data.length;if(total>268435456)throw Error('Pack limit: 256 MB');let n=enc.encode(name),crc=crc32(data),h=new Writer().u32(0x04034b50).u16(20).u16(0x800).u16(0).u16(0).u16(33).u32(crc).u32(data.length).u32(data.length).u16(n.length).u16(0).bytes(n).out();parts.push(h,data);central.push(new Writer().u32(0x02014b50).u16(20).u16(20).u16(0x800).u16(0).u16(0).u16(33).u32(crc).u32(data.length).u32(data.length).u16(n.length).u16(0).u16(0).u16(0).u16(0).u32(0).u32(offset).bytes(n).out());offset+=h.length+data.length;}
 let index=join(central);return join([...parts,index,new Writer().u32(0x06054b50).u16(0).u16(0).u16(entries.length).u16(entries.length).u32(index.length).u32(offset).u16(0).out()]);
}
const xml=s=>String(s).replace(/[<>&"']/g,c=>({'<':'&lt;','>':'&gt;','&':'&amp;','"':'&quot;',"'":'&apos;'})[c]);
function expansion(title,author,pads,cover=null){
 if(pads.length>128)throw Error('Expansion supports banks A–H: 128 pads maximum');
 if(!title.trim()||!pads.some(Boolean))throw Error('Name your pack and assign at least one WAV');
 let files=[],instruments=[];
 pads.forEach((pad,i)=>{if(!pad)return;let name='Pad-'+String(i+1).padStart(2,'0')+'.wav';files.push({name:'Samples/'+name,data:pad.bytes});instruments.push('<Instrument number="'+(i+1)+'"><Volume>0.7079</Volume><Tune>0</Tune><Layers><Layer number="1"><SampleName>Samples/'+name.slice(0,-4)+'</SampleName><SampleFile>Samples/'+name+'</SampleFile><VelStart>0</VelStart><VelEnd>127</VelEnd><Volume>1</Volume><Tune>0</Tune><SampleStart>0</SampleStart><SampleEnd>'+pad.frames+'</SampleEnd></Layer></Layers></Instrument>');});
 if(cover){files.push({name:'Artwork.jpg',data:cover});files.push({name:'TapeWorkshopExpansion.json',data:JSON.stringify({format:'TapeWorkshopExpansion',version:1,title,author,artwork:'Artwork.jpg',artworkType:'image/jpeg',artworkWidth:768,artworkHeight:768})});}
 files.push({name:'Expansion.xml',data:'<?xml version="1.0" encoding="UTF-8"?><expansion><title>'+xml(title)+'</title><manufacturer>'+xml(author)+'</manufacturer>'+(cover?'<image>Artwork.jpg</image>':'')+'<version>1.0</version><type>drum</type></expansion>'});
 files.push({name:'Kit.xpm',data:'<?xml version="1.0" encoding="UTF-8"?><MPCVObject><Version><File_Version>2.1</File_Version></Version><Program type="Drum"><ProgramName>'+xml(title)+'</ProgramName><Instruments>'+instruments.join('')+'</Instruments></Program></MPCVObject>'});
 files.push({name:'README.txt',data:title+'\nBy '+author+'\n\nOpen this .xpn in Sample Select to unpack it, then load Kit.xpm. Supports Melodee and Tape Workshop.\nBuilt with Tape Workshop Expansion Builder, based on Melodee by Kerem Kiliç (Ellic Studio).\n'});return zip(files);
}
function wav(buffer){let frames=buffer.length,channels=Math.min(2,buffer.numberOfChannels),rate=buffer.sampleRate,b=new Uint8Array(44+frames*channels*2),v=new DataView(b.buffer);b.set(enc.encode('RIFF'));v.setUint32(4,b.length-8,true);b.set(enc.encode('WAVEfmt '),8);v.setUint32(16,16,true);v.setUint16(20,1,true);v.setUint16(22,channels,true);v.setUint32(24,rate,true);v.setUint32(28,rate*channels*2,true);v.setUint16(32,channels*2,true);v.setUint16(34,16,true);b.set(enc.encode('data'),36);v.setUint32(40,b.length-44,true);for(let c=0;c<channels;c++){let data=buffer.getChannelData(c);for(let i=0;i<frames;i++){let x=Math.max(-1,Math.min(1,data[i]));v.setInt16(44+(i*channels+c)*2,Math.round(x*(x<0?32768:32767)),true);}}return b;}
const api={zip,expansion,wav};if(typeof module!=='undefined')module.exports=api;root.WorkshopExpansion=api;
})(typeof window==='undefined'?globalThis:window);
