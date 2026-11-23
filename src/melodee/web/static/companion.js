// SPDX-License-Identifier: AGPL-3.0-only
'use strict';
(() => {
const S=window.WorkshopExpansion,$=s=>document.querySelector(s),colors=['#edb58e','#8ac9c8','#b3a5ec','#df99ac','#e3ca83','#8ab6df'];
function save(data,name,type='application/octet-stream'){let url=URL.createObjectURL(new Blob([data],{type})),a=document.createElement('a');a.href=url;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(url),60000);}
function safeName(name){return name.replace(/[^a-z0-9 _.-]/gi,'-').replace(/^\.+/,'').slice(0,60)||'Workshop';}
let audioContext;const context=()=>audioContext ||= new (window.AudioContext||window.webkitAudioContext)();
async function decode(file){if(file.size>100*1024*1024)throw Error('Each audio file must be smaller than 100 MB');return context().decodeAudioData(await file.arrayBuffer());}
function option(parent,value,label){let o=document.createElement('option');o.value=value;o.textContent=label;parent.append(o);}
function guard(fn,status){return async(...args)=>{try{await fn(...args);}catch(e){status(e.message||String(e));}};}
let rpcBusy=false;
async function rpc(action,extra={}){
 if(rpcBusy)throw Error('Wait for the MPC to finish the previous command');rpcBusy=true;
 let id=Date.now().toString(36)+'-'+Math.random().toString(36).slice(2);
 try{let r=await fetch('/controls/command',{method:'POST',headers:{'X-Melodee':'1','Content-Type':'application/json'},body:JSON.stringify({id,action,...extra})});if(!r.ok)throw Error((await r.json()).error||'MPC command unavailable. Install v82 or later.');
  let end=Date.now()+6000;while(Date.now()<end){await new Promise(r=>setTimeout(r,80));let response=await fetch('/controls/response',{cache:'no-store'});if(!response.ok)throw Error('MPC disconnected');let data=await response.json();if(data.id===id){if(data.error)throw Error(data.error);return data.state;}}throw Error('MPC did not confirm the command. Check the MPC before retrying');
 }finally{rpcBusy=false;}
}
window.WorkshopRemote={command:rpc};
const pack=$('#expansion-builder');if(pack){
 let pads=Array(128).fill(null),selected=0,cover=null;const status=s=>$('#pack-status').textContent=s;
 const preview=new window.WorkshopPadPlayer(context);
 const bankOf=()=>Math.floor(selected/16),padLabel=i=>String.fromCharCode(65+Math.floor(i/16))+String(i%16+1).padStart(2,'0');
 const banks=$('#pack-banks');
 for(let bank=0;bank<8;bank++){const button=document.createElement('button');button.type='button';button.textContent=String.fromCharCode(65+bank);button.setAttribute('aria-label','Bank '+button.textContent);button.onclick=()=>{preview.stop();selected=bank*16+selected%16;draw();};banks.append(button);}
 function choose(i){selected=i;$('#pack-pads').querySelectorAll('button').forEach(b=>b.setAttribute('aria-pressed',+b.dataset.index===i));$('#pack-selection').textContent='Selected: '+padLabel(i);}
 async function hit(i){choose(i);if(!pads[i])return;await preview.play(pads[i].buffer);const b=$('#pack-pads').querySelector('[data-index="'+i+'"]');if(b){b.classList.remove('playing');void b.offsetWidth;b.classList.add('playing');setTimeout(()=>b.classList.remove('playing'),120);}}
 function draw(){
  $('#pack-bank-label').textContent=String.fromCharCode(65+bankOf());banks.querySelectorAll('button').forEach((button,i)=>button.setAttribute('aria-pressed',i===bankOf()));
  const grid=$('#pack-pads');grid.replaceChildren();
  for(let row=3;row>=0;row--)for(let col=0;col<4;col++){
   const i=bankOf()*16+row*4+col,b=document.createElement('button'),number=document.createElement('span'),sound=document.createElement('span'),detail=document.createElement('small');
   b.type='button';b.setAttribute('aria-pressed',selected===i);b.classList.toggle('assigned',!!pads[i]);
   const pad=padLabel(i);b.setAttribute('aria-label',pad+(pads[i]?': '+pads[i].name:': empty'));
   number.className='pad-number';number.textContent=pad;sound.className='pad-sound';sound.textContent=pads[i]?.name||'Empty';sound.title=pads[i]?.name||'Assign a sample';detail.textContent=pads[i]?(pads[i].frames/44100).toFixed(2)+' s':'WAV';
   b.append(number,sound,detail);b.dataset.index=i;b.onpointerdown=guard(e=>{if(e.button!==0)return;e.preventDefault();return hit(i);},status);b.onclick=guard(e=>{if(e.detail===0)return hit(i);},status);grid.append(b);
  }
  const count=$('#pack-count'),selection=$('#pack-selection');if(count)count.textContent=pads.filter(Boolean).length+' / 128 assigned';if(selection)selection.textContent='Selected: '+padLabel(selected);
 }
 $('#pack-files').onchange=guard(async e=>{let input=[...e.target.files];if(input.length>128)throw Error('Choose at most 128 WAV files');let total=pads.reduce((n,p)=>n+(p?.bytes.length||0),0);for(let f of input){if(!/\.wav$/i.test(f.name))throw Error('Choose WAV samples');let decoded=await decode(f);if(decoded.duration>120)throw Error('Each pad sample is limited to two minutes');let Offline=window.OfflineAudioContext||window.webkitOfflineAudioContext,ctx=new Offline(Math.min(2,decoded.numberOfChannels),Math.ceil(decoded.duration*44100),44100),source=ctx.createBufferSource();source.buffer=decoded;source.connect(ctx.destination);source.start();let buffer=await ctx.startRendering(),bytes=S.wav(buffer);total-=pads[selected]?.bytes.length||0;total+=bytes.length;if(total>250*1024*1024)throw Error('Expansion limit: 250 MB');pads[selected]={name:f.name,bytes,frames:buffer.length,buffer};selected=(selected+1)%128;}draw();status('Samples assigned. Export your expansion when ready.');e.target.value='';},status);
 $('#pack-cover-file').onchange=async event=>{
     const words=$('#pack-cover-status');
     try {
         const image=await window.WorkshopPictures.picture(event.target.files[0]);
         const canvas=document.createElement('canvas');canvas.width=canvas.height=768;
         const paint=canvas.getContext('2d'),edge=Math.min(image.naturalWidth,image.naturalHeight);
         paint.drawImage(image,(image.naturalWidth-edge)/2,(image.naturalHeight-edge)/2,edge,edge,0,0,768,768);
         const blob=await new Promise(resolve=>canvas.toBlob(resolve,'image/jpeg',.9));
         if(!blob)throw Error('Could not prepare the cover photo');
         cover=new Uint8Array(await blob.arrayBuffer());
         $('#pack-cover').src=canvas.toDataURL('image/jpeg',.9);$('#pack-cover').hidden=false;
         words.textContent='Cover added. Artwork.jpg and its metadata will be included in your expansion.';
     } catch(error){words.textContent=error.message;}
     event.target.value='';
 };
 $('#pack-cover-remove').onclick=()=>{cover=null;$('#pack-cover').hidden=true;$('#pack-cover').removeAttribute('src');$('#pack-cover-status').textContent='Cover removed.';};
 $('#pack-clear').onclick=()=>{pads[selected]=null;draw();};$('#pack-preview').onclick=guard(async()=>{if(!pads[selected])throw Error('Assign a sample to this pad');await hit(selected);},status);
 const mpcButton=$('#pack-mpc-pads'),mpcStatus=$('#pack-mpc-status');
 const mpcPads=new window.WorkshopMpcPads(i=>hit(bankOf()*16+i).catch(error=>mpcStatus.textContent=error.message),message=>mpcStatus.textContent=message);
 mpcButton.onclick=guard(async()=>{if(mpcPads.enabled){mpcPads.stop();mpcButton.textContent='Enable MPC pads';mpcButton.setAttribute('aria-pressed','false');mpcStatus.textContent='MPC pad preview off';}else{await context().resume();mpcPads.start();mpcButton.textContent='Disable MPC pads';mpcButton.setAttribute('aria-pressed','true');}},message=>mpcStatus.textContent=message);
 $('#pack-stop').onclick=()=>preview.stop();
 document.addEventListener('visibilitychange',()=>{if(document.hidden)preview.stop();});
 window.addEventListener('pagehide',()=>{mpcPads.stop();preview.stop();});
 function expansion(){const title=$('#pack-name').value.trim(),author=$('#pack-author').value.trim();return {bytes:S.expansion(title,author,pads,cover),name:safeName(title)+'.xpn'};}
 $('#pack-export').onclick=guard(()=>{const pack=expansion();save(pack.bytes,pack.name,'application/zip');status('Expansion downloaded to this phone or computer. You can also save a copy directly to the MPC.');},status);
 $('#pack-save-unit').onclick=guard(async()=>{
  const button=$('#pack-save-unit');button.disabled=true;
  try{const pack=expansion();status('Preparing to save to the MPC…');const path=await window.WorkshopPackStorage.save(pack.bytes,pack.name,percent=>status(percent===100?'Finishing save on the MPC…':'Saving to the MPC: '+percent+'%'));status('Saved on the MPC SD card: '+path+'. In EXPANSIONS, choose Install Pack and open Expansion Packs.');}
  finally{button.disabled=false;}
 },status);draw();
}
// Tutorial controls are momentary, with a device-side release timeout if the browser disconnects.
if($('#device-screen')){
 let message=$('#tutorial-action');document.querySelectorAll('[data-button],[data-pad]').forEach(el=>{el.style.cursor='pointer';el.setAttribute('tabindex','0');el.setAttribute('role','button');let run=guard(async()=>{let pad=el.dataset.pad;await rpc(pad!==undefined?'pad':'button',{index:+(pad??el.dataset.button)});el.classList.add('active');setTimeout(()=>el.classList.remove('active'),180);},s=>message.textContent=s);el.addEventListener('click',run);el.addEventListener('keydown',e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();run();}});});
 document.querySelectorAll('[data-control]').forEach(el=>{let index=+el.dataset.control,value=64,drag=null;el.style.touchAction='none';let send=()=>rpc('control',{index,value,delta:value>=64?1:-1}).catch(e=>message.textContent=e.message);el.addEventListener('pointerdown',e=>{drag={y:e.clientY,value:+el.dataset.value>=0?+el.dataset.value:64};el.setPointerCapture(e.pointerId);});el.addEventListener('pointermove',e=>{if(!drag)return;value=Math.max(0,Math.min(127,Math.round(drag.value+(drag.y-e.clientY))));if(!rpcBusy)send();});el.addEventListener('pointerup',()=>{drag=null;if(!rpcBusy)send();});el.addEventListener('pointercancel',()=>drag=null);el.style.cursor='ns-resize';el.addEventListener('wheel',e=>{e.preventDefault();value=Math.max(0,Math.min(127,value+(e.deltaY<0?2:-2)));rpc('control',{index,value,delta:e.deltaY<0?1:-1}).catch(e=>message.textContent=e.message);},{passive:false});});
}
})();
