// SPDX-License-Identifier: AGPL-3.0-only
'use strict';
(()=>{
 const strip=document.querySelector('[data-browser-media]');if(!strip)return;
 const button=strip.querySelector('[data-monitor]'),status=strip.querySelector('[data-audio-status]'),volume=strip.querySelector('[data-monitor-volume]'),bar=strip.querySelector('progress'),clock=strip.querySelector('[data-transport-clock]'),mode=strip.querySelector('[data-monitor-mode]');
 let context=null,gain=null,id=0,on=false,starting=false,epoch=0,request=null,timer=0,origin=null,base=0,rate=0,drops=null,expected=null,lastAudio=0,lastTiming=0;
 const history=[];const sources=new Set();const core=window.WorkshopMediaCore;
 function clearSources(){for(const source of sources){try{source.stop();source.disconnect();}catch{}}sources.clear();origin=null;expected=null;}
 function remoteStop(session){if(session)fetch('/audio/stop?id='+session,{method:'POST',headers:{'X-Melodee':'1'},body:'',keepalive:true}).catch(()=>{});}
 async function stop(message='Audio off'){++epoch;on=false;clearTimeout(timer);request?.abort();request=null;const session=id;id=0;clearSources();const old=context;context=null;gain=null;remoteStop(session);if(old)try{await old.close();}catch{}button.textContent='Enable MPC audio';button.setAttribute('aria-pressed','false');status.textContent=message;}
 async function poll(token){if(!on||token!==epoch||document.hidden)return;
  const abort=new AbortController();request=abort;const timeout=setTimeout(()=>abort.abort(),2500);
  try{const response=await fetch('/audio?id='+id+'&live='+(mode?.value==='stable'?'stable':'1'),{cache:'no-store',signal:abort.signal});if(!response.ok)throw Error('Audio session ended. Enable MPC audio again.');const bytes=await response.arrayBuffer();if(!on||token!==epoch)return;const p=core.decode(bytes,id);
   if(context.state!=='running')throw Error('Browser audio paused. Enable MPC audio again.');
   if(p.count){const window=core.monitorWindow(p.first,p.count,p.rate,mode?.value||'fast');if(window.offset){p.left=p.left.subarray(window.offset);p.right=p.right.subarray(window.offset);p.first=window.first;p.count=window.count;}const now=context.currentTime;if(rate&&rate!==p.rate||drops!==null&&drops!==p.drops||expected!==null&&expected!==p.first)clearSources();rate=p.rate;drops=p.drops;
    if(origin===null){base=p.first;origin=now+window.target;}let at=origin+(p.first-base)/rate;
    if(at<now+.002||at>now+window.ceiling){clearSources();base=p.first;origin=now+window.target;at=origin;}
    if(sources.size>256)throw Error('Browser audio is falling behind. Enable audio again.');
    const buffer=context.createBuffer(2,p.count,rate);buffer.getChannelData(0).set(p.left);buffer.getChannelData(1).set(p.right);const source=context.createBufferSource();source.buffer=buffer;source.connect(gain);sources.add(source);source.onended=()=>{sources.delete(source);source.disconnect();};source.start(at);expected=p.first+p.count;lastAudio=performance.now();status.textContent=(mode?.value==='stable'?'Stable stereo · ':'Fast stereo · ')+Math.round(Math.max(0,at-now)*1000)+' ms buffered';
   }else if(performance.now()-lastAudio>5000)throw Error('No audio arriving. Enable MPC audio again.');
  }catch(error){if(on&&token===epoch)await stop(error.name==='AbortError'?'Audio connection timed out. Enable again.':error.message);}
  finally{clearTimeout(timeout);if(request===abort)request=null;if(on&&token===epoch)timer=setTimeout(()=>poll(token),12);}
 }
 async function start(){if(starting)return;starting=true;button.disabled=true;const token=++epoch;
  try{const Audio=window.AudioContext||window.webkitAudioContext;if(!Audio)throw Error('This browser cannot play MPC audio.');context=new Audio({latencyHint:'interactive'});await context.resume();if(token!==epoch)return;gain=context.createGain();gain.channelCount=2;gain.channelCountMode='explicit';gain.gain.value=+volume.value/100;gain.connect(context.destination);
   const abort=new AbortController();request=abort;const timeout=setTimeout(()=>abort.abort(),5000);let response;try{response=await fetch('/audio/start',{method:'POST',headers:{'X-Melodee':'1'},body:'',signal:abort.signal});}finally{clearTimeout(timeout);if(request===abort)request=null;}
   if(!response.ok)throw Error('Could not start MPC audio.');const result=await response.json();if(!Number.isInteger(result.id)||result.id<=0)throw Error('Invalid MPC audio session');if(token!==epoch){remoteStop(result.id);return;}id=result.id;on=true;rate=0;drops=null;expected=null;origin=null;lastAudio=performance.now();button.textContent='Stop MPC audio';button.setAttribute('aria-pressed','true');status.textContent='Connecting stereo monitor…';poll(token);
  }catch(error){if(token===epoch)await stop(error.name==='AbortError'?'Audio start timed out. Try again.':error.message);}finally{starting=false;button.disabled=false;}
 }
 const settings=strip.querySelector('[data-monitor-settings]');if(settings)settings.onclick=()=>{const open=strip.classList.toggle('browser-settings-open');settings.setAttribute('aria-expanded',open);};
 button.onclick=()=>on?stop():start();volume.oninput=()=>{if(gain&&context)gain.gain.setTargetAtTime(+volume.value/100,context.currentTime,.015);};
 function audibleSample(){if(!on||!context||origin===null||!rate)return null;let heard=context.currentTime-(context.outputLatency||context.baseLatency||0);if(context.getOutputTimestamp){const stamp=context.getOutputTimestamp();if(stamp.contextTime>0&&stamp.performanceTime>0&&Math.abs(performance.now()-stamp.performanceTime)<1000)heard=stamp.contextTime+(performance.now()-stamp.performanceTime)/1000;}return base+(heard-origin)*rate;}
 if(mode)mode.onchange=()=>{clearSources();if(on)status.textContent='Re-aligning monitor…';};
 window.WorkshopBrowserMedia={transport(packet,meta){
  if(Number.isFinite(packet?.audioSample)){if(history.at(-1)?.audioSample!==packet.audioSample)history.push(packet);if(history.length>32)history.shift();}
  const audible=audibleSample(),matched=audible===null?null:core.aligned(history,meta,audible),display=matched||packet,p=core.position(display,meta);if(!p)return display;lastTiming=performance.now();bar.value=p.percent;bar.setAttribute('aria-valuetext',(p.song?'Song':'Pattern')+' bar '+p.bar+' beat '+p.beat+' of '+p.bars+' bars');clock.textContent=(p.song?'Song':'Pattern')+' · '+(p.playing?'Playing':'Stopped')+' · Bar '+p.bar+' / '+p.bars+' · Beat '+p.beat+(on?(matched?' · audio aligned':' · syncing monitor'):'');return display;
 }};
 setInterval(()=>{if(lastTiming&&performance.now()-lastTiming>2500){clock.textContent='Timing unavailable · check MPC connection';bar.removeAttribute('value');lastTiming=0;}},1000);
 document.addEventListener('visibilitychange',()=>{if(document.hidden)stop('Audio stopped while page is hidden');});window.addEventListener('pagehide',()=>stop());
})();
