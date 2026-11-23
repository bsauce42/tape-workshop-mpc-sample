// SPDX-License-Identifier: AGPL-3.0-only
(function(root){
    'use strict';
    const pendingLimit=64*1024*1024;
    class RecordingStore {
        constructor(db,name){this.db=db;this.name=name;this.chain=Promise.resolve();this.pending=0;this.count=0;this.bytes=0;this.fallback=[];this.error='';this.onfailure=null;this.ready=false;}
        static async open(){
            if(!root.indexedDB)throw Error('Browser disk buffering is unavailable.');
            const name='TapeWorkshopTake-'+Date.now()+'-'+Math.random().toString(36).slice(2);
            const db=await new Promise((resolve,reject)=>{const r=root.indexedDB.open(name,1);r.onupgradeneeded=()=>r.result.createObjectStore('chunks');r.onsuccess=()=>resolve(r.result);r.onerror=()=>reject(r.error);});
            return new RecordingStore(db,name);
        }
        activate(){try{const old=root.localStorage?.getItem('tw-active-take-buffer');if(old&&old!==this.name&&old.startsWith('TapeWorkshopTake-'))root.indexedDB.deleteDatabase(old);root.localStorage?.setItem('tw-active-take-buffer',this.name);}catch{}}
        fail(error){if(!this.error){this.error=error?.message||String(error);this.onfailure?.(this.error);}}
        add(blob){
            if(!blob.size)return true;
            this.bytes+=blob.size;this.pending+=blob.size;
            const within=this.pending<=pendingLimit;
            if(!within)this.fail(Error('Disk buffering fell behind. Recording stopped to keep the take recoverable.'));
            this.chain=this.chain.then(async()=>{
                if(this.error){this.fallback.push(blob);return;}
                try{await new Promise((resolve,reject)=>{const tx=this.db.transaction('chunks','readwrite');tx.objectStore('chunks').put(blob,this.count);tx.oncomplete=resolve;tx.onerror=tx.onabort=()=>reject(tx.error||Error('Could not buffer recording.'));});++this.count;}
                catch(error){this.fallback.push(blob);this.fail(error);}
            }).finally(()=>{this.pending-=blob.size;});
            return within;
        }
        async settle(){await this.chain;this.ready=true;}
        async read(index){return new Promise((resolve,reject)=>{const tx=this.db.transaction('chunks','readonly'),r=tx.objectStore('chunks').get(index);r.onsuccess=()=>r.result?resolve(r.result):reject(Error('A recording chunk is missing.'));r.onerror=()=>reject(r.error);});}
        async writeTo(writable,progress=()=>{}){await this.settle();let written=0;const write=async blob=>{await writable.write(blob);written+=blob.size;progress(this.bytes?written/this.bytes:1);};for(let n=0;n<this.count;++n)await write(await this.read(n));for(const blob of this.fallback)await write(blob);await writable.close();}
        async blob(type){await this.settle();const parts=[];for(let n=0;n<this.count;++n)parts.push(await this.read(n));parts.push(...this.fallback);return new Blob(parts,{type});}
        dispose(){try{if(root.localStorage?.getItem('tw-active-take-buffer')===this.name)root.localStorage.removeItem('tw-active-take-buffer');}catch{}this.db.close();root.indexedDB.deleteDatabase(this.name);this.fallback=[];}
    }
    if(typeof module!=='undefined')module.exports={RecordingStore,pendingLimit};
    root.WorkshopRecordingStore=RecordingStore;
})(typeof window==='undefined'?globalThis:window);
