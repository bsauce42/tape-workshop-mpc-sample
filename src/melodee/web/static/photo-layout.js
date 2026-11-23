// SPDX-License-Identifier: AGPL-3.0-only
// The supplied photo, DOM overlays and recorded video share these native-image coordinates.
(function(root){
    'use strict';
    const width=1199,height=1312;
    const screen={x:452,y:134,width:294,height:202};
    const buttons=[
        [85,405,91,46],[202,405,91,46],[84,475,91,47],[202,475,91,47],
        [83,566,94,77],[202,567,90,48],[80,1018,93,55],[199,1018,94,55],
        [464,63,78,45],[566,63,78,45],[667,63,78,45],
        [909,404,94,47],[1030,404,94,47],[909,475,94,47],[1030,475,94,47],
        [909,566,94,48],[1030,566,94,48],[910,831,94,49],[1031,831,94,49],
        [910,924,94,62],[1031,924,94,62],[910,1016,94,62],[1031,1016,94,62]
    ];
    const knobs=[[180,218,64],[394,457,62],[600,457,62],[805,457,62],null,[1015,742,66]];
    function pad(index){return {x:335+index%4*138,y:550+(3-Math.floor(index/4))*137,width:119,height:111};}
    function fit(sourceWidth,sourceHeight,box){
        const scale=Math.min(box.width/sourceWidth,box.height/sourceHeight);
        const w=sourceWidth*scale,h=sourceHeight*scale;
        return {x:box.x+(box.width-w)/2,y:box.y+(box.height-h)/2,width:w,height:h,scale};
    }
    function placement(box){
        const photo=fit(width,height,box),slot={x:photo.x+screen.x*photo.scale,y:photo.y+screen.y*photo.scale,width:screen.width*photo.scale,height:screen.height*photo.scale};
        return {photo,slot,display:fit(4,3,slot)};
    }
    // Match the recorder app: cover the photographed knob's static pointer,
    // then draw a physical-looking indicator at the last known hardware value.
    function knobPose(index,value,turn){
        const knob=knobs[index];
        if(!knob || (index===5?!Number.isFinite(turn):!Number.isFinite(value)||value<0))return null;
        const angle=(index===5?(turn%20)*18:-135+Math.min(127,value)/127*270)*Math.PI/180;
        return {x:knob[0],y:knob[1],radius:37,angle,
            x1:knob[0]+Math.sin(angle)*16,y1:knob[1]-Math.cos(angle)*16,
            x2:knob[0]+Math.sin(angle)*31,y2:knob[1]-Math.cos(angle)*31};
    }
    function poses(device){
        return [...device.querySelectorAll('svg>g[data-control]')].map(group=>({index:+group.dataset.control,
            pose:knobPose(+group.dataset.control,Number(group.dataset.value??NaN),Number(group.dataset.turn??NaN))}));
    }
    function updateMotion(device){
        const svg=device.querySelector('svg');
        let layer=svg.querySelector('.photo-knob-motion');
        if(!layer){
            layer=document.createElementNS('http://www.w3.org/2000/svg','g');
            layer.setAttribute('class','photo-knob-motion');layer.setAttribute('aria-hidden','true');
            layer.setAttribute('pointer-events','none');svg.appendChild(layer);
            for(const index of [0,1,2,3,5]){
                const rotor=document.createElementNS('http://www.w3.org/2000/svg','g');rotor.dataset.knob=String(index);
                const face=document.createElementNS('http://www.w3.org/2000/svg','circle');
                face.setAttribute('r','37');face.setAttribute('fill','#ababab');face.setAttribute('stroke','#d4cdbb');face.setAttribute('stroke-width','2');
                const pointer=document.createElementNS('http://www.w3.org/2000/svg','line');
                pointer.setAttribute('stroke','#263645');pointer.setAttribute('stroke-width','8');pointer.setAttribute('stroke-linecap','round');
                rotor.append(face,pointer);layer.appendChild(rotor);
            }
        }
        for(const {index,pose} of poses(device)){
            const rotor=layer.querySelector(`[data-knob="${index}"]`);if(!rotor)continue;
            rotor.setAttribute('display',pose?'inline':'none');if(!pose)continue;
            const face=rotor.querySelector('circle'),pointer=rotor.querySelector('line');
            face.setAttribute('cx',pose.x);face.setAttribute('cy',pose.y);
            for(const key of ['x1','y1','x2','y2'])pointer.setAttribute(key,pose[key]);
        }
    }
    function indicators(device,levels,bank){
        device.dataset.audioLevels=JSON.stringify(levels);device.dataset.bank=String(bank);
        const svg=device.querySelector('svg');let layer=svg.querySelector('.live-indicators');
        if(!layer){layer=document.createElementNS('http://www.w3.org/2000/svg','g');layer.setAttribute('class','live-indicators');layer.setAttribute('pointer-events','none');svg.appendChild(layer);}
        const photo=device.classList.contains('photo-device'),x=photo?280:98,y=photo?213:88,scale=photo?2.6:1;
        layer.setAttribute('transform',`translate(${x} ${y}) scale(${scale})`);
        let content='';
        for(let n=0;n<8;n++)content+=`<text x="${n%4*11}" y="${Math.floor(n/4)*13}" font-family="sans-serif" font-size="9" font-weight="bold" fill="${n===bank?'#c778ff':'#88838e'}">${'ABCDEFGH'[n]}</text>`;
        for(let channel=0;channel<2;channel++)for(let n=0;n<12;n++)content+=`<rect x="${n*3.5}" y="${25+channel*6}" width="2.5" height="4" fill="${levels[channel]>n/12?(n>9?'#ff5353':n>7?'#ffcb55':'#44efb1'):'#24212a'}"/>`;
        layer.innerHTML=content;
    }
    function apply(device,on){
        const svg=device.querySelector('svg'),canvas=device.querySelector('#device-screen');
        device.style.aspectRatio=on?`${width}/${height}`:'';
        svg.setAttribute('viewBox',on?`0 0 ${width} ${height}`:'0 0 400 500');
        for(const key of ['left','top','width','height','object-fit','background','aspect-ratio'])canvas.style.removeProperty(key);
        if(on){
            canvas.style.left=screen.x/width*100+'%';canvas.style.top=screen.y/height*100+'%';
            canvas.style.width=screen.width/width*100+'%';canvas.style.height=screen.height/height*100+'%';
            canvas.style.objectFit='contain';canvas.style.background='#000';canvas.style.aspectRatio='auto';
        }
        device.querySelectorAll('svg>g[data-button],svg>g[data-pad],svg>g[data-control]').forEach(group=>{
            group.removeAttribute('transform');
            const fader=group.querySelector('#hardware-fader');if(fader)fader.setAttribute('height',on?'51':'14');
            if(!on)return;
            let target=null;
            if(group.dataset.pad!==undefined)target=pad(+group.dataset.pad);
            else if(group.dataset.button!==undefined && buttons[+group.dataset.button]){
                const b=buttons[+group.dataset.button];target={x:b[0],y:b[1],width:b[2],height:b[3]};
            }
            const rect=group.querySelector('rect');
            if(target&&rect){
                const sx=target.width/+rect.getAttribute('width'),sy=target.height/+rect.getAttribute('height');
                group.setAttribute('transform',`translate(${target.x-+rect.getAttribute('x')*sx} ${target.y-+rect.getAttribute('y')*sy}) scale(${sx} ${sy})`);
            }else{
                const index=group.dataset.button==='23'?5:+group.dataset.control,target=knobs[index],circle=group.querySelector('circle');
                if(target&&circle){
                    const scale=target[2]/+circle.getAttribute('r');
                    group.setAttribute('transform',`translate(${target[0]-+circle.getAttribute('cx')*scale} ${target[1]-+circle.getAttribute('cy')*scale}) scale(${scale})`);
                }else if(index===4)group.setAttribute('transform','translate(19.8 186.6153846) scale(2.6 1.846153846)');
            }
        });
        if(on)updateMotion(device);
    }
    function highlights(paint,device,box){
        paint.save();paint.translate(box.x,box.y);paint.scale(box.width/width,box.height/height);
        paint.fillStyle='rgba(138,201,200,.45)';paint.strokeStyle='#d6fffa';paint.lineWidth=3;
        for(const group of device.querySelectorAll('svg>g.active')){
            let rect=null,index;
            if(group.dataset.pad!==undefined)rect=pad(+group.dataset.pad);
            else if(group.dataset.button!==undefined&&buttons[+group.dataset.button]){
                const b=buttons[+group.dataset.button];rect={x:b[0],y:b[1],width:b[2],height:b[3]};
            }
            if(rect){paint.fillRect(rect.x,rect.y,rect.width,rect.height);paint.strokeRect(rect.x,rect.y,rect.width,rect.height);continue;}
            index=group.dataset.button==='23'?5:+group.dataset.control;
            const knob=knobs[index];
            if(knob){
                paint.beginPath();paint.arc(knob[0],knob[1],knob[2],0,Math.PI*2);paint.fill();paint.stroke();
                const value=+group.dataset.value;
                const angle=(index===5?(+group.dataset.turn||0)*18:-135+Math.max(0,value)/127*270)*Math.PI/180;
                paint.beginPath();paint.moveTo(knob[0],knob[1]);paint.lineTo(knob[0]+Math.sin(angle)*knob[2]*.8,knob[1]-Math.cos(angle)*knob[2]*.8);paint.stroke();
            }else if(index===4){
                const y=698+(1-Math.max(0,+group.dataset.value)/127)*144;
                paint.fillRect(155,y,62,94);paint.strokeRect(155,y,62,94);
            }
        }
        // Persistent knob positions also appear in recorded/exported frames.
        for(const {pose} of poses(device)){
            if(!pose)continue;
            paint.fillStyle='#ababab';paint.strokeStyle='#d4cdbb';paint.lineWidth=2;
            paint.beginPath();paint.arc(pose.x,pose.y,pose.radius,0,Math.PI*2);paint.fill();paint.stroke();
            paint.strokeStyle='#263645';paint.lineWidth=8;paint.lineCap='round';
            paint.beginPath();paint.moveTo(pose.x1,pose.y1);paint.lineTo(pose.x2,pose.y2);paint.stroke();
        }
        const levels=JSON.parse(device.dataset.audioLevels||'[0,0]'),bank=Number(device.dataset.bank||0);
        paint.font='bold 23px sans-serif';
        for(let n=0;n<8;n++){paint.fillStyle=n===bank?'#c778ff':'#88838e';paint.fillText('ABCDEFGH'[n],280+n%4*29,213+Math.floor(n/4)*34);}
        for(let channel=0;channel<2;channel++)for(let n=0;n<12;n++){paint.fillStyle=levels[channel]>n/12?(n>9?'#ff5353':n>7?'#ffcb55':'#44efb1'):'#24212a';paint.fillRect(280+n*9,278+channel*16,6,10);}
        paint.restore();
    }
    function render(paint,photo,overlay,live,box,smooth=true){
        const p=placement(box);
        paint.imageSmoothingEnabled=true;paint.drawImage(photo,p.photo.x,p.photo.y,p.photo.width,p.photo.height);
        if(overlay?.querySelectorAll)highlights(paint,overlay,p.photo);
        else if(overlay)paint.drawImage(overlay,p.photo.x,p.photo.y,p.photo.width,p.photo.height);
        paint.fillStyle='#000';paint.fillRect(p.slot.x,p.slot.y,p.slot.width,p.slot.height);
        paint.imageSmoothingEnabled=smooth;paint.imageSmoothingQuality="high";paint.drawImage(live,p.display.x,p.display.y,p.display.width,p.display.height);
    }
    const api={width,height,screen,buttons,knobs,pad,fit,placement,knobPose,updateMotion,indicators,apply,highlights,render};
    if(typeof module!=='undefined')module.exports=api;root.WorkshopPhotoLayout=api;
})(typeof window==='undefined'?globalThis:window);
