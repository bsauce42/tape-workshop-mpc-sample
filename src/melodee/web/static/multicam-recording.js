// SPDX-License-Identifier: AGPL-3.0-only
(function(root){
    'use strict';
    const encoder=new TextEncoder();
    const crcTable=Array.from({length:256},(_,n)=>{for(let k=0;k<8;k++)n=n&1?0xedb88320^(n>>>1):n>>>1;return n>>>0;});
    async function crc(blob){let value=0xffffffff;for(let at=0;at<blob.size;at+=262144)for(const b of new Uint8Array(await blob.slice(at,at+262144).arrayBuffer()))value=crcTable[(value^b)&255]^(value>>>8);return (value^0xffffffff)>>>0;}
    async function zip(files){
        const parts=[],index=[];let offset=0;
        for(const file of files){
            const name=encoder.encode(file.name),data=file.data instanceof Blob?file.data:new Blob([file.data]),sum=await crc(data);
            if(!file.name||file.name.includes('/')||file.name.includes('\\')||data.size>0xffffffff)throw Error('Invalid recording file');
            const local=new Uint8Array(30+name.length),v=new DataView(local.buffer);
            v.setUint32(0,0x04034b50,true);v.setUint16(4,20,true);v.setUint16(6,0x800,true);v.setUint16(12,33,true);v.setUint32(14,sum,true);v.setUint32(18,data.size,true);v.setUint32(22,data.size,true);v.setUint16(26,name.length,true);local.set(name,30);
            const central=new Uint8Array(46+name.length),c=new DataView(central.buffer);
            c.setUint32(0,0x02014b50,true);c.setUint16(4,20,true);c.setUint16(6,20,true);c.setUint16(8,0x800,true);c.setUint16(14,33,true);c.setUint32(16,sum,true);c.setUint32(20,data.size,true);c.setUint32(24,data.size,true);c.setUint16(28,name.length,true);c.setUint32(42,offset,true);central.set(name,46);
            parts.push(local,data);index.push(central);offset+=local.length+data.size;
        }
        const length=index.reduce((n,p)=>n+p.length,0),end=new Uint8Array(22),e=new DataView(end.buffer);
        e.setUint32(0,0x06054b50,true);e.setUint16(8,files.length,true);e.setUint16(10,files.length,true);e.setUint32(12,length,true);e.setUint32(16,offset,true);
        return new Blob([...parts,...index,end],{type:'application/zip'});
    }
    function png(canvas){return new Promise((resolve,reject)=>canvas.toBlob(blob=>blob?resolve(blob):reject(Error('Could not save recording mask')),'image/png'));}
    function create({width,height,audio,draw,masks,onError}){
        const mime=['video/mp4;codecs=avc1.42E028,mp4a.40.2','video/mp4;codecs=avc1,mp4a.40.2','video/mp4'].find(type=>MediaRecorder.isTypeSupported(type));
        if(!mime)throw Error('Final Cut recording needs MP4 support in this browser. Use Safari or Chrome on your Mac.');
        let bytes=0,stopped=false;const views=[];
        for(const name of ['MPC','Screen']){
            const canvas=document.createElement('canvas');canvas.width=width;canvas.height=height;
            const stream=canvas.captureStream(0),track=stream.getVideoTracks()[0];
            const recorder=new MediaRecorder(new MediaStream([track,...audio.getAudioTracks()]),{mimeType:mime,videoBitsPerSecond:name==='MPC'?1600000:1200000,audioBitsPerSecond:128000});
            let parts=[];
            const done=new Promise((resolve,reject)=>{
                recorder.ondataavailable=event=>{if(event.data.size){parts.push(event.data);bytes+=event.data.size;}};
                recorder.onstop=()=>{track.stop();const blob=new Blob(parts,{type:recorder.mimeType});parts=[];resolve(blob);};
                recorder.onerror=()=>{track.stop();reject(Error('Separate-angle recording failed. The combined take is still available.'));onError();};
            });
            done.catch(()=>{});views.push({name,canvas,track,recorder,done});
        }
        return {
            get bytes(){return bytes;},
            abort(){for(const v of views){if(v.recorder.state!=='inactive')v.recorder.stop();v.track.stop();}},
            start(){for(const v of views)v.recorder.start(1000);},
            frame(){for(const v of views){draw(v.canvas,v.name);v.track.requestFrame();}},
            async finish(background){
                if(stopped)throw Error('Already stopped');stopped=true;
                for(const v of views)if(v.recorder.state!=='inactive')v.recorder.stop();
                const clips=await Promise.all(views.map(v=>v.done));
                const files=views.map((v,n)=>({name:v.name+'.mp4',data:clips[n]}));
                for(const name of ['MPC','Screen'])files.push({name:name+'-Mask.png',data:await png(masks(name))});
                files.push({name:'Background.png',data:await png(background)});
                files.push({name:'README.txt',data:'Tape Workshop — Final Cut layers\n\nExtract this ZIP into a folder. Open Tape Workshop Final Cut Prep on your Mac and choose that folder. It creates transparent MOV layers and a Final Cut XML timeline. Import the resulting Tutorial.fcpxml in Final Cut Pro. The Background clip is separate: replace it with a color generator, image or movie. Choose the MPC or Screen angle in the multicam viewer. All source angles keep the full unit/screen so you can reframe later.\n\nMelodee by Kerem Kiliç (Ellic Studio).\n'});
                return zip(files);
            }
        };
    }
    const api={create,zip};if(typeof module!=='undefined')module.exports=api;root.WorkshopMulticam=api;
})(typeof window==='undefined'?globalThis:window);
