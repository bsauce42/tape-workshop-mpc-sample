// SPDX-License-Identifier: AGPL-3.0-only
// Run with node; external ZIP extraction/CRC verification complements this check.
const core=require('../src/melodee/web/static/multicam-recording.js');
(async()=>{
    const archive=await core.zip([{name:'MPC.mp4',data:new Blob([new Uint8Array([1,2,3])])},{name:'README.txt',data:'Editable background'}]);
    const bytes=await archive.arrayBuffer(),view=new DataView(bytes);
    if(view.getUint32(0,true)!==0x04034b50||view.getUint32(bytes.byteLength-22,true)!==0x06054b50)throw Error('Invalid ZIP envelope');
    if(view.getUint16(bytes.byteLength-12,true)!==2)throw Error('Missing recording files');
    let rejected=false;try{await core.zip([{name:'../unsafe',data:'x'}]);}catch{rejected=true;}
    if(!rejected)throw Error('Unsafe archive path accepted');
})().catch(error=>{throw error;});
