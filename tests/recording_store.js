// SPDX-License-Identifier: AGPL-3.0-only
// Install fake-indexeddb in an ignored test folder; pass its package directory as argv[2].
const assert=require('node:assert/strict');
global.indexedDB=require(process.argv[2]).indexedDB;
const {RecordingStore,pendingLimit}=require('../src/melodee/web/static/recording-store.js');
(async()=>{
 const store=await RecordingStore.open();const expected=[];
 for(let i=0;i<7200;++i){const b=new Blob([String(i).padStart(4,'0')]);expected.push(await b.text());assert(store.add(b));}
 await store.settle();assert.equal(store.pending,0);assert.equal(store.count,7200);assert.equal(store.fallback.length,0);
 assert.equal(await (await store.blob('video/webm')).text(),expected.join(''));
 let saved='';let closed=false;await store.writeTo({write:async blob=>{saved+=await blob.text();},close:async()=>{closed=true;}});
 assert.equal(saved,expected.join(''));assert(closed);store.dispose();
 const fail=await RecordingStore.open();fail.add(new Blob(['start']));await fail.settle();
 fail.db.transaction=()=>{throw Error('Simulated storage full');};let errors=0;fail.onfailure=()=>++errors;
 fail.add(new Blob(['middle']));fail.add(new Blob(['end']));await fail.settle();
 assert.equal(errors,1);assert.equal(fail.count,1);assert.equal(fail.fallback.length,2);
 delete fail.db.transaction;
 assert.equal(await (await fail.blob('video/mp4')).text(),'startmiddleend');fail.dispose();
 const slow=await RecordingStore.open();let stopped=false;slow.onfailure=()=>stopped=true;
 assert(!slow.add({size:pendingLimit+1}));assert(stopped);await slow.settle();assert.equal(slow.fallback.length,1);slow.dispose();
 console.log('PASS: 7,200 ordered chunks, single-Blob and streamed saving, storage-error preservation and pending-buffer guard');
})().catch(error=>{console.error(error);process.exitCode=1;});
