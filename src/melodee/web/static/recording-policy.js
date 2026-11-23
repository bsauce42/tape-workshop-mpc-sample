// SPDX-License-Identifier: AGPL-3.0-only
(function(root){
    'use strict';
    const maxMilliseconds=2*60*60*1000;
    const maxBytes=4*1024*1024*1024-64*1024*1024;
    function limit(elapsed,bytes,disk=true){
        if(!disk){if(bytes>=768*1024*1024)return "Short-mode recording reached 768 MB.";if(elapsed>=20*60*1000)return "Short-mode twenty-minute limit reached.";return "";}
        if(bytes>=maxBytes)return 'Recording reached the 3.94 GiB file-size limit.';
        if(elapsed>=maxMilliseconds)return 'Two-hour take limit reached.';
        return '';
    }
    function clock(milliseconds){
        const seconds=Math.floor(Math.max(0,milliseconds)/1000);
        const tail=String(Math.floor(seconds/60)%60).padStart(2,'0')+':'+String(seconds%60).padStart(2,'0');
        return seconds>=3600?Math.floor(seconds/3600)+':'+tail:tail;
    }
    function createClock(start){
  let held=null,excluded=0;
  return {
   pause(now){if(held===null)held=now;},
   resume(now){if(held!==null){excluded+=now-held;held=null;}},
   elapsed(now){return Math.max(0,(held===null?now:held)-start-excluded);}
  };
 }
 const api={createClock,maxMilliseconds,maxBytes,limit,clock};
    if(typeof module!=='undefined')module.exports=api;
    root.WorkshopRecordingPolicy=api;
})(typeof window==='undefined'?globalThis:window);
