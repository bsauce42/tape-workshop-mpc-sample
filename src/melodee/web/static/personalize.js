// SPDX-License-Identifier: AGPL-3.0-only
'use strict';
(() => {
    if(new URLSearchParams(location.search).get('embedded')==='studio')document.documentElement.classList.add('studio-embedded');
    const $ = selector => document.querySelector(selector);
    const logo = $('#tutorial-logo');
    // Rasterize local images before use. The original is never uploaded to a service.
    async function picture(file) {
        if (!file || !/^image\/(png|jpeg|webp)$/i.test(file.type) || file.size > 8 * 1024 * 1024)
            throw Error('Choose a PNG, JPEG or WebP image smaller than 8 MB.');
        const url = URL.createObjectURL(file);
        const image = new Image();
        try {
            await new Promise((resolve, reject) => { image.onload = resolve; image.onerror = () => reject(Error('This image could not be opened.')); image.src = url; });
            if (!image.naturalWidth || !image.naturalHeight || image.naturalWidth * image.naturalHeight > 80000000)
                throw Error('Choose an image smaller than 80 megapixels.');
            return image;
        } finally { URL.revokeObjectURL(url); }
    }
    window.WorkshopPictures = {picture};
    if (logo) {
        const status = $('#tutorial-logo-status');
        const show = data => {
            logo.src = data; logo.hidden = !data;
            document.querySelector('.tutorial-layout').classList.toggle('has-custom-logo',!!data);
        };
        try { const data = localStorage.getItem('tape-workshop-tutorial-logo'); if (data?.startsWith('data:image/png;base64,') && data.length < 1500000) show(data); } catch {}
        $('#tutorial-logo-file').addEventListener('change', async event => {
            try {
                const image = await picture(event.target.files[0]);
                const canvas = document.createElement('canvas');
                const scale = Math.min(1, 720/image.naturalWidth, 200/image.naturalHeight);
                canvas.width = Math.max(1,Math.round(image.naturalWidth*scale));
                canvas.height = Math.max(1,Math.round(image.naturalHeight*scale));
                canvas.getContext('2d').drawImage(image,0,0,canvas.width,canvas.height);
                const data = canvas.toDataURL('image/png'); show(data);
                try { localStorage.setItem('tape-workshop-tutorial-logo',data); status.textContent = 'Logo added at top left. It is included in recordings and remembered in this browser.'; }
                catch { status.textContent = 'Logo added for this session. Browser storage is full.'; }
            } catch (error) { status.textContent = error.message; }
            event.target.value = '';
        });
        $('#tutorial-logo-remove').addEventListener('click', () => {
            show(''); try { localStorage.removeItem('tape-workshop-tutorial-logo'); } catch {}
            status.textContent = 'Logo removed.';
        });
    }
    const customBackground=$('#custom-background'),backgroundFile=$('#background-file');
    if(customBackground&&backgroundFile){
        const scene=$('#tutorial-scene'),fit=$('#background-fit'),status=$('#background-status'),remove=$('#background-remove');
        const key='tape-workshop-custom-background';
        function framing(){customBackground.style.objectFit=fit.value;try{localStorage.setItem(key+'-fit',fit.value);}catch{}}
        try{const saved=localStorage.getItem(key+'-fit');if(['cover','contain'].includes(saved))fit.value=saved;}catch{}
        fit.addEventListener('change',framing);framing();
        async function apply(data,select){
            const image=new Image();await new Promise((resolve,reject)=>{image.onload=resolve;image.onerror=()=>reject(Error('This background could not be opened.'));image.src=data;});
            customBackground.src=data;customBackground.hidden=false;remove.disabled=false;
            if(select){scene.value='custom';scene.dispatchEvent(new Event('change'));}
        }
        try{const data=localStorage.getItem(key);if(data?.startsWith('data:image/')&&data.length<5000000)apply(data,false).then(()=>{status.textContent='Saved background restored.';}).catch(error=>{status.textContent=error.message;});}catch{}
        backgroundFile.addEventListener('change',async event=>{
            const file=event.target.files[0];if(!file)return;
            backgroundFile.disabled=true;status.textContent='Opening background…';
            try{
                const image=await picture(file),canvas=document.createElement('canvas');
                const scale=Math.min(1,1920/image.naturalWidth,1080/image.naturalHeight);
                canvas.width=Math.max(1,Math.round(image.naturalWidth*scale));canvas.height=Math.max(1,Math.round(image.naturalHeight*scale));
                const paint=canvas.getContext('2d');paint.imageSmoothingQuality='high';paint.drawImage(image,0,0,canvas.width,canvas.height);
                const data=canvas.toDataURL('image/webp',.94);await apply(data,true);
                try{if(data.length>=5000000)throw Error('Large background');localStorage.setItem(key,data);status.textContent='Background added and saved in this browser. Included in recordings.';}
                catch{status.textContent='Background added for this session. Browser storage is full.';}
            }catch(error){status.textContent=error.message;}
            finally{backgroundFile.disabled=false;event.target.value='';}
        });
        remove.addEventListener('click',()=>{
            customBackground.removeAttribute('src');customBackground.hidden=true;remove.disabled=true;
            try{localStorage.removeItem(key);}catch{}
            if(scene.value==='custom'){scene.value='flat';scene.dispatchEvent(new Event('change'));}
            status.textContent='Background removed. Choose another image anytime.';
        });
    }
    const devicePhoto = $('#device-photo');
    if(devicePhoto){
        const ready=()=>$('#device-style').dispatchEvent(new Event('change'));
        devicePhoto.addEventListener('load',ready);
        if(devicePhoto.complete && devicePhoto.naturalWidth)ready();
        // v82's saved freehand screen offsets are intentionally no longer used.
        try{localStorage.removeItem('tape-workshop-photo-screen');localStorage.removeItem('tape-workshop-device-photo');}catch{}
    }
    const install = $('#home-install'), help = $('#home-install-help');
    let prompt = null;
    window.addEventListener('beforeinstallprompt', event => { event.preventDefault(); prompt = event; });
    install?.addEventListener('click', async () => {
        if (prompt) {
            await prompt.prompt(); const result = await prompt.userChoice; prompt = null;
            help.textContent = result.outcome === 'accepted' ? 'Home screen shortcut added.' : 'You can add the shortcut later from your browser menu.';
        } else {
            help.textContent = /iPhone|iPad|iPod/.test(navigator.userAgent) ?
                'In Safari: tap Share (the square with an up arrow), choose Add to Home Screen, then Add. Safari supplies the Tape Workshop icon automatically.' :
                'Open your browser menu and choose Add to Home Screen, Install app, or Create shortcut. On iPhone, use Safari → Share → Add to Home Screen. The MPC must be connected to open the companion.';
            help.scrollIntoView({block:'nearest',behavior:'smooth'});
        }
    });
})();
