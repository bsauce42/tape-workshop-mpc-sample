// Melodee's web page: the link to the MPC in the header on every page, the
// device page's update and plug-in uploads, and the docs' side lists.
"use strict";
const $ = (s, root = document) => root.querySelector(s);
const $$ = (s, root = document) => [...root.querySelectorAll(s)];
const tones = ["var(--coral)", "var(--amber)", "var(--mint)", "var(--lilac)"];

// The MPC answers one request at a time: no status asks while a file is on its way, and one at a time.
let sending = false, asking = false, last = null;

function link(state, word) {
  const el = $("#link");
  el.dataset.state = state;
  $("#link-word").textContent = word;
}

let statusBusy=false,statusFailures=0;
async function status() {
  if(statusBusy)return last;
  statusBusy=true;
  const request=new AbortController(),timeout=setTimeout(()=>request.abort(),3000);
  try {
    const response=await fetch("/status", {cache: "no-store",signal:request.signal});
    if(!response.ok)throw new Error('Status unavailable');
    const s = await response.json();statusFailures=0;
    link(update.watching ? "busy" : "on", "MPC " + s.version);
    device.show(s);
    last = s;
    window.TapeWorkshopUpdates?.connected(s);
    return s;
  } catch (e) {
    statusFailures++;
    link(update.watching === "install" || statusFailures<3 ? "busy" : "off", update.watching === "install" ? "Restarting" : statusFailures<3 ? "Connecting…" : "Connection delayed · retrying…");
    device.lost();
    window.TapeWorkshopUpdates?.offline();
    return null;
  } finally {clearTimeout(timeout);statusBusy=false;}
}

function gb(bytes) { return bytes >= 1e9 ? (bytes / 1e9).toFixed(1) + " GB" : Math.round(bytes / 1e6) + " MB"; }

const sampleSearch = $("#sample-search");
sampleSearch?.addEventListener("submit", async event => {
  event.preventDefault();
  const input = sampleSearch.elements.namedItem("q");
  const button = $("button", sampleSearch);
  const message = $("#sample-search-status");
  const query = input.value.trim();
  button.disabled = true;
  try {
    const result = await fetch("/sample-search?q=" + encodeURIComponent(query), {
      method: "POST", headers: {"X-Melodee": "1"}, body: ""
    });
    const data = await result.json();
    message.textContent = result.ok ? (query ? "Check the results on your MPC." : "Search cleared.") : data.error;
  } catch {
    message.textContent = "Could not reach the MPC.";
  } finally {
    button.disabled = false;
  }
});

// ---- The device page ----

const device = {
  here: !!$("#device"),
  show(s) {
    if (!this.here) return;
    $("#v-version").textContent = s.version;
    $("#g-version").style.width = "100%";
    if (s.card) {
      $("#v-card").innerHTML = gb(s.card_free) + "<small>free</small>";
      $("#g-card").style.width = (s.card_total ? 100 * (1 - s.card_free / s.card_total) : 0) + "%";
    } else {
      $("#v-card").textContent = "None";
      $("#g-card").style.width = "0";
    }
    if (s.card !== this.card) { this.card = s.card; plugins(); }
    update.follow(s);
    panels.forEach(p => p.ready());
  },
  lost() {
    if (!this.here) return;
    this.card = false;
    panels.forEach(p => p.ready());
  },
};

// One upload panel: its drop zone, file, gauge, button and line of words.
function Panel(el, path) {
  const input = $("input", el), drop = $("[data-drop]", el);
  const self = {
    el, file: null, busy: false,
    say(text, kind) { const p = $("[data-say]", el); p.textContent = text || ""; p.className = "say" + (kind ? " " + kind : ""); },
    bar(part) { $("[data-bar]", el).style.width = (part * 100) + "%"; },
    ready() {
      el.classList.toggle("busy", self.busy);
      $("[data-send]", el).disabled = self.busy || sending || !self.file || !device.card;
      if (!device.card && !self.busy && last) self.say(last.card === false ? "No SD card in the MPC" : "", "bad");
    },
    pick(file) {
      if (file && !/\.zip$/i.test(file.name)) { self.say("Not a .zip", "bad"); return; }
      self.file = file || null;
      $("[data-name]", el).textContent = file ? file.name : $("[data-name]", el).dataset.empty;
      $("[data-hint]", el).textContent = file ? gb(file.size) : "Drop here or choose";
      drop.classList.toggle("chosen", !!file);
      self.say("");
      self.bar(0);
      self.ready();
    },
    send(done) {
      const file = self.file;
      self.busy = sending = true;
      self.ready();
      self.say("Sending…");
      const xhr = new XMLHttpRequest();
      xhr.open("POST", path + "?name=" + encodeURIComponent(file.name));
      xhr.setRequestHeader("X-Melodee", "1");
      xhr.setRequestHeader("Content-Type", "application/zip");
      xhr.upload.onprogress = e => { if (e.lengthComputable) self.bar(e.loaded / e.total); };
      xhr.upload.onload = () => { self.bar(1); self.say(path === "/update" ? "Checking the zip…" : "Unzipping…"); };
      xhr.onerror = () => { sending = self.busy = false; self.say("The MPC stopped answering", "bad"); self.ready(); done(null); };
      xhr.onload = () => {
        sending = false;
        let r = {};
        try { r = JSON.parse(xhr.responseText); } catch (e) {}
        self.pick(null);
        done(xhr.status === 200 ? r : {error: r.error || "Failed"});
      };
      xhr.send(file);
    },
  };
  $("[data-name]", el).dataset.empty = $("[data-name]", el).textContent;
  input.addEventListener("change", () => self.pick(input.files[0]));
  drop.addEventListener("dragover", e => { e.preventDefault(); drop.classList.add("over"); });
  drop.addEventListener("dragleave", () => drop.classList.remove("over"));
  drop.addEventListener("drop", e => { e.preventDefault(); drop.classList.remove("over"); if (!self.busy) self.pick(e.dataTransfer.files[0]); });
  $("[data-send]", el).addEventListener("click", () => self.file && self.send(el.id === "update" ? update.sent : installed));
  return self;
}

const panels = device.here ? [Panel($("#update"), "/update"), Panel($("#plugins"), "/plugin")] : [];
const [updatePanel, pluginPanel] = panels;

// The update: sent, asked on the MPC, then the MPC's own steps, then the restart.
const update = {
  watching: null, from: "",
  order: ["send", "ask", "saving", "checking", "writing", "starting"],
  step(name, how) {
    const at = this.order.indexOf(name);
    $$("#update .steps li").forEach((li, i) => {
      li.className = i < at ? "done" : i === at ? how || "now" : "";
    });
  },
  sent(r) {
    if (!r) { update.step("send", "failed"); return; }
    if (r.error) { updatePanel.busy = false; updatePanel.say(r.error, "bad"); update.step("send", "failed"); updatePanel.ready(); return; }
    update.watching = "offer";
    update.from = last ? last.version : "";
    update.step("ask", "now wait");
    updatePanel.say("Stop playback, then press INSTALL on the MPC to update to " + r.offered, "ask");
  },
  follow(s) {
    if (!updatePanel) return;
    if (this.watching && this.from && s.version !== this.from) {
      this.watching = null;
      $$("#update .steps li").forEach(li => li.className = "done");
      updatePanel.busy = false;
      updatePanel.say("Updated to " + s.version, "ok");
      return;
    }
    if (!this.watching) return;
    const words = {saving: "Saving the project…", checking: "Checking the update…", writing: "Copying to the card…", starting: "Restarting…"};
    if (s.phase === "failed") {
      this.step(this.order[Math.max(2, this.order.indexOf(this.lastPhase || "saving"))], "failed");
      updatePanel.say(s.error || "Update failed", "bad");
      this.watching = null;
      updatePanel.busy = false;
    } else if (words[s.phase]) {
      this.watching = "install";
      this.lastPhase = s.phase;
      this.step(s.phase);
      updatePanel.say(words[s.phase], "ask");
    } else if (this.watching === "offer" && !s.offer) {
      this.watching = null;
      this.step("ask", "failed");
      updatePanel.busy = false;
      updatePanel.say("LATER on the MPC: not installed", "");
    }
  },
};
if (device.here) update.step("none");

function installed(r) {
  pluginPanel.busy = false;
  if (!r) return;
  if (r.error) { pluginPanel.say(r.error, "bad"); pluginPanel.ready(); return; }
  pluginPanel.say("Installed " + (r.installed || []).join(", "), "ok");
  plugins(r.installed || []);
}

async function plugins(fresh = []) {
  if (!device.here) return;
  const list = $("#plugin-list");
  try {
    const p = await (await fetch("/plugins", {cache: "no-store"})).json();
    list.innerHTML = "";
    const real = p.plugins.filter(x => !x.folder || x.loadable);
    const ready = real.filter(x => x.loadable).length;
    $("#v-plugins").innerHTML = p.card ? ready + (real.length > ready ? "<small>of " + real.length + "</small>" : "") : "–";
    $("#g-plugins").style.width = real.length ? (100 * ready / real.length) + "%" : "0";
    if (!p.plugins.length) {
      const li = document.createElement("li");
      li.className = "empty";
      li.textContent = p.card ? "None yet" : "No SD card";
      list.append(li);
      return;
    }
    p.plugins.forEach((item, n) => {
      const li = document.createElement("li");
      li.style.setProperty("--stripe", tones[n % 4]);
      if (item.why) li.classList.add("off");
      if (fresh.includes(item.file)) li.classList.add("new");
      const name = document.createElement("span");
      name.className = "name";
      name.textContent = item.name;
      li.append(name);
      if (item.why) {
        const why = document.createElement("span");
        why.className = "why";
        why.textContent = item.why;
        li.append(why);
      }
      const chip = document.createElement("span");
      chip.className = "chip";
      chip.textContent = item.folder ? "FOLDER" : item.format;
      li.append(chip);
      const del = document.createElement("button");
      del.type = "button";
      del.className = "act del";
      del.title = "Delete";
      del.setAttribute("aria-label", "Delete " + item.name);
      del.innerHTML = glyphs.del;
      del.addEventListener("click", () => removePlugin(item));
      li.append(del);
      list.append(li);
    });
  } catch (e) { /* the next status ask says whether the MPC is there */ }
}

