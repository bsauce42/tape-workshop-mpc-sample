// SPDX-License-Identifier: AGPL-3.0-only
"use strict";
(() => {
  const rules=window.TapeWorkshopReleases, panel=document.querySelector("#online-update");
  const banner=document.querySelector("#update-alert"), text=document.querySelector("#online-update-state");
  const check=document.querySelector("#online-update-check"), send=document.querySelector("#online-update-send");
  const download=document.querySelector("#online-update-zip"), auto=document.querySelector("#online-update-auto");
  const install=document.querySelector("#online-update-install"), autoInstall=document.querySelector("#online-update-auto-install");
  let installSending=false;
  let facts=null, selected=null, inFlight=false, downloading=false, tried=0, connected=false;
  function say(words) { if(text) text.textContent=words; }
  function cacheGet() { try { return JSON.parse(localStorage.getItem("tape-workshop-release-cache") || "null"); } catch { return null; } }
  function cacheSet(value) { try { localStorage.setItem("tape-workshop-release-cache",JSON.stringify(value)); } catch {} }
  if(auto) { try { auto.checked=localStorage.getItem("tape-workshop-auto-download")==="yes"; } catch {} }
  if(autoInstall) { try { autoInstall.checked=localStorage.getItem("tape-workshop-auto-install")==="yes"; } catch {} }
  function ready() {
    if(send) send.disabled=downloading || !connected || !facts?.card || !selected?.api || selected.order<=0 || facts?.offer===selected.version || sending || updatePanel?.busy;
    if(install) { install.disabled=installSending || !connected || !facts?.offer || ["saving","checking","writing","starting"].includes(facts.phase); install.textContent=facts?.install_queued?"Cancel queued installation":"Install downloaded update"; }
  }
  async function requestInstall(cancel=false) {
    if(installSending || !connected || (!cancel && !facts?.offer)) return;
    installSending=true; ready();
    try {
      const version=cancel?"":facts.offer;
      const response=await fetch("/update/install?version="+encodeURIComponent(version),{method:"POST",headers:{"X-Melodee":"1"},body:""});
      const result=await response.json();
      if(!response.ok) throw new Error(result.error || "The update could not be queued");
      facts.install_queued=!cancel;
      say(cancel?"Queued installation cancelled.":"Installation queued. It starts when playback stops and the MPC is ready. Save a new project first.");
    } catch(error) { say(error.message); }
    finally { installSending=false; ready(); }
  }
  function show(release) {
    selected=rules.offer(release,facts.version);
    if(!selected) { banner.hidden=true; say("This release cannot be compared with your Tape Workshop build. View GitHub releases or choose a signed ZIP."); ready(); return; }
    banner.hidden=selected.order<=0;
    if(!banner.hidden) banner.querySelector("a").textContent="New update available · "+selected.version+" →";
    if(download) { download.href=selected.download || selected.releaseUrl; download.hidden=selected.order<=0; download.textContent=selected.api?"Download ZIP":"View release"; }
    if(selected.order>0 && facts.offer===selected.version) say("Already downloaded. Stop playback, then press INSTALL on the MPC.");
    else if(selected.order<0) say("Your MPC is newer than the latest GitHub release ("+selected.version+").");
    else if(selected.order===0) say("You’re up to date · "+facts.version);
    else if(!selected.api) say("New release: "+selected.version+". Its signed update ZIP has not been published yet.");
    else say("New update available: "+selected.version+". Download to the MPC or choose a ZIP below.");
    ready();
    if(auto?.checked && selected.order>0 && selected.api && facts.offer!==selected.version && connected && facts.card && !sending && !updatePanel?.busy) transfer();
  }
  async function query(force=false) {
    if(inFlight || downloading || !connected || !facts) return;
    const cached=cacheGet();
    if(!force && cached && Date.now()-cached.at<600000) { show(cached.release); return; }
    inFlight=true; tried=Date.now(); if(check) check.disabled=true; say("Checking your GitHub releases…");
    const stop=new AbortController(), timeout=setTimeout(()=>stop.abort(),15000);
    try {
      const response=await fetch("https://api.github.com/repos/"+rules.repository+"/releases/latest",{headers:{Accept:"application/vnd.github+json"},signal:stop.signal,cache:"no-store"});
      if(response.status===404) { say("No GitHub release is published yet. Manual signed ZIP installation is available below."); banner.hidden=true; return; }
      if(!response.ok) throw new Error(response.status===403 || response.status===429 ? "GitHub’s request limit was reached. Try later or choose a ZIP." : "GitHub could not be reached. Use manual ZIP installation below.");
      const release=await response.json(); cacheSet({at:Date.now(),release}); show(release);
    } catch(error) { say(error.name==="AbortError" ? "GitHub timed out. Check internet access or choose a ZIP." : error.message || "Connect your phone or computer to the internet, or choose a ZIP below."); }
    finally { clearTimeout(timeout); inFlight=false; if(check) check.disabled=false; ready(); }
  }
  async function receive(url) {
    const stop=new AbortController(), timeout=setTimeout(()=>stop.abort(),180000);
    try {
      const response=await fetch(url,{headers:{Accept:"application/octet-stream"},signal:stop.signal,cache:"no-store"});
      if(!response.ok || (response.headers.get("Content-Type") || "").includes("application/json")) throw new Error("No downloadable ZIP at this address");
      if(!response.body?.getReader) {
        const blob=await response.blob(); if(blob.size!==selected.size) throw new Error("The ZIP size did not match the release"); return blob;
      }
      const reader=response.body.getReader(), parts=[]; let bytes=0;
      for(;;) {
        const chunk=await reader.read(); if(chunk.done) break;
        bytes+=chunk.value.byteLength;
        if(bytes>selected.size || bytes>268435456) { await reader.cancel(); throw new Error("The ZIP size did not match the release"); }
        parts.push(chunk.value); say("Downloading "+selected.version+" · "+Math.min(100,Math.round(bytes/selected.size*100))+"%");
      }
      if(bytes!==selected.size) throw new Error("The download stopped before the complete ZIP arrived");
      return new Blob(parts,{type:"application/zip"});
    } finally { clearTimeout(timeout); }
  }
  async function transfer() {
    if(!panel || downloading || !selected?.api || facts?.offer===selected.version || !facts?.card || sending || updatePanel?.busy || pluginPanel?.busy) return;
    downloading=true; sending=true; if(check)check.disabled=true; panels.forEach(p=>p.ready()); ready();
    try {
      let blob;
      try { blob=await receive(selected.api); }
      catch { blob=await receive(selected.mirror); }
      say("Downloaded. Sending the signed ZIP to your MPC…");
      sending=false; updatePanel.pick(new File([blob],selected.name,{type:"application/zip"}));
      updatePanel.send(result=> {
        update.sent(result);
        say(result?.offered ? "Ready. Stop playback, then press INSTALL on the MPC to save your project and install "+result.offered+"." : result?.error || "The MPC did not accept the update.");
        downloading=false; if(check)check.disabled=false; panels.forEach(p=>p.ready()); ready();
      });
    } catch {
      downloading=false; sending=false; if(check)check.disabled=false; panels.forEach(p=>p.ready()); ready();
      say("Automatic transfer is unavailable. Tap Download ZIP, then choose that signed ZIP in Manual install below. The GitHub update mirror may not be published yet.");
    }
  }
  check?.addEventListener("click",()=>query(true)); send?.addEventListener("click",transfer);
  auto?.addEventListener("change",()=> { try { localStorage.setItem("tape-workshop-auto-download",auto.checked?"yes":"no"); } catch {} if(auto.checked) { ready(); transfer(); } });
  install?.addEventListener("click",()=> {
    if(facts?.install_queued) {
      if(autoInstall) { autoInstall.checked=false; try { localStorage.setItem("tape-workshop-auto-install","no"); } catch {} }
      requestInstall(true);
    } else requestInstall();
  });
  autoInstall?.addEventListener("change",()=> {
    try { localStorage.setItem("tape-workshop-auto-install",autoInstall.checked?"yes":"no"); } catch {}
    if(autoInstall.checked && facts?.offer) requestInstall();
    else if(!autoInstall.checked && facts?.install_queued) requestInstall(true);
  });
  window.TapeWorkshopUpdates={
    connected(current) {
      const reconnect=!connected, changed=facts?.version!==current.version;
      connected=true; facts=current; ready();
      if(autoInstall?.checked && facts.offer && !facts.install_queued && facts.phase==="none" && !installSending) requestInstall();
      if(reconnect || changed) query();
    },
    offline() { connected=false; ready(); }
  };
  window.addEventListener("online",()=> { if(Date.now()-tried>30000)query(); });
  if(typeof last!=="undefined" && last) window.TapeWorkshopUpdates.connected(last);
})();
