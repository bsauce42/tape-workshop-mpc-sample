// SPDX-License-Identifier: AGPL-3.0-only
(async () => {
    const assert = require('node:assert/strict'), fs = require('node:fs'), vm = require('node:vm');
    const document = {hidden: false}, messages = [], hits = [], pending = [];
    let state = {pad_hits: Array(16).fill(0)}, failure = false, requests = 0;
    const sandbox = {window: {}, document, AbortController, setTimeout: fn => {pending.push(fn);return pending.length;}, clearTimeout: () => {},
        fetch: async () => {requests++;if(failure)throw Error('Disconnected');return {ok:true,json:async()=>state};}};
    vm.runInNewContext(fs.readFileSync(require('node:path').join(__dirname,'../src/melodee/web/static/expansion-play.js'),'utf8'),sandbox);
    const controller = new sandbox.window.WorkshopMpcPads(i=>hits.push(i),s=>messages.push(s));
    controller.enabled = true; controller.generation = 1;
    state.pad_hits[2] = 5; await controller.poll(1); assert.equal(hits.length,0); // ignore pre-enable hits
    state = {pad_hits:[...state.pad_hits]};state.pad_hits[2]++;state.pad_hits[15]++;
    await controller.poll(1); assert.deepEqual(hits,[2,15]);
    await controller.poll(1);assert.equal(hits.length,2); // a held/unchanged pad does not retrigger
    failure=true;await controller.poll(1);failure=false;
    state={pad_hits:Array(16).fill(50)};await controller.poll(1);assert.equal(hits.length,2); // no reconnect burst
    document.hidden=true;let before=requests;await controller.poll(1);assert.equal(requests,before);
    document.hidden=false;await controller.poll(1);assert.equal(hits.length,2);
    state={pad_hits:Array(16).fill(0)};await controller.poll(1);assert.equal(hits.length,2); // device restarted
    controller.stop();before=requests;await controller.poll(1);assert.equal(requests,before);
    const sources = [], context = {currentTime:0,destination:{},resume:async()=>{},
        createBufferSource:()=>{const s={connect(){},disconnect(){},start(){s.started=true;},stop(){s.stopped=true;}};sources.push(s);return s;},
        createGain:()=>({connect(){},disconnect(){},gain:{value:1,cancelScheduledValues(){},setValueAtTime(){},linearRampToValueAtTime(){}}})};
    const player = new sandbox.window.WorkshopPadPlayer(()=>context);
    for(let i=0;i<20;i++)await player.play({});
    assert.equal(player.voices.length,16);assert.equal(sources.filter(s=>s.stopped).length,4);
    player.stop();assert.equal(player.voices.length,0);assert.ok(sources.every(s=>s.stopped));
    let resume;context.resume=()=>new Promise(resolve=>resume=resolve);
    const play=player.play({});player.stop();resume();await play;assert.equal(player.voices.length,0);
    console.log('PASS: MPC pad hit counters, no old/held/reconnect replay, hidden-page pause, disable, bounded preview voices and stop during audio unlock');
})();