async function removePlugin(item) {
  const what = item.folder ? `Delete the folder "${item.file}" and everything in it?` : `Delete ${item.name} (${item.file})?`;
  if (!confirm(what + " This cannot be undone.")) return;
  try {
    const r = await fetch("/plugin/delete?file=" + encodeURIComponent(item.file), {method: "POST", headers: {"X-Melodee": "1"}, body: ""});
    const data = await r.json().catch(() => ({}));
    if (!r.ok) throw new Error(data.error || "Failed");
    pluginPanel.say("Deleted " + item.name, "ok");
  } catch (e) { pluginPanel.say(e.message, "bad"); }
  plugins();
}

// ---- Files: the card's folders ----

const glyphs = {
  dir: '<svg class="icon" viewBox="0 0 20 20" aria-hidden="true"><path d="M2.5 5h5.5l1.5 2h8v9h-15z" fill="currentColor" opacity=".9"/></svg>',
  audio: '<svg class="icon" viewBox="0 0 20 20" aria-hidden="true"><path d="M3 10h1.5M6 6v8M9 3v14M12 7v6M15 5v10M17.5 9v2" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" fill="none"/></svg>',
  file: '<svg class="icon" viewBox="0 0 20 20" aria-hidden="true"><path d="M5 2.5h6.5l3.5 3.5v11.5H5z M11.5 2.5V6H15" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linejoin="round"/></svg>',
  play: '<svg viewBox="0 0 16 16" aria-hidden="true"><path d="M4.5 3v10l8.5-5z" fill="currentColor"/></svg>',
  stop: '<svg viewBox="0 0 16 16" aria-hidden="true"><rect x="4" y="4" width="8" height="8" rx="1" fill="currentColor"/></svg>',
  save: '<svg viewBox="0 0 16 16" aria-hidden="true"><path d="M8 2.5v8M4.5 7 8 10.5 11.5 7M3 13.5h10" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"/></svg>',
  del: '<svg viewBox="0 0 16 16" aria-hidden="true"><path d="M3 4.5h10M6.5 4.5V3h3v1.5M4.5 4.5l.7 9h5.6l.7-9" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linejoin="round" stroke-linecap="round"/></svg>',
};
const audioKinds = /\.(wav|mp3|flac|ogg|oga|aif|aiff)$/i;

const files = {
  here: !!$("#files"),
  path: "", writable: false, fresh: new Set(), playing: "",
  rel(name) { return this.path ? this.path + "/" + name : name; },
  url(rel, save) { return "/file?path=" + encodeURIComponent(rel) + (save ? "&save=1" : ""); },
  size(n) { return n >= 1e9 ? (n / 1e9).toFixed(1) + " GB" : n >= 1e6 ? (n / 1e6).toFixed(1) + " MB" : n >= 1e3 ? Math.round(n / 1e3) + " KB" : n + " B"; },
  when(t) {
    if (!t) return "";
    const d = new Date(t * 1000);
    return d.toLocaleDateString(undefined, {day: "numeric", month: "short", year: d.getFullYear() === new Date().getFullYear() ? undefined : "numeric"});
  },
  async open(path) {
    this.path = path;
    const list = $("#entries");
    let r;
    try { r = await fetch("/list?path=" + encodeURIComponent(path), {cache: "no-store"}); } catch (e) { return; }
    const data = await r.json().catch(() => ({}));
    this.crumbs();
    list.innerHTML = "";
    if (!r.ok) { this.say(data.error || "Cannot open this folder"); this.writable = false; this.tools(); return; }
    this.writable = data.writable;
    this.tools();
    if (!data.entries.length) { this.say(this.writable ? "Empty. Drop files here." : "Empty"); return; }
    data.entries.forEach(e => list.append(this.row(e)));
    this.fresh.clear();
  },
  say(words) {
    const li = document.createElement("li");
    li.className = "empty";
    li.textContent = words;
    $("#entries").append(li);
  },
  tools() {
    $("#tools").hidden = !this.writable;
    $("#readonly").hidden = this.writable || !last || !last.card;
  },
  crumbs() {
    const nav = $("#crumbs");
    nav.innerHTML = "";
    const parts = this.path ? this.path.split("/") : [];
    const link = (label, to) => {
      const a = document.createElement("a");
      a.href = "#" + to.split("/").map(encodeURIComponent).join("/");
      a.textContent = label;
      return a;
    };
    nav.append(link("SD card", ""));
    parts.forEach((p, i) => {
      const s = document.createElement("span");
      s.className = "sep";
      s.textContent = "/";
      nav.append(s, link(p, parts.slice(0, i + 1).join("/")));
    });
  },
  row(e) {
    const li = document.createElement("li");
    const rel = this.rel(e.name);
    const audio = !e.dir && audioKinds.test(e.name);
    li.className = "entry " + (e.dir ? "dir" : audio ? "audio" : "file");
    li.style.setProperty("--stripe", e.dir ? "var(--amber)" : audio ? "var(--mint)" : "var(--lilac)");
    if (this.fresh.has(e.name)) li.classList.add("fresh");
    if (rel === this.playing) li.classList.add("playing");
    li.innerHTML = glyphs[e.dir ? "dir" : audio ? "audio" : "file"];
    const name = document.createElement("span");
    name.className = "name";
    name.textContent = e.name;
    const size = document.createElement("span");
    size.className = "meta";
    size.textContent = e.dir ? "" : this.size(e.size);
    const when = document.createElement("span");
    when.className = "meta when";
    when.textContent = this.when(e.time);
    const acts = document.createElement("span");
    acts.className = "acts";
    const act = (kind, title, run) => {
      const b = document.createElement(kind === "save" ? "a" : "button");
      b.className = "act " + kind;
      b.title = title;
      b.setAttribute("aria-label", title + " " + e.name);
      b.innerHTML = glyphs[kind === "play" && rel === this.playing ? "stop" : kind];
      if (kind === "save") { b.href = this.url(rel, true); b.setAttribute("download", e.name); }
      else b.addEventListener("click", ev => { ev.stopPropagation(); run(); });
      return b;
    };
    if (audio) acts.append(act("play", "Play", () => this.play(rel)));
    if (!e.dir) acts.append(act("save", "Download"));
    if (this.writable) acts.append(act("del", "Delete", () => this.remove(e)));
    li.append(name, size, when, acts);
    if (e.dir) li.addEventListener("click", () => { location.hash = rel.split("/").map(encodeURIComponent).join("/"); });
    else if (audio) li.addEventListener("click", () => this.play(rel));
    return li;
  },
  play(rel) {
    const player = $("#player");
    if (this.playing === rel) { player.pause(); this.playing = ""; }
    else { player.src = this.url(rel); player.play().catch(() => {}); this.playing = rel; }
    this.mark();
  },
  mark() {
    $$("#entries .entry").forEach(li => {
      const rel = this.rel($(".name", li).textContent);
      const on = rel === this.playing;
      li.classList.toggle("playing", on);
      const b = $(".act.play", li);
      if (b) b.innerHTML = glyphs[on ? "stop" : "play"];
    });
  },
  async post(path) {
    const r = await fetch(path, {method: "POST", headers: {"X-Melodee": "1"}, body: ""});
    const data = await r.json().catch(() => ({}));
    if (!r.ok) throw new Error(data.error || "Failed");
    return data;
  },
  async folder() {
    const name = prompt("New folder name");
    if (!name) return;
    try {
      await this.post("/mkdir?path=" + encodeURIComponent(this.path) + "&name=" + encodeURIComponent(name.trim()));
      this.fresh.add(name.trim());
      this.open(this.path);
    } catch (e) { alert(e.message); }
  },
  async remove(e) {
    const what = e.dir ? `Delete the folder "${e.name}" and everything in it?` : `Delete "${e.name}"?`;
    if (!confirm(what + " This cannot be undone.")) return;
    try {
      if (this.playing === this.rel(e.name)) this.play(this.playing);
      await this.post("/delete?path=" + encodeURIComponent(this.rel(e.name)));
      this.open(this.path);
    } catch (err) { alert(err.message); }
  },
  // Files one at a time (the MPC serves one request at a time), into the folder open when they were dropped.
  async upload(list) {
    const queue = [...list].filter(f => f.size >= 0);
    if (!queue.length || !this.writable) return;
    const into = this.path;
    sending = true;
    $("#queue").hidden = false;
    let done = 0, failed = [];
    for (const file of queue) {
      $("#queue-words").textContent = `${done + 1} of ${queue.length} · ${file.name}`;
      $("#queue-bar").style.width = "0";
      const r = await new Promise(resolve => {
        const xhr = new XMLHttpRequest();
        xhr.open("POST", "/upload?path=" + encodeURIComponent(into) + "&name=" + encodeURIComponent(file.name));
        xhr.setRequestHeader("X-Melodee", "1");
        xhr.setRequestHeader("Content-Type", "application/octet-stream");
        xhr.upload.onprogress = e => { if (e.lengthComputable) $("#queue-bar").style.width = (100 * e.loaded / e.total) + "%"; };
        xhr.onload = () => { let d = {}; try { d = JSON.parse(xhr.responseText); } catch (e) {} resolve(xhr.status === 200 ? d : {error: d.error || "Failed"}); };
        xhr.onerror = () => resolve({error: "The MPC stopped answering"});
        xhr.send(file);
      });
      done++;
      if (r.error) failed.push(file.name + ": " + r.error);
      else this.fresh.add(r.name);
    }
    sending = false;
    $("#queue-words").textContent = failed.length ? failed.join(" · ") : `${done} uploaded`;
    $("#queue-bar").style.width = "100%";
    setTimeout(() => { if (!sending) $("#queue").hidden = true; }, failed.length ? 8000 : 2500);
    if (this.path === into) this.open(into);
  },
  start() {
    if (!this.here) return;
    const go = () => this.open(decodeURIComponent(location.hash.slice(1)).split("/").filter(Boolean).join("/"));
    addEventListener("hashchange", go);
    $("#player").addEventListener("ended", () => { this.playing = ""; this.mark(); });
    $("#new-folder").addEventListener("click", () => this.folder());
    $("#upload-input").addEventListener("change", e => { this.upload(e.target.files); e.target.value = ""; });
    const zone = $("#file-list");
    let depth = 0;
    zone.addEventListener("dragenter", e => { e.preventDefault(); if (this.writable && ++depth === 1) zone.classList.add("over"); });
    zone.addEventListener("dragleave", () => { if (--depth <= 0) { depth = 0; zone.classList.remove("over"); } });
    zone.addEventListener("dragover", e => e.preventDefault());
    zone.addEventListener("drop", e => { e.preventDefault(); depth = 0; zone.classList.remove("over"); this.upload(e.dataTransfer.files); });
    go();
  },
};

