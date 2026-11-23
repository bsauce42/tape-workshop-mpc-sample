// SPDX-License-Identifier: AGPL-3.0-only
const assert=require('node:assert/strict');const edit=require('../src/melodee/web/static/studio-edit-core.js');
function run(){const state={track:0,tracks:[{drum:true}],length:384,totalNotes:1,revision:'original',notes:[[0,24,24,36,100,45,0,-2,3,0,0,0,0]]};
 const pending=edit.preview(state,[{kind:'update',index:0,tick:48,length:24,key:36,velocity:95}]);assert.equal(state.notes[0][1],24);assert.equal(pending.state.notes[0][1],48);assert.equal(pending.state.notes[0][8],3);assert.equal(pending.state.revision,'original');assert.deepEqual(pending.selected,[0]);
 const add=edit.preview(state,[{kind:'add',tick:96,length:24,key:38,velocity:80}]);assert.equal(add.state.notes[1][0],-1);assert.equal(add.state.totalNotes,2);assert.equal(state.totalNotes,1);
 assert.throws(()=>edit.preview(state,[{kind:'update',index:0,tick:380,length:24,key:36,velocity:95}]));assert.throws(()=>edit.preview(state,[{kind:'add',tick:0,length:24,key:35,velocity:95}]));assert.equal(state.notes[0][1],24);
 return 'PASS: immediate note previews preserve confirmed revision/play metadata, temporary IDs and rollback-safe originals; invalid edits rejected';}
if(require.main===module)console.log(run());module.exports=run;
