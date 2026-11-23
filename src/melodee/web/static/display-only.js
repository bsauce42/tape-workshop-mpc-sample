// SPDX-License-Identifier: AGPL-3.0-only
(() => {
  const page=document.getElementById('display-only');
  if(!page)return;
  const canvas=document.getElementById('display-canvas'),ctx=canvas.getContext('2d');
  const status=document.getElementById('display-status'),button=document.getElementById('display-fullscreen');
  let timer,request,controlsTimer,closed=false,stamp='';
  function showControls(){
    clearTimeout(controlsTimer);page.classList.add('controls-visible');
    if(page.classList.contains('is-fullscreen'))controlsTimer=setTimeout(()=>page.classList.remove('controls-visible'),1800);
  }
  page.addEventListener('pointermove',showControls);
  page.addEventListener('pointerdown',showControls);
  page.addEventListener('focusin',showControls);
  function expanded(on){page.classList.toggle('is-fullscreen',on);document.body.classList.toggle('display-expanded',on);button.textContent=on?'Exit full screen':'Full screen';button.setAttribute('aria-pressed',String(on));showControls();}
  button.addEventListener('click',async()=>{
    if(page.classList.contains('is-fullscreen')){expanded(false);if(document.fullscreenElement)await document.exitFullscreen().catch(()=>{});}
    else {expanded(true);try{await page.requestFullscreen?.();}catch{} }
  });
  document.addEventListener('fullscreenchange',()=>{if(!document.fullscreenElement)expanded(false);});
  document.addEventListener('keydown',e=>{if(e.key==='Escape')expanded(false);});
  async function refresh(){
    if(closed||document.hidden||request)return;
    request=new AbortController();const timeout=setTimeout(()=>request?.abort(),5000);
    let picture,url;
    try{
      const response=await fetch('/screen-frame',{cache:'no-store',signal:request.signal});
      if(!response.ok)throw Error('Waiting for MPC screen…');
      const bytes=await response.arrayBuffer(),view=new DataView(bytes);
      const format=bytes.byteLength>=16?String.fromCharCode(...new Uint8Array(bytes,0,4)):'';
      if(!['TWF2','TWRF','TWVF'].includes(format)||(format==='TWF2'&&bytes.byteLength<24))throw Error('Waiting for valid screen…');
      const next=format==='TWF2'?'s'+view.getUint32(20,true):view.getUint32(4,true)+':'+view.getUint32(8,true)+':'+view.getUint32(12,true);
      status.textContent='Live · MPC display';
      if(next===stamp)return;
      if(format==='TWVF'){
        const blob=new Blob([bytes.slice(16)],{type:'image/png'});
        if(window.createImageBitmap)picture=await createImageBitmap(blob);
        else {url=URL.createObjectURL(blob);picture=new Image();await new Promise((resolve,reject)=>{picture.onload=resolve;picture.onerror=reject;picture.src=url;});}
        if(!closed)ctx.drawImage(picture,0,0,320,240);
      }else{
        const offset=format==='TWF2'?24:16,encoding=format==='TWF2'?view.getUint32(16,true):0;
        const pixels=ctx.createImageData(320,240);let position=0;
        function put(color,count){
          if(!count||position+count>320*240)throw Error('Invalid screen run');
          const r=color>>>11,g=(color>>>5)&63,b=color&31;
          for(let n=0;n<count;n++){const at=position++*4;pixels.data[at]=(r<<3)|(r>>>2);pixels.data[at+1]=(g<<2)|(g>>>4);pixels.data[at+2]=(b<<3)|(b>>>2);pixels.data[at+3]=255;}
        }
        if(encoding===0){if(bytes.byteLength!==offset+320*240*2)throw Error('Incomplete screen');for(let at=offset;at<bytes.byteLength;at+=2)put(view.getUint16(at,true),1);}
        else if(encoding===1){if((bytes.byteLength-offset)%4)throw Error('Incomplete screen runs');for(let at=offset;at<bytes.byteLength;at+=4)put(view.getUint16(at+2,true),view.getUint16(at,true));}
        else throw Error('Unknown screen encoding');
        if(position!==320*240)throw Error('Incomplete screen');
        if(!closed)ctx.putImageData(pixels,0,0);
      }
      stamp=next;
    }catch(e){if(!closed)status.textContent=e.name==='AbortError'?'Reconnecting to MPC…':e.message;}
    finally{picture?.close?.();if(url)URL.revokeObjectURL(url);clearTimeout(timeout);request=null;if(!closed&&!document.hidden)timer=setTimeout(refresh,33);}
  }
  document.addEventListener('visibilitychange',()=>{clearTimeout(timer);if(document.hidden)request?.abort();else refresh();});
  window.addEventListener('pagehide',()=>{closed=true;clearTimeout(timer);clearTimeout(controlsTimer);request?.abort();});
  window.addEventListener('pageshow',()=>{if(closed){closed=false;refresh();}});
  refresh();
})();