// ---- The docs ----

function docs() {
  // On a phone the chapter list starts closed.
  const menu = $(".chapters-menu");
  if (menu && matchMedia("(max-width: 860px)").matches) menu.open = false;
  // The section in view is marked in the side list.
  const links = $$("aside .toc a");
  if (!links.length || !("IntersectionObserver" in window)) return;
  const byId = new Map(links.map(a => [decodeURIComponent(a.hash.slice(1)), a]));
  const seen = new Set();
  const watch = new IntersectionObserver(entries => {
    entries.forEach(e => e.isIntersecting ? seen.add(e.target.id) : seen.delete(e.target.id));
    const first = $$(".doc h2[id]").find(h => seen.has(h.id));
    if (!first) return;
    links.forEach(a => a.classList.toggle("here", a === byId.get(first.id)));
  }, {rootMargin: "-70px 0px -55% 0px"});
  $$(".doc h2[id]").forEach(h => watch.observe(h));
}

docs();
status().then(() => { plugins(); files.start(); });
setInterval(() => {
  if (sending || asking) return;
  asking = true;
  status().finally(() => { asking = false; });
}, 1500);

// One in-flight screen request; no polling while paused, hidden or uploading.
(() => {
  if (!$("#live-screen")) return;
  const page = $("#live-screen"), canvas = $("#live-canvas"), ctx = canvas.getContext("2d");
  const state = $("#live-state"), pause = $("#live-pause"), save = $("#live-save");
  let paused = false, controller = null, screenshot = null, timer = null, lastScreen = performance.now();
  let audioStarting = null, audioStopping=null, resumeAudioOnReturn=false, audioFailures=0;
  let audioContext = null, audioDestination = null, recordingBus = null, audioOn = false, audioId = 0, audioRequest = null;
  let audioOrigin = null, audioBase = 0, audioRate = 0, lastAudio = 0, previousDrops = null;
  let hardwareQueue = [], showHardware = () => {};
  let videoQueue = [], recorder = null, recordTimer = null, recordingCanvas = null, devicePicture = null;
  let refreshDevicePicture = () => {}, recordingUrl = null, recordingNote = "", recordingStore = null;
  const recordState = $("#record-state"), recordStart = $("#record-start"), recordStop = $("#record-stop"), recordPause = $("#record-pause");
  let recordClock=null;
  const timecode=$("#record-timecode"),clockState=$("#record-clock-state");
  function updateRecordingClock(elapsed,label){
    if(timecode){const value=window.WorkshopRecordingPolicy.clock(elapsed);timecode.textContent=value.split(":").length===2?"00:"+value:value.padStart(8,"0");}
    if(clockState)clockState.textContent=label;
    page.dataset.captureState=label.toLowerCase();
  }
  const audioButton = $("#audio-enable"), recordingDownload = $("#record-download");
  const isRecording = () => recorder && recorder.state !== "inactive";
  function uint64(view, offset) { return view.getUint32(offset, true) + view.getUint32(offset + 4, true) * 4294967296; }
  function displayFrame(frame) {
    if(frame.pixels) ctx.putImageData(frame.pixels,0,0);
    else ctx.drawImage(frame.picture, 0, 0, 320, 240);
    const mini = $("#device-screen");
    if (mini) { const paint = mini.getContext("2d"); paint.imageSmoothingEnabled = false; paint.drawImage(canvas, 0, 0, 320, 240); }
    screenshot = true; save.disabled = false;
    if(!last)link("on","MPC connected");
    state.textContent = "Live · " + new Date().toLocaleTimeString();
    dispose(frame);
  }
  function presentLive() {
    requestAnimationFrame(presentLive);
    if (paused || document.hidden) return;
    const clockRunning=audioOn && audioOrigin!==null && audioContext?.state==="running";
    const sample = clockRunning ? audioBase + (audioContext.currentTime - audioOrigin) * audioRate : -Infinity;
    let selected = null;
    while (videoQueue.length && (videoQueue[0].sample <= sample || performance.now()-videoQueue[0].received >= 350)) { dispose(selected); selected = videoQueue.shift(); }
    if (selected) displayFrame(selected);
    let hardware = null;
    while (hardwareQueue.length && (hardwareQueue[0].sample <= sample || performance.now()-hardwareQueue[0].received >= 350)) hardware = hardwareQueue.shift();
    if (hardware) showHardware(hardware);
  }
  requestAnimationFrame(presentLive);
  ctx.imageSmoothingEnabled = false;
  let lastFrameStamp="";
  function dispose(frame) { frame?.picture?.close?.(); }
  function clearVideo() { videoQueue.forEach(dispose); videoQueue=[]; }
  function schedule(delay = 33) { clearTimeout(timer); timer = setTimeout(refresh, delay); }
  async function refresh() {
    if (paused || document.hidden || sending) { schedule(500); return; }
    if(controller)return;
    const requestAt=performance.now(), request=new AbortController();
    controller = request;
    const timeout = setTimeout(() => request.abort(), 5000);
    let url = null;
    try {
      const response = await fetch("/screen-frame", {cache:"no-store", signal:request.signal});
      if (!response.ok) throw new Error("Waiting for the MPC screen…");
      const bytes = await response.arrayBuffer(), header = new DataView(bytes);
      const format=bytes.byteLength>=16?String.fromCharCode(...new Uint8Array(bytes,0,4)):"";
      if (!["TWVF","TWRF","TWF2"].includes(format)) throw new Error("Invalid screen frame");
      const rate = header.getUint32(4, true), sample = uint64(header, 8);
      if(format==="TWF2" && bytes.byteLength<24)throw new Error("Incomplete screen header");
      const stamp=format==="TWF2"?"sequence:"+header.getUint32(20,true):rate+":"+sample;
      if(stamp===lastFrameStamp){
        if(isRecording()&&performance.now()-lastScreen>2500)stopRecording("Screen connection interrupted; recording stopped.");
        return;
      }
      const blob = format==="TWVF" ? new Blob([bytes.slice(16)], {type:"image/png"}) : null;
      let frame, pixels;
      if(format==="TWRF" || format==="TWF2") {
        const offset=format==="TWF2"?24:16;
        const encoding=format==="TWF2"?header.getUint32(16,true):0;
        pixels=new ImageData(320,240);
        const rgba=pixels.data;
        let position=0;
        function put(value,count) {
          if(!count||position+count>320*240)throw new Error("Invalid pixel run");
          const r=value>>>11,g=(value>>>5)&63,b=value&31;
          for(let n=0;n<count;n++){
            const at=position++*4;
            rgba[at]=(r<<3)|(r>>>2);rgba[at+1]=(g<<2)|(g>>>4);rgba[at+2]=(b<<3)|(b>>>2);rgba[at+3]=255;
          }
        }
        if(encoding===0){
          if(bytes.byteLength!==offset+320*240*2)throw new Error("Incomplete pixel frame");
          for(let at=offset;at<bytes.byteLength;at+=2)put(header.getUint16(at,true),1);
        }else if(encoding===1){
          if((bytes.byteLength-offset)%4)throw new Error("Invalid pixel runs");
          for(let at=offset;at<bytes.byteLength;at+=4)put(header.getUint16(at+2,true),header.getUint16(at,true));
          if(position!==320*240)throw new Error("Incomplete pixel runs");
        }else throw new Error("Unknown pixel encoding");
      }
      if(blob&&window.createImageBitmap) {
        try { frame=await createImageBitmap(blob); } catch {}
      }
      if(blob&&!frame){
        url = URL.createObjectURL(blob);frame=new Image();
        await new Promise((resolve,reject)=>{frame.onload=resolve;frame.onerror=reject;frame.src=url;});
      }
      lastFrameStamp=stamp;
      if (!paused && !document.hidden) {
        lastScreen = performance.now();
        const next = {picture:frame, pixels, rate, sample, received:performance.now()};
        if (audioOn && audioOrigin !== null && rate === audioRate) {
          videoQueue.push(next);
          if (videoQueue.length > 24) dispose(videoQueue.shift());
        } else displayFrame(next);
      } else frame?.close?.();
    } catch (error) {
      if (!paused && !document.hidden) state.textContent = "Reconnecting to your MPC…";
      if (isRecording() && performance.now() - lastScreen > 2500) stopRecording("Screen connection interrupted; recording stopped.");
    } finally {
      clearTimeout(timeout);
      if (url) URL.revokeObjectURL(url);
      if(controller===request)controller = null;
      schedule(Math.max(0,33-(performance.now()-requestAt)));
    }
  }
  pause.addEventListener("click", () => {
    paused = !paused;
    pause.textContent = paused ? "Resume view" : "Pause view";
    state.textContent = paused ? "Paused · last screen held" : "Connecting…";
    if (paused) controller?.abort();
    else if (!controller) schedule(0);
  });
  document.addEventListener("visibilitychange", () => {
    if (document.hidden) { controller?.abort(); if (isRecording()) stopRecording("Stopped because the page was hidden."); resumeAudioOnReturn=audioOn; if (audioOn) stopAudio(); }
    else {
      if (!controller) schedule(0);
      if(resumeAudioOnReturn){resumeAudioOnReturn=false;enableAudio().catch(error=>{recordState.textContent='Tap Enable MPC audio to resume: '+error.message;});}
    }
  });
  const expand = $("#live-expand"), exit = $("#live-exit-fullscreen");
  let nativeFull = false;
  function showFullScreen(on) {
    page.classList.toggle("fullscreen", on);
    document.body.classList.toggle("screen-fullscreen", on);
    expand.setAttribute("aria-pressed", String(on));
    exit.hidden = !on;
    if (on) exit.focus();
    else expand.focus();
  }
  async function leaveFullScreen() {
    showFullScreen(false);
    if (document.fullscreenElement === page) {
      try { await document.exitFullscreen(); } catch {}
    }
    nativeFull = false;
  }
  expand.addEventListener("click", async () => {
    showFullScreen(true);
    // iPhone Safari may omit the Fullscreen API: the edge-to-edge viewer
    // still fills the available browser viewport without the page furniture.
    if (page.requestFullscreen) {
      try { await page.requestFullscreen(); nativeFull = document.fullscreenElement === page; } catch {}
    }
  });
  exit.addEventListener("click", leaveFullScreen);
  document.addEventListener("keydown", event => {
    if (event.key === "Escape" && page.classList.contains("fullscreen")) leaveFullScreen();
  });
  document.addEventListener("fullscreenchange", () => {
    if (nativeFull && document.fullscreenElement !== page) {
      nativeFull = false;
      showFullScreen(false);
    }
  });
  save.addEventListener("click", async () => {
    if (!screenshot) return;
    let target=null;
    if(window.showSaveFilePicker){try{target=await window.showSaveFilePicker({suggestedName:"Tape-Workshop-screen.png"});}catch(error){if(error.name!=="AbortError")state.textContent=error.message;return;}}
    canvas.toBlob(async blob=>{
    if(!blob)return;
    if(target){try{const file=await target.createWritable();await file.write(blob);await file.close();}catch(error){state.textContent=error.message;}return;}
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url; a.download = "Tape-Workshop-screen.png"; a.target = "_blank"; a.rel = "noopener";
    document.body.appendChild(a); a.click(); a.remove();
    setTimeout(() => URL.revokeObjectURL(url), 60000);
    },"image/png");
  });
  function stopAudio(){
    if(audioStopping)return audioStopping;
    const task=stopAudioNow();audioStopping=task;
    task.finally(()=>{if(audioStopping===task)audioStopping=null;}).catch(()=>{});
    return task;
  }
  async function stopAudioNow() {
    if (isRecording()) stopRecording("Audio was stopped.");
    audioOn = false; audioRequest?.abort();
    const id = audioId; audioId = 0; audioOrigin = null; clearVideo(); hardwareQueue = [];
    if (audioContext) { try { await audioContext.close(); } catch {} }
    recordingBus?.disconnect(); recordingBus=null;
    audioContext = null; audioDestination = null;meterAnalysers=[];meterArrays=[];spectrumArrays=[];meterSplitter=null;
    audioButton.textContent = "Enable MPC audio";
    if (id) fetch("/audio/stop?id=" + id, {method:"POST", headers:{"X-Melodee":"1"}, body:"", keepalive:true}).catch(() => {});
  }
  async function pollAudio() {
    if (!audioOn || document.hidden) return;
    const session = audioId, request = new AbortController();
    audioRequest = request;
    const timeout = setTimeout(() => request.abort(), 3000);
    try {
      const response = await fetch("/audio?id=" + audioId, {cache:"no-store", signal:request.signal});
      if (!response.ok) throw new Error("Audio session ended. Enable MPC audio again.");
      const bytes = await response.arrayBuffer(), view = new DataView(bytes);
      if (session !== audioId || !audioOn) return;
      if (bytes.byteLength < 32 || String.fromCharCode(...new Uint8Array(bytes,0,4)) !== "TWAC") throw new Error("Invalid audio packet");
      const rate = view.getUint32(4,true), first = uint64(view,8), count = view.getUint32(16,true), drops = view.getUint32(20,true);
      if (bytes.byteLength !== 32 + count * 4) throw new Error("Incomplete audio packet");
      if (previousDrops !== null && drops !== previousDrops && isRecording()) stopRecording("Audio connection dropped samples; check this take.");
      previousDrops = drops;audioFailures=0;
      if (count && audioOn && audioContext) {
        if (audioRate && rate !== audioRate) {
          if (isRecording()) stopRecording("The MPC sample rate changed; recording stopped.");
          audioOrigin = null; clearVideo(); hardwareQueue = [];
        }
        audioRate = rate;
        if (audioOrigin === null) { audioBase = first; audioOrigin = audioContext.currentTime + .25; }
        let at = audioOrigin + (first - audioBase) / rate;
        if (at < audioContext.currentTime - .02 || at > audioContext.currentTime + 2) {
          if (isRecording()) stopRecording("Audio timing was interrupted; check this take.");
          audioBase = first; audioOrigin = audioContext.currentTime + .25; at = audioOrigin; clearVideo();
        }
        const buffer = audioContext.createBuffer(2, count, rate);
        for (let channel=0; channel<2; ++channel) {
          const samples = buffer.getChannelData(channel);
          for (let n=0; n<count; ++n) samples[n] = view.getInt16(32 + (n*2+channel)*2,true) / 32768;
        }
        const source = audioContext.createBufferSource(); source.buffer = buffer;
        source.connect(audioContext.destination); source.connect(recordingBus.input);source.connect(meterSplitter);
        source.onended=()=>source.disconnect();
        source.start(at); lastAudio = performance.now();
      }
      if (audioOn && performance.now() - lastAudio > 5000) throw new Error("No audio arriving from the MPC. Enable audio again.");
    } catch (error) {
      if (audioOn && session === audioId) {
        audioFailures++;
        if(audioFailures<3 && performance.now()-lastAudio<6000){recordState.textContent='Reconnecting MPC audio…';return;}
        if (isRecording()) stopRecording("Audio connection interrupted; recording stopped.");
        recordState.textContent = error.message || "Audio connection interrupted.";
        await stopAudio();
      }
    } finally {
      clearTimeout(timeout);
      if (audioRequest === request) audioRequest = null;
      if (audioOn && session === audioId) setTimeout(pollAudio, 40);
    }
  }
  async function enableAudio() {
    if(audioStopping)await audioStopping;
    if (audioOn) return;
    if (audioStarting) return audioStarting;
    audioStarting = (async () => {
      const Context = window.AudioContext || window.webkitAudioContext;
      if (!Context) throw new Error("This browser cannot play direct MPC audio.");
      audioContext = new Context();
      await audioContext.resume();
      meterSplitter=audioContext.createChannelSplitter(2);
      meterAnalysers=[0,1].map(channel=>{const analyser=audioContext.createAnalyser();analyser.fftSize=1024;meterSplitter.connect(analyser,channel);return analyser;});
      meterArrays=meterAnalysers.map(()=>new Float32Array(1024));
      spectrumArrays=meterAnalysers.map(()=>new Uint8Array(512));
      recordingBus = window.WorkshopStereoRecording.create(audioContext);
      audioDestination = recordingBus.destination;
      const response = await fetch("/audio/start", {method:"POST", headers:{"X-Melodee":"1"}, body:""});
      if (!response.ok) throw new Error("Could not start MPC audio.");
      audioId = (await response.json()).id;
      audioOn = true; audioOrigin = null; audioRate = 0; previousDrops = null; lastAudio = performance.now();
      audioButton.textContent = "Stop MPC audio";
      recordState.textContent = "Direct stereo audio enabled. Keep the page open.";
      pollAudio();
    })();
    try { await audioStarting; } finally { audioStarting = null; }
  }
  audioButton.addEventListener("click", async () => {
    audioButton.disabled = true;
    try { resumeAudioOnReturn=false;if (audioOn) await stopAudio(); else await enableAudio(); }
    catch (error) { await stopAudio(); recordState.textContent = error.message; }
    finally { audioButton.disabled = false; }
  });
  function recoverLiveAfterSave(){
    if(document.hidden||paused)return;
    lastFrameStamp="";
    if(audioContext?.state==='suspended')audioContext.resume().catch(()=>{});
    if(!controller)schedule(0);
  }
  window.addEventListener('focus',recoverLiveAfterSave);
  // Invoke the picker inside the user gesture; browsers without it use downloads.
  for(const link of [recordingDownload]) link?.addEventListener('click',async event=>{
    const toSave=recordingStore;
    if(!link.href.startsWith('blob:')&&!toSave?.ready){event.preventDefault();recordState.textContent='Stop recording first, then save the video.';return;}
    if(!window.showSaveFilePicker){link.target='_blank';link.rel='noopener';setTimeout(recoverLiveAfterSave,500);return;}
    event.preventDefault();
    recordStart.disabled=true;
    try {
      const file=await window.showSaveFilePicker({suggestedName:link.download});
      const writable=await file.createWritable();
      try {if(toSave?.ready)await toSave.writeTo(writable,fraction=>{recordState.textContent="Saving video: "+Math.round(fraction*100)+"%";});else {const response=await fetch(link.href);await response.body.pipeTo(writable);}recordState.textContent="Video saved.";} catch(error){try{await writable.abort();}catch{}throw error;}
    }catch(error){if(error.name!=='AbortError')recordState.textContent='Could not save: '+error.message;}
    finally{recordStart.disabled=isRecording();recoverLiveAfterSave();}
  });
  let meterAnalysers=[],meterArrays=[],spectrumArrays=[],meterSplitter=null,meterBank=0;
  let recordingVideo=null;
  const presentation=window.WorkshopPresentation;
  for(const [id,key,allowed,fallback] of [
    ['screen-frame','tape-workshop-screen-frame',['plain','tablet'],'plain'],
    ['screen-rendering','tape-workshop-screen-rendering',['smooth','original'],'smooth']]){
    const select=$("#"+id);if(!select)continue;
    try { const saved=localStorage.getItem(key);select.value=allowed.includes(saved)?saved:fallback; } catch {}
    const update=()=>{
      page.classList.toggle('tablet-frame',$("#screen-frame")?.value==='tablet');
      page.classList.toggle('original-screen-pixels',$("#screen-rendering")?.value==='original');
      const device=page.querySelector('.mpc-device');
      if(device)device.style.transform='none';
      try { localStorage.setItem(key,select.value); } catch {}
    };
    select.addEventListener('change',update);update();
  }
  const scene=$("#tutorial-scene"),sceneCanvas=$("#scene-background"),sceneMotion=$("#scene-motion"),sceneReactive=$("#scene-reactive");
  const sceneAudio={level:0,bass:0,bands:new Float32Array(64)};
  let sceneAudioAt=-Infinity,frozenSceneAudio=null;
  function readSceneAudio(){
    const now=performance.now();if(now-sceneAudioAt<30)return sceneAudio;sceneAudioAt=now;
    if(!audioOn||audioContext?.state!=="running"||meterAnalysers.length!==2){sceneAudio.level=0;sceneAudio.bass=0;sceneAudio.bands.fill(0);return sceneAudio;}
    let sum=0;for(let ch=0;ch<2;ch++){meterAnalysers[ch].getFloatTimeDomainData(meterArrays[ch]);meterAnalysers[ch].getByteFrequencyData(spectrumArrays[ch]);for(const value of meterArrays[ch])sum+=value*value;}
    const level=Math.min(1,Math.sqrt(sum/2048)*3);sceneAudio.level+=(level-sceneAudio.level)*.25;
    for(let n=0;n<64;n++){const first=Math.floor((n/64)**2*512),end=Math.min(512,Math.max(first+1,Math.floor(((n+1)/64)**2*512)));let value=0;for(let at=first;at<end;at++)value+=(spectrumArrays[0][at]+spectrumArrays[1][at])/510;value/=(end-first);sceneAudio.bands[n]+=(value-sceneAudio.bands[n])*.3;}
    sceneAudio.bass=sceneAudio.bands.subarray(0,8).reduce((a,b)=>a+b,0)/8;return sceneAudio;
  }
  function sceneInput(){return sceneReactive?.checked?(sceneMotion?.checked?readSceneAudio():frozenSceneAudio):null;}
  if(sceneReactive){try{const saved=localStorage.getItem("tape-workshop-scene-reactive");if(saved!==null)sceneReactive.checked=saved==="true";}catch{}sceneReactive.addEventListener("change",()=>{try{localStorage.setItem("tape-workshop-scene-reactive",String(sceneReactive.checked));}catch{}});}
  const sceneClock=window.WorkshopRecordingPolicy.createClock(performance.now());
  let sceneAnimation=null,sceneDrawnAt=-Infinity;
  function sceneTime(){return sceneClock.elapsed(performance.now())/1000;}
  function renderScene(now=performance.now()){
    sceneAnimation=null;
    if(document.hidden||!sceneCanvas||!window.WorkshopScenes.isAnimated(scene?.value))return;
    if(now-sceneDrawnAt>=1000/30){window.WorkshopScenes.draw(sceneCanvas.getContext("2d"),scene.value,sceneTime(),sceneInput());sceneDrawnAt=now;}
    if(sceneMotion?.checked)sceneAnimation=requestAnimationFrame(renderScene);
  }
  if(scene){
    try{const saved=localStorage.getItem("tape-workshop-scene");if(["studio","flat","custom","neon","warehouse","prism","sunset","space","arcade","scope","embers","ribbons","cubes","tunnel"].includes(saved))scene.value=saved;}catch{}
    if(sceneMotion){
      sceneMotion.checked=!window.matchMedia?.("(prefers-reduced-motion: reduce)").matches;
      try{const saved=localStorage.getItem("tape-workshop-scene-motion");if(saved!==null)sceneMotion.checked=saved==="true";}catch{}
      const updateMotion=()=>{if(sceneMotion.checked)sceneClock.resume(performance.now());else {const audio=readSceneAudio();frozenSceneAudio={level:audio.level,bass:audio.bass,bands:audio.bands.slice()};sceneClock.pause(performance.now());}try{localStorage.setItem("tape-workshop-scene-motion",String(sceneMotion.checked));}catch{}updateScene();};
      sceneMotion.addEventListener("change",updateMotion);
      if(!sceneMotion.checked)sceneClock.pause(performance.now());
    }
    function updateScene(save=false){
      const animated=window.WorkshopScenes.isAnimated(scene.value);
      page.classList.toggle("custom-scene",scene.value==="custom");
      page.classList.toggle("studio-room",scene.value==="studio");page.classList.toggle("animated-scene",animated);
      if(sceneCanvas)sceneCanvas.hidden=!animated;
      if(sceneMotion)sceneMotion.disabled=!animated;
      cancelAnimationFrame(sceneAnimation);sceneDrawnAt=-Infinity;renderScene();
      if(save)try{localStorage.setItem("tape-workshop-scene",scene.value);}catch{}
    }
    scene.addEventListener("change",()=>updateScene(true));document.addEventListener("visibilitychange",()=>updateScene());
    updateScene();
  }
  const waveformCanvas=$("#audio-waveform"),waveformToggle=$("#scene-waveform");
  let waveformFrame=null,waveformDrawnAt=-Infinity;
  if(waveformToggle){
    try{const saved=localStorage.getItem("tape-workshop-waveform");if(saved!==null)waveformToggle.checked=saved==="true";}catch{}
    const updateWaveform=()=>{waveformCanvas.hidden=!waveformToggle.checked;try{localStorage.setItem("tape-workshop-waveform",String(waveformToggle.checked));}catch{}};
    waveformToggle.addEventListener("change",updateWaveform);updateWaveform();
  }
  function waveformData(){
    if(!audioOn||audioContext?.state!=="running"||meterAnalysers.length!==2)return [null,null];
    for(let channel=0;channel<2;channel++)meterAnalysers[channel].getFloatTimeDomainData(meterArrays[channel]);
    return meterArrays;
  }
  function drawWaveformFrame(now){
    waveformFrame=null;
    if(document.hidden)return;
    if(waveformCanvas&&!waveformCanvas.hidden&&now-waveformDrawnAt>=1000/30){
      const paint=waveformCanvas.getContext("2d"),data=waveformData();paint.clearRect(0,0,1232,22);
      window.WorkshopScenes.waveform(paint,data[0],data[1],{x:0,y:0,width:1232,height:22},scene?.value==="studio");waveformDrawnAt=now;
    }
    waveformFrame=requestAnimationFrame(drawWaveformFrame);
  }
  document.addEventListener("visibilitychange",()=>{cancelAnimationFrame(waveformFrame);if(!document.hidden)waveformFrame=requestAnimationFrame(drawWaveformFrame);});
  waveformFrame=requestAnimationFrame(drawWaveformFrame);
  const background=$("#tutorial-background");
  if(background){
    function chooseBackground(selectFlat=false){
      page.style.setProperty("--tutorial-bg",background.value);
      if(selectFlat&&scene){scene.value="flat";scene.dispatchEvent(new Event("change"));}
      try{localStorage.setItem("tape-workshop-tutorial-background",background.value);}catch{}
    }
    try{const saved=localStorage.getItem("tape-workshop-tutorial-background");if(/^#[0-9a-f]{6}$/i.test(saved))background.value=saved;}catch{}
    background.addEventListener("input",()=>chooseBackground(true));
    background.addEventListener("change",()=>chooseBackground(true));
    $("#tutorial-bg-reset")?.addEventListener("click",()=>{background.value="#101527";chooseBackground(true);});
    chooseBackground();
  }
  const stage=page.querySelector('.tutorial-layout');
  let layoutFrom=presentation.layout('both'),layoutTo=layoutFrom,layoutAt=0,layoutFrame=0;
  const cameras={device:{from:{zoom:1,x:0,y:0,yaw:0},to:{zoom:1,x:0,y:0,yaw:0},at:0},screen:{from:{zoom:1,x:0,y:0,yaw:0},to:{zoom:1,x:0,y:0,yaw:0},at:0}};
  function cameraNow(panel){
    const c=cameras[panel],progress=Math.min(1,(performance.now()-c.at)/220),ease=(1-Math.cos(Math.PI*progress))/2;
    return {...Object.fromEntries(['zoom','x','y','yaw'].map(key=>[key,c.from[key]+(c.to[key]-c.from[key])*ease])),smooth:$("#screen-rendering")?.value!=="original"};
  }
  function setCamera(panel,values){
    const c=cameras[panel];c.from=cameraNow(panel);c.to={...c.to,...values};c.at=performance.now();
    for(const key of ['zoom','x','y','yaw']){
      const control=$('#'+panel+'-'+key);if(!control)continue;
      control.value=key==='zoom'||key==='yaw'?c.to[key]:Math.round(c.to[key]*100);
      $('#'+panel+'-'+key+'-value').textContent=key==='zoom'?c.to[key].toFixed(2)+'×':key==='yaw'?Math.round(c.to[key])+'°':Math.round(c.to[key]*100)+'%';
    }
    cancelAnimationFrame(layoutFrame);updateLayout();
  }
  page.querySelectorAll('[data-frame-panel]').forEach(control=>control.addEventListener('input',()=>setCamera(control.dataset.framePanel,{[control.dataset.frameField]:Number(control.value)/(['zoom','yaw'].includes(control.dataset.frameField)?1:100)})));
  page.querySelectorAll('[data-device-shot]').forEach(button=>button.addEventListener('click',()=>setCamera('device',button.dataset.deviceShot==='knobs'?{zoom:2.2,x:0,y:-.65}:button.dataset.deviceShot==='pads'?{zoom:2.3,x:0,y:.55}:{zoom:1,x:0,y:0,yaw:0})));
  page.querySelectorAll('[data-screen-angle]').forEach(button=>button.addEventListener('click',()=>setCamera('screen',{yaw:Number(button.dataset.screenAngle)})));
  $('#screen-framing-reset')?.addEventListener('click',()=>setCamera('screen',{zoom:1,x:0,y:0,yaw:0}));
  page.querySelectorAll('[data-tutorial-tab]').forEach(button=>button.addEventListener('click',()=>{
    page.querySelectorAll('[data-tutorial-tab]').forEach(tab=>tab.setAttribute('aria-selected',String(tab===button)));
    page.querySelectorAll('[data-tutorial-panel]').forEach(panel=>panel.hidden=panel.dataset.tutorialPanel!==button.dataset.tutorialTab);
  }));
  $('#framing-toggle')?.addEventListener('click',()=>{
    const panel=$('#tutorial-framing');panel.hidden=!panel.hidden;$('#framing-toggle').setAttribute('aria-expanded',String(!panel.hidden));
  });
  const layoutNow=()=>presentation.interpolate(layoutFrom,layoutTo,(performance.now()-layoutAt)/450);
  function updateLayout(){
    if(!stage)return;
    const pose=layoutNow(),screen=stage.querySelector('.tutorial-display'),hardware=stage.querySelector('.tutorial-hardware');
    pose.screen=presentation.safeScreenBox(pose.screen,cameraNow('screen').yaw);
    stage.classList.add('presentation-stage');
    const photo=$('#device-style')?.value==='photo'&&$('#device-photo')?.naturalWidth;
    const size=photo?[window.WorkshopPhotoLayout.width,window.WorkshopPhotoLayout.height]:[400,500];
    for(const [element,box,alpha] of [[screen,pose.screen,pose.screen.opacity],[hardware,pose.device,pose.device.opacity]]){
      element.style.left=box.x/1280*100+'%';element.style.top=box.y/720*100+'%';
      element.style.width=box.width/1280*100+'%';element.style.height=box.height/720*100+'%';
      element.style.opacity=alpha;element.style.visibility=alpha<.001?'hidden':'visible';
    }
    for(const [element,box,native,panel] of [[screen.querySelector('#live-canvas'),pose.screen,[4,3],'screen'],[hardware.querySelector('.mpc-device'),pose.device,size,'device']]){
      const crop=presentation.framing(box,native[0],native[1],cameraNow(panel));
      element.style.left=(crop.x-box.x)/box.width*100+'%';element.style.top=(crop.y-box.y)/box.height*100+'%';
      element.style.width=crop.width/box.width*100+'%';element.style.height=crop.height/box.height*100+'%';
    }
    screen.querySelector('.tutorial-screen-viewport').style.transform='perspective('+(stage.clientWidth/1280*pose.screen.width*2)+'px) rotateY('+cameraNow('screen').yaw+'deg)';
    if(performance.now()-layoutAt<450||Object.values(cameras).some(c=>performance.now()-c.at<220))layoutFrame=requestAnimationFrame(updateLayout);
  }

  page.querySelectorAll('[data-record-view]').forEach(button=>button.addEventListener('click',()=>{
    layoutFrom=layoutNow();layoutTo=presentation.layout(button.dataset.recordView);layoutAt=performance.now();
    page.querySelectorAll('[data-record-view]').forEach(item=>item.setAttribute('aria-pressed',String(item===button)));
    cancelAnimationFrame(layoutFrame);updateLayout();
  }));
  $('#device-style')?.addEventListener('change',()=>requestAnimationFrame(updateLayout));
  $('#device-photo')?.addEventListener('load',()=>requestAnimationFrame(updateLayout));
  for(const [selector,panel] of [['.tutorial-hardware-viewport','device'],['.tutorial-screen-viewport','screen']]){
    const viewport=page.querySelector(selector);if(!viewport)continue;
    let drag=null;
    viewport.addEventListener('pointerdown',event=>{
      if(cameras[panel].to.zoom<=1)return;
      drag={x:event.clientX,y:event.clientY,camera:{...cameras[panel].to}};
      viewport.setPointerCapture(event.pointerId);event.preventDefault();
    });
    viewport.addEventListener('pointermove',event=>{
      if(!drag)return;const box=viewport.getBoundingClientRect();
      const scale=Math.max(.1,drag.camera.zoom-1);
      setCamera(panel,{x:Math.max(-1,Math.min(1,drag.camera.x-(event.clientX-drag.x)*2/(box.width*scale))),y:Math.max(-1,Math.min(1,drag.camera.y-(event.clientY-drag.y)*2/(box.height*scale)))});
    });
    for(const type of ['pointerup','pointercancel','lostpointercapture'])viewport.addEventListener(type,()=>{drag=null;});
  }
  updateLayout();
  function drawRecording() {
    const paint = recordingCanvas.getContext("2d");
    paint.setTransform(recordingCanvas.width/1280,0,0,recordingCanvas.height/720,0,0);
    // Each exported frame starts clean; hidden panels must not survive a fade.
    paint.globalAlpha=1;paint.globalCompositeOperation="source-over";
    paint.clearRect(0,0,1280,720);
    const studio=scene?.value==="studio";
    if(!window.WorkshopScenes.draw(paint,scene?.value,sceneTime(),sceneInput()))presentation.background(paint,studio,$("#tutorial-background")?.value||"#101527");
    const backdrop=$("#custom-background");
    if(scene?.value==="custom"&&backdrop?.complete&&backdrop.naturalWidth){
      const fit=presentation.imagePlacement(backdrop.naturalWidth,backdrop.naturalHeight,$("#background-fit")?.value);
      paint.imageSmoothingEnabled=true;paint.imageSmoothingQuality="high";paint.drawImage(backdrop,fit.x,fit.y,fit.width,fit.height);
      const shade=paint.createLinearGradient(0,0,0,96);shade.addColorStop(0,"#0008");shade.addColorStop(1,"#0000");paint.fillStyle=shade;paint.fillRect(0,0,1280,96);
    }
    paint.fillStyle = studio?"#4b515a":"#8ac9c8"; paint.font = "600 18px system-ui";
    const customLogo = $("#tutorial-logo");
    if (customLogo && !customLogo.hidden && customLogo.complete && customLogo.naturalWidth) {
      const scale = Math.min(360/customLogo.naturalWidth,52/customLogo.naturalHeight);
      paint.imageSmoothingEnabled = true;
      paint.drawImage(customLogo,24,8,customLogo.naturalWidth*scale,customLogo.naturalHeight*scale);
    } else paint.fillText("TAPE WORKSHOP",24,35);
    paint.textAlign="right";paint.font="600 14px system-ui";
    paint.fillText("MPC LIVE DISPLAY",1256,38);paint.textAlign="left";
    if(waveformToggle?.checked){const data=waveformData();window.WorkshopScenes.waveform(paint,data[0],data[1],{x:24,y:68,width:1232,height:22},studio);}
    paint.imageSmoothingEnabled = true;paint.imageSmoothingQuality="high";
    // Shared protected header: camera crops and projected bezels stay below it.
    paint.save();paint.beginPath();paint.rect(0,96,1280,624);paint.clip();
    if ($("#device-screen")) {
      const pose=layoutNow(),box=pose.device;
      pose.screen=presentation.safeScreenBox(pose.screen,cameraNow('screen').yaw);
      if(pose.screen.opacity>.001){
        if(studio)presentation.contactShadow(paint,pose.screen);
        paint.save();paint.globalAlpha=pose.screen.opacity;
        presentation.screen(paint,canvas,pose.screen,$("#screen-frame")?.value==="tablet",cameraNow('screen'));paint.restore();
      }
      if(box.opacity>.001){
      if(studio)presentation.contactShadow(paint,box);
      paint.save();paint.globalAlpha=box.opacity;
      paint.beginPath();paint.rect(box.x,box.y,box.width,box.height);paint.clip();
      const pixel = $("#device-style").value === "pixel";
      const photo = $("#device-style").value === "photo" && $("#device-photo")?.naturalWidth ? $("#device-photo") : null;
      const crop=presentation.framing(box,photo?window.WorkshopPhotoLayout.width:400,photo?window.WorkshopPhotoLayout.height:500,cameraNow('device'));
      const illustration = pixel ? $("#pixel-device") : devicePicture;
      if (photo) window.WorkshopPhotoLayout.render(paint,photo,page.querySelector(".mpc-device"),canvas,crop,$("#screen-rendering")?.value!=="original");
      else {
        paint.imageSmoothingEnabled=!pixel;
        if (illustration) paint.drawImage(illustration,crop.x,crop.y,crop.width,crop.height);
        paint.imageSmoothingEnabled=$("#screen-rendering")?.value!=="original";
        paint.drawImage(canvas,crop.x+crop.width*.37,crop.y+crop.height*.1,crop.width*.27,crop.height*.162);
      }
      paint.restore();
      }
      paint.fillStyle = studio?"#4b515a":"#f1c19e"; paint.font = "600 24px system-ui";
      paint.fillText($("#tutorial-action").textContent,24,681);
    } else {
      const box={x:300,y:112,width:680,height:510,opacity:1};
      if(studio)presentation.contactShadow(paint,box);
      presentation.screen(paint,canvas,box,$("#screen-frame")?.value==="tablet",{smooth:$("#screen-rendering")?.value!=="original"});
    }
    paint.restore();
  }
  function stopRecording(note = "") {
    if (!isRecording()) return;
    updateRecordingClock(recordClock.elapsed(performance.now()),"STOPPED");
    recordingNote = note; recordPause.disabled=true; recorder.stop(); recordStop.disabled = true;
  }
  function updateRecordPause(){
    const held=recorder?.state==="paused";
    recordPause.textContent=held?"Resume recording":"Pause recording";
    recordPause.setAttribute("aria-pressed",String(held));
  }
  recordPause.addEventListener("click",()=>{
    if(recordPause.disabled||!isRecording())return;
    try{
      if(recorder.state==="recording")recorder.pause();
      else if(recorder.state==="paused")recorder.resume();
    }catch(error){recordState.textContent="Could not pause or resume: "+error.message;}
  });
  recordStop.addEventListener("click", () => stopRecording());
  recordStart.addEventListener("click", async () => {
    if (isRecording()) return;
    if (!window.MediaRecorder || !HTMLCanvasElement.prototype.captureStream) {
      recordState.textContent = "This browser cannot record a combined video. Enable MPC audio and use your device’s screen recorder."; return;
    }
    if (!screenshot) { recordState.textContent = "Wait for the live screen, then try Record Tutorial."; return; }
    if(scene?.value==="custom"&&!$("#custom-background")?.naturalWidth){recordState.textContent="Upload a background image first, or choose another scene.";return;}
    recordStart.disabled = true;
    let pendingStore=null;
    try {
      await enableAudio();
      const waitAt = performance.now();
      while (audioOn && audioOrigin === null && performance.now() - waitAt < 5000) await new Promise(resolve => setTimeout(resolve,50));
      if (!audioOn || audioOrigin === null) throw new Error("No MPC audio yet. Try enabling audio again.");
      const fullHD = $("#record-quality")?.value === "1080";
      let store=null;
      try{store=await window.WorkshopRecordingStore.open();}catch{}
      const disk=!!store;
      pendingStore=store;
      if(disk&&navigator.storage?.estimate){const space=await navigator.storage.estimate().catch(()=>null);const oneHour=(fullHD?4128000:2128000)/8*3600;
        if(space&&Number.isFinite(space.quota)&&space.quota-(space.usage||0)<oneHour*1.15)throw Error('Not enough browser storage for an hour at this quality. Free storage or select 720p.');}

      recordingCanvas = document.createElement("canvas"); recordingCanvas.width=fullHD?1920:1280; recordingCanvas.height=fullHD?1080:720;
      drawRecording();
      const video = recordingCanvas.captureStream(30);recordingVideo=video;
      const combined = new MediaStream([...video.getVideoTracks(), ...audioDestination.stream.getAudioTracks()]);
      const mime = ["video/mp4;codecs=avc1.42E028,mp4a.40.2", "video/mp4;codecs=avc1,mp4a.40.2", "video/webm;codecs=vp8,opus", "video/webm;codecs=vp9,opus", "video/mp4", "video/webm"].find(type => MediaRecorder.isTypeSupported(type));
      if (!mime) { video.getTracks().forEach(track => track.stop()); throw new Error("This browser has no supported video recorder. Enable audio and use your device’s screen recorder."); }
      recorder = new MediaRecorder(combined,{mimeType:mime,videoBitsPerSecond:fullHD?4000000:2000000,audioBitsPerSecond:128000});
      recordingStore?.dispose();recordingStore=store;pendingStore=null;store?.activate();
      let parts=[], bytes=0, displayedSecond=-1; const policy=window.WorkshopRecordingPolicy; recordingNote="";
      const previousPreview = $("#record-preview"); previousPreview.pause(); previousPreview.removeAttribute("src"); previousPreview.load(); previousPreview.hidden=true;
      if (recordingUrl) URL.revokeObjectURL(recordingUrl);
      recordingDownload.hidden=true;
      recorder.ondataavailable = event => {
        if (event.data.size) { if(store)store.add(event.data);else parts.push(event.data); bytes += event.data.size; }
        const reason=policy.limit(recordClock.elapsed(performance.now()),bytes,disk); if(reason) stopRecording(reason);
      };
      recorder.onpause=()=>{recordClock.pause(performance.now());updateRecordPause();updateRecordingClock(recordClock.elapsed(performance.now()),"PAUSED");};
      recorder.onresume=()=>{recordClock.resume(performance.now());updateRecordPause();updateRecordingClock(recordClock.elapsed(performance.now()),"RECORDING");};
      recorder.onerror = () => stopRecording("The browser recorder reported an error; check this take.");
      recorder.onstop = async () => {
        cancelAnimationFrame(recordTimer); video.getTracks().forEach(track => track.stop());
        recordPause.disabled=true; updateRecordPause();
        recordState.textContent='Finishing recording…';
        try{
          if(store)await store.settle();
          const mimeType=recorder.mimeType;
          recordingDownload.download="Tape-Workshop-Tutorial."+(mimeType.includes("mp4")?"mp4":"webm");
          // Stream large takes directly to Save As where supported. Other browsers
          // assemble immutable disk-backed Blob references for a single download.
          let blob=null;
          if(!store||!window.showSaveFilePicker||bytes<768*1024*1024)blob=store?await store.blob(mimeType):new Blob(parts,{type:mimeType});
          parts=[];
          if(blob){recordingUrl=URL.createObjectURL(blob);recordingDownload.href=recordingUrl;}
          else recordingDownload.href='#recording-ready';
          recordingDownload.hidden=!bytes;
          const preview=$("#record-preview");preview.hidden=!blob||bytes>=768*1024*1024;
          if(blob&&!preview.hidden){preview.src=recordingUrl;preview.defaultPlaybackRate=1;preview.playbackRate=1;}
          recordState.textContent=(recordingNote?recordingNote+' ':'')+(bytes?'Tap Save recording to choose where to keep this single video. Large takes may take time to save.':'No recording was produced.');
        }catch(error){recordState.textContent='Could not prepare this take: '+error.message;}
        recordingVideo=null;
        if($('#device-style'))$('#device-style').disabled=false;
        recordStart.disabled=false; recordStop.disabled=true; pause.disabled=false;
      };
      if(store)store.onfailure=message=>stopRecording(message);
      recordClock=policy.createClock(performance.now());
      recorder.start(1000);updateRecordPause();updateRecordingClock(0,"RECORDING");
      recordPause.disabled=typeof recorder.pause!=="function"||typeof recorder.resume!=="function";
      refreshDevicePicture(); recordStop.disabled=false; pause.disabled=true;
      recordState.textContent=disk?"Recording to browser disk buffer. Maximum take: 2 hours. Keep this page visible.":"Disk buffering unavailable: short mode (20 minutes / 768 MB). Keep this page visible.";
      let drawnAt=0;
      function recordFrame(now) {
        if(!isRecording())return;
        recordTimer=requestAnimationFrame(recordFrame);
        if(now-drawnAt<1000/30)return;
        drawnAt=now-((now-drawnAt)%(1000/30));
        drawRecording();
        const elapsed=recordClock.elapsed(performance.now()), reason=policy.limit(elapsed,bytes,disk);
        if(reason) { stopRecording(reason); return; }
        const second=recorder.state+":"+Math.floor(elapsed/1000);
        if(second!==displayedSecond){
          displayedSecond=second;
          updateRecordingClock(elapsed,recorder.state==="paused"?"PAUSED":"RECORDING");
          recordState.textContent=(recorder.state==="paused"?"Recording paused · ":"Recording ")+policy.clock(elapsed)+(disk?" / 2:00:00":" / 20:00")+" · video + direct MPC audio. Keep this page visible.";
        }
      }
      recordTimer=requestAnimationFrame(recordFrame);
    } catch(error) {
      pendingStore?.dispose();
      recordingVideo?.getVideoTracks().forEach(track=>track.stop());recordingVideo=null;
      if(!audioOn)await stopAudio();
      recordPause.disabled=true;
      recordState.textContent=error.message||'Recording could not start.';recordStart.disabled=false;
    }
  });
  window.addEventListener("pagehide", () => { if (audioOn) stopAudio(); });
  if ($("#device-screen")) {
    const device = page.querySelector(".mpc-device"), style = $("#device-style");
    const pixels = $("#pixel-device"), pixelPaint = pixels.getContext("2d");
    let paintingPixel = false;
    function drawPixelDevice() {
      if(style.value === "photo" && $("#device-photo")?.naturalWidth) { devicePicture=null; return; }
      if ((style.value === "detailed" && !isRecording()) || paintingPixel) return;
      paintingPixel = true;
      const svg = device.querySelector("svg").cloneNode(true);
      svg.querySelector(".photo-knob-motion")?.remove();
      const photoMode=style.value === "photo" && !!$("#device-photo")?.naturalWidth;
      svg.setAttribute("width", String(photoMode?window.WorkshopPhotoLayout.width:400)); svg.setAttribute("height", String(photoMode?window.WorkshopPhotoLayout.height:500));
      const rules = document.createElementNS("http://www.w3.org/2000/svg", "style");
      rules.textContent = `
        .mpc-brand{fill:#d75c4c;font:bold 17px Georgia,serif}
        .mpc-model{fill:#d75c4c;font:bold 8px system-ui}
        .hardware-label{text-anchor:middle;fill:#263645;font:600 7px system-ui}
        .button-label{text-anchor:middle;fill:#f3f0df;font:600 5.5px system-ui}
        .pad-label{text-anchor:middle;fill:#e3e9ea;font:600 11px system-ui}
        .hardware-button.active rect,.hardware-pad.active rect{fill:#e3bc7e;stroke:#fff3c2;stroke-width:3}
        .hardware-button.active text,.hardware-pad.active text{fill:#101527}
        .hardware-knob.active circle,.hardware-knob.active line,.wheel-push.active circle{stroke:#f3bd6c;stroke-width:4}
      `;
      if (style.value === "photo" && $("#device-photo")?.naturalWidth) rules.textContent += `
        svg>*:not(g):not(defs){opacity:0} svg>g{opacity:0}
        svg>g.active{opacity:1} svg>g.active text{opacity:0}
        svg>g.live-indicators,svg>g.live-indicators text,svg>g.live-indicators rect{opacity:1}
        svg>g.active rect,svg>g.active circle{fill:#f1c19e;fill-opacity:.35;stroke:#ffe1ac;stroke-width:2}
      `;
      svg.insertBefore(rules, svg.firstChild);
      const url = URL.createObjectURL(new Blob([new XMLSerializer().serializeToString(svg)], {type:"image/svg+xml"}));
      const picture = new Image();
      picture.onload = () => {
        devicePicture = picture;
        pixelPaint.clearRect(0, 0, 100, 125);
        pixelPaint.imageSmoothingEnabled = false;
        pixelPaint.drawImage(picture, 0, 0, 100, 125);
        URL.revokeObjectURL(url); paintingPixel = false;
      };
      picture.onerror = () => { URL.revokeObjectURL(url); paintingPixel = false; };
      picture.src = url;
    }
    try { const savedStyle=localStorage.getItem("tape-workshop-device-style"); style.value = ["pixel","photo","detailed"].includes(savedStyle)?savedStyle:"photo";
      if(!localStorage.getItem("tape-workshop-product-photo-v82")){style.value="photo";localStorage.setItem("tape-workshop-product-photo-v82","yes");} } catch {}
    function chooseStyle() {
      const photoMap = style.value === "photo" && !!$("#device-photo")?.naturalWidth;
      window.WorkshopPhotoLayout.apply(device,photoMap);
      device.classList.toggle("pixel-art", style.value === "pixel");
      device.classList.toggle("photo-device", style.value === "photo" && !!$("#device-photo")?.naturalWidth);
      if($("#device-photo")) $("#device-photo").hidden=style.value !== "photo" || !$("#device-photo").naturalWidth;
      try { localStorage.setItem("tape-workshop-device-style", style.value); } catch {}
      drawPixelDevice();
      updateLayout();
    }
    refreshDevicePicture = drawPixelDevice;
    style.addEventListener("change", chooseStyle);
    // On a cold connection the product image often arrives after this script.
    // Reapply its shared screen/control map when ready; preserve the user's
    // current style if they switched to Pixel Art or Realistic while it loaded.
    $("#device-photo")?.addEventListener("load", chooseStyle);
    $("#device-photo")?.addEventListener("error", chooseStyle);
    chooseStyle();
    const names = ["Sample", "SEQ", "Pad FX", "Knob FX", "Shift", "Pad Bank", "Erase", "Note Repeat", "B1", "B2", "B3", "Chop", "Mute", "Loop", "16 Levels", "Sample Select", "Tap Tempo", "Minus", "Plus", "Sample Record", "SEQ Record", "Stop", "Play", "Data push"];
    const controls = ["Main volume", "K1", "K2", "K3", "Fader", "Data"];
    function updateMeters(){
      const levels=meterAnalysers.length?meterAnalysers.map((analyser,index)=>{analyser.getFloatTimeDomainData(meterArrays[index]);let peak=0;for(const value of meterArrays[index])peak=Math.max(peak,Math.abs(value));return Math.max(0,Math.min(1,(20*Math.log10(Math.max(peak,0.001))+60)/60));}):[0,0];
      window.WorkshopPhotoLayout.indicators(device,levels,meterBank);
    }
    let meterTimer=setInterval(updateMeters,80);
    window.addEventListener('pagehide',()=>clearInterval(meterTimer));
    window.addEventListener('pageshow',()=>{clearInterval(meterTimer);meterTimer=setInterval(updateMeters,80);updateMeters();});
    let previousHardwarePicture="";
    showHardware = input => {
            if(Number.isInteger(input.bank))meterBank=Math.max(0,Math.min(7,input.bank));
            let action = "", newest = 2000;
            input.buttons.forEach(([down, age], n) => {
              const el = page.querySelector(`[data-button="${n}"]`);
              el?.classList.toggle("active", !!down || age < 400);
              if (age < newest) { newest = age; action = names[n]; }
            });
            input.pads.forEach(([down, age], n) => {
              page.querySelector(`[data-pad="${n}"]`)?.classList.toggle("active", !!down || age < 400);
              if (age < newest) { newest = age; action = "Pad " + (n + 1); }
            });
            input.controls.forEach(([value, age], n) => {
              const el = page.querySelector(`[data-control="${n}"]`);
              el?.classList.toggle("active", age < 500);
              if(el) { el.dataset.value=String(value); if(n===5)el.dataset.turn=String(input.wheel); }
              const pointer = el?.querySelector(".knob-pointer");
              if (pointer && (value >= 0 || n === 5)) {
                const angle = n === 5 ? input.wheel * 18 : -135 + value / 127 * 270;
                pointer.setAttribute("transform", `rotate(${angle} ${pointer.dataset.cx} ${pointer.dataset.cy})`);
              }
              if (n === 4 && value >= 0) $("#hardware-fader").setAttribute("y", String(355 - value / 127 * 78));
              if (age < newest) { newest = age; action = controls[n] + (n === 5 ? " turned" : " · " + value); }
            });
            if(device.classList.contains("photo-device"))window.WorkshopPhotoLayout.updateMotion(device);
            const signature=JSON.stringify([input.buttons.map(([down,age])=>!!down||age<400),input.pads.map(([down,age])=>!!down||age<400),input.controls.map(([value,age])=>[value,age<500]),input.wheel]);
            if(signature!==previousHardwarePicture){previousHardwarePicture=signature;drawPixelDevice();}
            if (action) {
              const shift = input.buttons[4][0] && action !== "Shift" ? "Shift + " : "";
              $("#tutorial-action").textContent = shift + action;
            }
    };
    async function hardware() {
      if (!paused && !document.hidden && !sending) {
        const stop = new AbortController();
        const timeout = setTimeout(() => stop.abort(), 3000);
        try {
          const response = await fetch("/surface", {cache:"no-store", signal:stop.signal});
          if (!response.ok) throw new Error("No hardware");
          const input = await response.json();
          if (!paused && !document.hidden) {
            if (audioOn && audioOrigin !== null && input.rate === audioRate) {
              input.received=performance.now();
              hardwareQueue.push(input);
              if (hardwareQueue.length > 40) hardwareQueue.shift();
            } else showHardware(input);
          }
        } catch { /* The live-screen connection label handles reconnecting. */ }
        finally { clearTimeout(timeout); }
      }
      setTimeout(hardware, 100);
    }
    hardware();
  }
  schedule(0);
})();
