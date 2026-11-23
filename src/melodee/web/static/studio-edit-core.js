// SPDX-License-Identifier: AGPL-3.0-only
(function(root){'use strict';
 function preview(state,changes){
  if(!state||!changes.length||changes.length>128)throw Error('Select 1–128 note changes per edit');
  const notes=state.notes.map(n=>n.slice()),touched=new Set(),selected=[];let temporary=-1;
  for(const c of changes){const at=notes.findIndex(n=>n[0]===c.index);if(c.kind!=='add'){if(at<0||touched.has(c.index))throw Error('Note changed. Refresh before editing.');touched.add(c.index);}if(c.kind==='delete'){notes.splice(at,1);continue;}
   const drum=state.tracks[state.track].drum;if(![c.tick,c.length,c.key,c.velocity].every(Number.isInteger)||c.tick<0||c.length<1||c.length>65535||c.tick+c.length>state.length||c.key<(drum?36:0)||c.key>(drum?163:127)||c.velocity<1||c.velocity>127)throw Error('Note is outside this pattern');
   const n=c.kind==='add'?[temporary--,c.tick,c.length,c.key,c.velocity,...(c.play||[100,0,0,1,0,0,0,0])]:notes[at];n[1]=c.tick;n[2]=c.length;n[3]=c.key;n[4]=c.velocity;if(c.kind==='update'&&c.play)n.splice(5,8,...c.play);if(c.kind==='add')notes.push(n);selected.push(n[0]);
  }
  return{state:{...state,notes,totalNotes:state.totalNotes+changes.filter(c=>c.kind==='add').length-changes.filter(c=>c.kind==='delete').length},selected};
 }
 const api={preview};if(typeof module!=='undefined'&&module.exports)module.exports=api;else root.WorkshopStudioEdit=api;
})(typeof window!=='undefined'?window:this);
