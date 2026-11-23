// SPDX-License-Identifier: AGPL-3.0-only
(async () => {
    const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
    class Element {
        constructor(){this.children=[];this.dataset={};this.attrs={};this.classList={toggle(){},add(){},remove(){}};this.value='';}
        append(...nodes){this.children.push(...nodes);} replaceChildren(){this.children=[];}
        setAttribute(k,v){this.attrs[k]=String(v);} removeAttribute(k){delete this.attrs[k];}
        querySelectorAll(){return this.children;}
        querySelector(selector){return this.children.find(b=>selector.includes('"'+b.dataset.index+'"'));}
    }
    const ids={};for(const id of ['expansion-builder','pack-banks','pack-bank-label','pack-pads','pack-selection','pack-status','pack-count','pack-files','pack-cover-file','pack-cover-remove','pack-clear','pack-preview','pack-mpc-pads','pack-mpc-status','pack-stop','pack-export','pack-save-unit','pack-name','pack-author'])ids['#'+id]=new Element();
    ids['#pack-name'].value='All banks';
    let exported,physicalHit;
    const buffer={length:4410,duration:.1,numberOfChannels:1};
    const sandbox={window:{AudioContext:class{async decodeAudioData(){return buffer;}},OfflineAudioContext:class{createBufferSource(){return{connect(){},start(){}};}async startRendering(){return buffer;}},WorkshopExpansion:{wav:()=>new Uint8Array([1,2,3,4]),expansion:(title,author,pads)=>{exported=pads.slice();return new Uint8Array([1]);}},WorkshopPadPlayer:class{stop(){}async play(){}},WorkshopMpcPads:class{constructor(hit){physicalHit=hit;}},WorkshopPackStorage:{save:async()=> 'Expansion Packs/All banks.xpn'},addEventListener(){}},document:{querySelector:s=>ids[s]||null,createElement:()=>new Element(),addEventListener(){}},setTimeout:()=>0,clearTimeout(){}};
    vm.runInNewContext(fs.readFileSync(require('node:path').join(__dirname,'../src/melodee/web/static/companion.js'),'utf8'),sandbox);
    const banks=ids['#pack-banks'].children;
    assert.deepEqual(banks.map(b=>b.textContent),['A','B','C','D','E','F','G','H']);
    const files=Array.from({length:18},(_,i)=>({name:'Sound'+i+'.wav',size:4,arrayBuffer:async()=>new ArrayBuffer(4)}));
    await ids['#pack-files'].onchange({target:{files,value:''}});
    assert.equal(ids['#pack-bank-label'].textContent,'B');
    assert.equal(ids['#pack-selection'].textContent,'Selected: B03');
    assert.equal(ids['#pack-count'].textContent,'18 / 128 assigned');
    banks[7].onclick();assert.equal(ids['#pack-bank-label'].textContent,'H');
    const last=ids['#pack-pads'].children.find(b=>b.dataset.index===127);
    await last.onclick({detail:0});assert.equal(ids['#pack-selection'].textContent,'Selected: H16');
    await ids['#pack-files'].onchange({target:{files:[{...files[0],name:'Last.wav'}],value:''}});
    assert.equal(ids['#pack-selection'].textContent,'Selected: A01');
    banks[1].onclick();physicalHit(15);assert.equal(ids['#pack-selection'].textContent,'Selected: B16');
    await ids['#pack-save-unit'].onclick();
    assert.equal(exported.length,128);assert.equal(exported[0].name,'Sound0.wav');assert.equal(exported[16].name,'Sound16.wav');assert.equal(exported[127].name,'Last.wav');assert.equal(exported[126],null);
    console.log('PASS: A–H bank navigation, bulk import across A/B, H16 wrap, physical pad routing to selected bank, assignments retained in export');
})();
