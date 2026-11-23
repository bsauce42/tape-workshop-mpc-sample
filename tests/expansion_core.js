// SPDX-License-Identifier: AGPL-3.0-only
const expansionAPI=typeof require==='function'?require('../src/melodee/web/static/expansion-core.js'):module.exports;
function assert(value,words){if(!value)throw Error(words);}
function contents(bytes){
    const view=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength), files={};let at=0;
    while(view.getUint32(at,true)===0x04034b50){
        const size=view.getUint32(at+18,true),nameSize=view.getUint16(at+26,true),extra=view.getUint16(at+28,true);
        const name=new TextDecoder().decode(bytes.slice(at+30,at+30+nameSize)),start=at+30+nameSize+extra;
        files[name]=bytes.slice(start,start+size);at=start+size;
    }
    return files;
}
const pad={bytes:new Uint8Array([82,73,70,70]),frames:10},cover=new Uint8Array([255,216,255,217]);
const withCover=contents(expansionAPI.expansion('Bridge & drums','Maker',[pad],cover));
assert(withCover['Artwork.jpg'].length===cover.length,'Cover bytes must be embedded');
const meta=JSON.parse(new TextDecoder().decode(withCover['TapeWorkshopExpansion.json']));
assert(meta.artwork==='Artwork.jpg'&&meta.artworkWidth===768&&meta.version===1,'Future gallery metadata must name the cover');
const xml=new TextDecoder().decode(withCover['Expansion.xml']);
assert(xml.includes('Bridge &amp; drums')&&xml.includes('<image>Artwork.jpg</image>'),'Expansion XML must escape text and reference artwork');
const plain=contents(expansionAPI.expansion('Plain','Maker',[pad]));
assert(!plain['Artwork.jpg']&&!plain['TapeWorkshopExpansion.json'],'No artwork after removing cover');
let rejected=false;try{expansionAPI.zip([{name:'../bad',data:cover}]);}catch{rejected=true;}assert(rejected,'Unsafe ZIP paths must be rejected');

const banks=Array(128).fill(null);
for(const i of [0,15,16,31,112,127])banks[i]={bytes:new Uint8Array([i]),frames:i+1};
const allBanks=contents(expansionAPI.expansion('Eight banks','Maker',banks,cover));
const program=new TextDecoder().decode(allBanks['Kit.xpm']);
for(const i of [0,15,16,31,112,127]){
 const name='Pad-'+String(i+1).padStart(2,'0')+'.wav';
 assert(allBanks['Samples/'+name][0]===i,'Bank samples keep their absolute pad index');
 assert(program.includes('<Instrument number="'+(i+1)+'">'),'Instrument maps correctly across all eight banks');
}
assert((program.match(/<Instrument number=/g)||[]).length===6,'Empty pads remain unassigned');
let oversized=false;try{expansionAPI.expansion('Too many','Maker',Array(129).fill(pad));}catch{oversized=true;}
assert(oversized,'A ninth bank must be rejected');
console.log('PASS: expansion export through H16, bank boundaries, artwork and empty-pad preservation');
