// SPDX-License-Identifier: AGPL-3.0-only
// Original Tape Workshop artwork. No external images, fonts, brands or requests.
(function(root){
    'use strict';
    const modes=['neon','warehouse','prism','sunset','space','arcade','scope','embers','ribbons','cubes','tunnel'],layers=new Map();
    function random(seed){let state=seed>>>0;return()=>{state=(Math.imul(state,1664525)+1013904223)>>>0;return state/4294967296;};}
    function gradient(p,x,y,r,color){
        const g=p.createRadialGradient(x,y,0,x,y,r);g.addColorStop(0,color);g.addColorStop(1,'transparent');return g;
    }
    function city(p){
        const r=random(7069),sky=p.createLinearGradient(0,0,0,720);
        sky.addColorStop(0,'#030413');sky.addColorStop(.7,'#16104a');sky.addColorStop(1,'#321044');p.fillStyle=sky;p.fillRect(0,0,1280,720);
        p.fillStyle=gradient(p,920,300,430,'#6526a650');p.fillRect(0,0,1280,720);
        for(let n=0;n<110;n++){p.fillStyle=n%3?'#8277b5':'#a8c8e4';p.fillRect(Math.floor(r()*1280),Math.floor(r()*340),n%4?1:2,1);}
        p.fillStyle='#b9c8e8';p.beginPath();p.arc(1010,118,34,0,Math.PI*2);p.fill();p.fillStyle='#07091c';p.beginPath();p.arc(1023,109,32,0,Math.PI*2);p.fill();
        for(let layer=0;layer<3;layer++){
            let x=-20;
            while(x<1300){
                const w=44+Math.floor(r()*74),h=100+Math.floor(r()*(140+layer*75)),y=590-h;
                p.fillStyle=['#11132e','#111a39','#091027'][layer];p.fillRect(x,y,w,h);
                p.fillStyle=layer===2?'#2f5594':'#303568';p.fillRect(x,y,w,3);
                if(r()>.5){p.fillStyle='#35417a';p.fillRect(x+w*.5,y-22,2,22);p.fillStyle='#de75d4';p.fillRect(x+w*.5,y-24,3,3);}
                for(let yy=y+13;yy<585;yy+=12)for(let xx=x+7;xx<x+w-5;xx+=9){
                    const lit=r();p.fillStyle=lit>.88?'#dd55a5':lit>.73?'#37a7bc':lit>.46?'#253b6e':'#0c132e';p.fillRect(xx,yy,4,3);
                }
                if(layer===2&&r()>.65){p.fillStyle='#4a8fc7';p.fillRect(x+3,y+12,2,h-12);}
                x+=w+8;
            }
        }
        p.fillStyle='#080b1c';p.fillRect(0,590,1280,130);
        const road=p.createLinearGradient(0,590,0,720);road.addColorStop(0,'#30204a');road.addColorStop(1,'#050918');p.fillStyle=road;p.fillRect(0,595,1280,125);
        p.strokeStyle='#574076';p.lineWidth=1;
        for(let x=-800;x<2100;x+=160){p.beginPath();p.moveTo(640+(x-640)*.15,595);p.lineTo(x,720);p.stroke();}
        for(const y of [605,622,650,695]){p.strokeStyle='#271e49';p.beginPath();p.moveTo(0,y);p.lineTo(1280,y);p.stroke();}
        for(const [x,y,w,h,c] of [[12,180,64,300,'#cc3bc5'],[1208,110,52,400,'#27c6d7'],[80,346,90,52,'#9851e3'],[1090,320,100,32,'#2aa9cb']]){
            p.fillStyle='#0a1026';p.fillRect(x,y,w,h);p.strokeStyle=c;p.lineWidth=2;p.strokeRect(x,y,w,h);
            p.fillStyle=c;for(let row=0;row<Math.floor(h/20);row++)p.fillRect(x+7,y+8+row*20,w-14,4);
        }
        p.fillStyle=gradient(p,100,610,180,'#d535b429');p.fillRect(0,430,400,290);
        p.fillStyle=gradient(p,1170,610,180,'#20c9d429');p.fillRect(950,430,330,290);
    }
    function warehouse(p){
        const wall=p.createLinearGradient(0,0,0,720);wall.addColorStop(0,'#11141c');wall.addColorStop(.67,'#30313a');wall.addColorStop(.68,'#20242b');wall.addColorStop(1,'#11131c');
        p.fillStyle=wall;p.fillRect(0,0,1280,720);
        p.fillStyle='#242832';p.beginPath();p.moveTo(0,0);p.lineTo(350,150);p.lineTo(930,150);p.lineTo(1280,0);p.fill();
        const r=random(4421);
        for(let y=120;y<490;y+=22)for(let x=0;x<1280;x+=62){p.fillStyle=`rgba(175,169,153,${.02+r()*.04})`;p.fillRect(x+(y%44?30:0),y,58,18);}
        p.strokeStyle='#080e16';p.lineWidth=12;
        for(const x of [30,260,510,770,1020,1250]){p.beginPath();p.moveTo(x,0);p.lineTo(640+(x-640)*.45,160);p.stroke();}
        for(const y of [45,100,150]){p.strokeStyle='#0b1119';p.lineWidth=9;p.beginPath();p.moveTo(0,y);p.lineTo(1280,y);p.stroke();}
        for(const x of [90,325,750,1000]){
            const light=p.createLinearGradient(x,155,x+130,350);light.addColorStop(0,'#cad5db');light.addColorStop(1,'#647785');
            p.fillStyle='#090f17';p.fillRect(x-7,145,144,163);p.fillStyle=light;p.fillRect(x,152,130,148);
            p.strokeStyle='#303a44';p.lineWidth=5;for(const xx of [x+43,x+86]){p.beginPath();p.moveTo(xx,152);p.lineTo(xx,300);p.stroke();}
            for(const yy of [201,250]){p.beginPath();p.moveTo(x,yy);p.lineTo(x+130,yy);p.stroke();}
        }
        for(const x of [25,560,1260]){p.fillStyle='#151d27';p.fillRect(x-9,125,18,400);p.fillStyle='#56606a';p.fillRect(x+8,125,2,400);}
        p.strokeStyle='#68707727';p.lineWidth=1;
        for(let x=-900;x<2200;x+=180){p.beginPath();p.moveTo(640+(x-640)*.13,488);p.lineTo(x,720);p.stroke();}
        for(const y of [505,541,605,705]){p.beginPath();p.moveTo(0,y);p.lineTo(1280,y);p.stroke();}
        p.fillStyle=gradient(p,490,610,420,'#b49b6f20');p.fillRect(0,460,1280,260);
        p.fillStyle='#080e17';p.fillRect(0,0,1280,82);
    }
    function pixelLandscape(p,mode){
        const r=random(mode==='sunset'?122:mode==='space'?819:355);
        if(mode==='sunset'){
            const sky=p.createLinearGradient(0,0,0,720);sky.addColorStop(0,'#201442');sky.addColorStop(.55,'#b55383');sky.addColorStop(.72,'#ed9869');sky.addColorStop(1,'#18152f');p.fillStyle=sky;p.fillRect(0,0,1280,720);
            p.fillStyle='#f8bf84';for(let y=-80;y<=80;y+=4){const half=Math.floor(Math.sqrt(80*80-y*y)/4)*4;p.fillRect(940-half,238+y,half*2,3);}
            for(let layer=0;layer<3;layer++){
                p.fillStyle=['#694366','#392e53','#1b213a'][layer];p.beginPath();p.moveTo(0,720);
                for(let x=0;x<=1320;x+=20){const y=410+layer*76+Math.sin(x*.007+layer)*52+Math.sin(x*.021)*18;p.lineTo(x,Math.floor(y/4)*4);}p.lineTo(1280,720);p.closePath();p.fill();
            }
            p.fillStyle='#0d152a';for(let n=0;n<17;n++){const x=n*83-20,top=460+(n*59)%150,h=720-top;p.fillRect(x,top+20,4,h);for(let y=top;y<710;y+=14){const half=(y-top)*.23+10;p.fillRect(x-half,y,half*2,8);}}
            for(let n=0;n<45;n++){p.fillStyle='#efd6ac';p.fillRect(Math.floor(r()*1280/3)*3,Math.floor(r()*180/3)*3,2,2);}
        }else if(mode==='space'){
            p.fillStyle='#060b25';p.fillRect(0,0,1280,720);
            for(let n=0;n<170;n++){p.fillStyle=['#51357735','#39326740','#22587730'][n%3];p.fillRect(Math.floor(r()*1280/12)*12,Math.floor((240+r()*400)/12)*12,24+Math.floor(r()*90/12)*12,12+Math.floor(r()*36/12)*12);}
            for(let n=0;n<180;n++){p.fillStyle=n%4?'#8a9ec7':'#e6c1ee';p.fillRect(Math.floor(r()*1280/2)*2,Math.floor(r()*720/2)*2,n%7?2:4,2);}
            p.fillStyle='#7b5b98';for(let y=-108;y<=108;y+=4){let half=Math.floor(Math.sqrt(108*108-y*y)/4)*4;p.fillStyle=y%16===0?'#a075ae':'#735488';p.fillRect(1010-half,230+y,half*2,4);}
            p.fillStyle='#dd9fbd';for(let x=-190;x<190;x+=8){const y=Math.floor((x*.2+Math.sin(x*.008)*18)/4)*4;p.fillRect(1010+x,230+y,8,8);}
            p.fillStyle='#253455';for(let x=0;x<1280;x+=24)p.fillRect(x,650+Math.floor(r()*40/4)*4,24,80);
            p.fillStyle='#111c34';p.fillRect(0,700,1280,20);
        }else{
            p.fillStyle='#100d24';p.fillRect(0,0,1280,720);
            for(let y=110;y<520;y+=32)for(let x=0;x<1280;x+=96){p.fillStyle=y%64?'#1f1938':'#1b1731';p.fillRect(x+(y%64?48:0),y,92,28);}
            p.fillStyle='#382b4d';p.fillRect(0,500,1280,12);p.fillStyle='#080b20';p.fillRect(0,512,1280,208);
            for(let y=520;y<720;y+=40)for(let x=0;x<1280;x+=80){p.fillStyle=(x/80+y/40)%2?'#1d1b37':'#10142b';p.fillRect(x,y,78,38);}
            for(const [x,color] of [[30,'#ae45cd'],[1110,'#38b5bf']]){
                p.fillStyle='#211c3a';p.fillRect(x,185,140,350);p.strokeStyle=color;p.lineWidth=4;p.strokeRect(x+4,189,132,342);
                p.fillStyle='#080e20';p.fillRect(x+14,210,112,90);p.fillStyle=color;for(let n=0;n<5;n++)p.fillRect(x+24+n*16,235+n%2*16,10,6);
                p.fillStyle='#373154';p.fillRect(x+12,320,116,84);p.fillStyle='#0b1025';p.fillRect(x+16,422,108,94);
            }
            p.fillStyle='#2c2543';p.fillRect(300,150,680,14);p.fillRect(300,350,680,14);
            for(let n=0;n<11;n++){p.fillStyle=['#514266','#775174','#41647b'][n%3];p.fillRect(314+n*58,110,38,40);p.fillStyle='#13132d';p.fillRect(320+n*58,122,26,5);}
            p.fillStyle='#49405c';p.fillRect(510,214,260,106);p.fillStyle='#18162e';p.fillRect(520,226,240,80);
            p.fillStyle='#92829e';p.fillRect(545,246,52,34);p.fillRect(678,246,52,34);
        }
    }
    function visual(p,mode,t,a){
        const level=Math.max(0,Math.min(1,Number(a?.level)||0)),bass=Math.max(0,Math.min(1,Number(a?.bass)||0)),bands=a?.bands;
        const band=n=>Math.max(0,Math.min(1,Number(bands?.[n%64])||0));
        if(mode==='cubes'){
            p.fillStyle='#060b19';p.fillRect(0,0,1280,720);
            for(let row=0;row<8;row++)for(let col=0;col<10;col++){
                const x=320+(col-row)*80,y=180+(col+row)*29,height=26+((row*19+col*31)%5)*14+band(col*5+row)*115+bass*28;
                const top=p.createLinearGradient(x-40,y-height,x+40,y);top.addColorStop(0,'#234b5a');top.addColorStop(1,'#111c34');
                for(const [points,color] of [[[[x,y-height],[x+78,y-27-height],[x+78,y-27],[x,y]],'#102132'],[[[x,y-height],[x-78,y-27-height],[x-78,y-27],[x,y]],'#080f21'],[[[x,y-height],[x+78,y-27-height],[x,y-54-height],[x-78,y-27-height]],top]]){
                    p.fillStyle=color;p.strokeStyle=col%2?'#ab45c4':'#466cd7';p.lineWidth=1+bass;p.beginPath();points.forEach(([xx,yy],n)=>n?p.lineTo(xx,yy):p.moveTo(xx,yy));p.closePath();p.fill();p.stroke();
                }
            }
        }else if(mode==='tunnel'){
            const centerX=640+Math.sin(t*.13)*70,centerY=400+Math.cos(t*.15)*35;
            p.fillStyle=gradient(p,centerX,centerY,520,'#41266c');p.fillRect(0,0,1280,720);
            for(let n=0;n<28;n++){
                const depth=((n/28+t*.035)%1),radius=35+depth*depth*1000,angle=n*.23+t*.08;
                p.strokeStyle=n%2?'#a43ca5':'#217ab7';p.globalAlpha=.12+depth*.5;p.lineWidth=1+depth*2+level;
                p.beginPath();for(let k=0;k<=12;k++){const phi=k/12*Math.PI*2+angle,x=centerX+Math.cos(phi)*radius,y=centerY+Math.sin(phi)*radius*.58;if(k)p.lineTo(x,y);else p.moveTo(x,y);}p.stroke();
            }
            p.globalAlpha=1;
            for(let n=0;n<80;n++){const depth=(n/80+t*.06)%1,phi=n*2.3999,r=depth*depth*1000;p.strokeStyle=n%2?'#a46cdb':'#4dc2e8';p.globalAlpha=depth*(.2+level*.5);p.beginPath();p.moveTo(centerX+Math.cos(phi)*r,centerY+Math.sin(phi)*r*.65);p.lineTo(centerX+Math.cos(phi)*(r+12+level*50),centerY+Math.sin(phi)*(r+12+level*50)*.65);p.stroke();}p.globalAlpha=1;
        }else{
            const warm=mode==='embers',rainbow=mode==='ribbons';
            p.fillStyle=gradient(p,720,450,710,warm?'#48213b':'#20254e');p.fillRect(0,0,1280,720);
            if(!rainbow){
                for(let n=0;n<64;n++){
                    const value=band(n),x=40+n*19,height=value*280;
                    const color=p.createLinearGradient(0,570-height,0,630);color.addColorStop(0,warm?'#ffe29d':'#54d8ec');color.addColorStop(.6,warm?'#e58a94':'#8551c9');color.addColorStop(1,warm?'#6b3356':'#28304f');
                    p.fillStyle=color;p.globalAlpha=.65;p.fillRect(x,570-height,12,height);p.globalAlpha=.1;p.fillRect(x,574,12,height*.3);
                }p.globalAlpha=1;
            }
            const lines=rainbow?24:warm?14:6;
            for(let line=0;line<lines;line++){
                const hue=rainbow?(line*15+t*2)%360:warm?20+line*3:185+line*25;
                p.strokeStyle=`hsla(${hue},85%,${warm?65:64}%,${rainbow ? .4 : .55})`;p.lineWidth=warm?1:line%3===0?2:1;
                p.beginPath();for(let n=0;n<=120;n++){
                    const u=n/120,x=u*1280,envelope=Math.sin(u*Math.PI),sample=band(Math.floor(u*63));
                    const amplitude=(rainbow?12:5)+level*90+sample*35;
                    const y=(warm?550:mode==='scope'?490:365)+Math.sin(u*10+t*.5+line*.13)*amplitude*envelope+Math.cos(u*19-t*.35+line*.18)*amplitude*.45+(line-lines/2)*(rainbow?3:6);
                    if(n)p.lineTo(x,y);else p.moveTo(x,y);
                }p.stroke();
            }
            if(warm)for(let n=0;n<35;n++){p.globalAlpha=(.06+level*.12)*(.5+.5*Math.sin(t*.2+n));p.fillStyle='#f5b96e';p.beginPath();p.arc(n*173%1280,120+n*89%390,3+n%5,0,Math.PI*2);p.fill();}p.globalAlpha=1;
        }
    }
    function base(mode){
        if(layers.has(mode))return layers.get(mode);
        const canvas=document.createElement('canvas');canvas.width=1280;canvas.height=720;
        const p=canvas.getContext('2d');if(mode==='neon')city(p);else if(mode==='warehouse')warehouse(p);else if(mode==='prism'){p.fillStyle='#080b1c';p.fillRect(0,0,1280,720);}else if(['sunset','space','arcade'].includes(mode))pixelLandscape(p,mode);else {p.fillStyle='#080b1c';p.fillRect(0,0,1280,720);}
        if(layers.size>=3)layers.delete(layers.keys().next().value);
        layers.set(mode,canvas);return canvas;
    }
    function draw(p,mode,seconds=0,audio=null){
        if(!modes.includes(mode))return false;
        const t=Number.isFinite(seconds)?seconds:0;p.save();p.imageSmoothingEnabled=mode!=='neon';p.drawImage(base(mode),0,0,1280,720);
        if(['scope','embers','ribbons','cubes','tunnel'].includes(mode))visual(p,mode,t,audio);
        if(mode==='neon'){
            p.fillStyle='#5ee6ee';
            for(let n=0;n<66;n++){const x=(n*197+t*12)%1320-20,y=(n*97+t*(65+n%4*8))%760-20;p.globalAlpha=.1+(n%4)*.035;p.fillRect(Math.floor(x),Math.floor(y),1,9+n%7);}
            p.globalAlpha=1;
            for(let n=0;n<3;n++){const x=((t*(13+n*6)+n*421)%1500)-110,y=310+n*82;p.fillStyle='#15203b';p.fillRect(x,y,52,8);p.fillStyle=n%2?'#e06ad6':'#4bd9e2';p.fillRect(x+3,y+6,45,2);p.fillStyle='#f4c786';p.fillRect(x+48,y+2,3,3);}
            p.globalAlpha=.32+.12*Math.sin(t*.8);p.fillStyle='#df52c8';p.fillRect(20,190,48,4);p.globalAlpha=1;
        }else if(mode==='warehouse'){
            for(let n=0;n<3;n++){
                const x=195+n*353+Math.sin(t*.12+n)*10;
                const beam=p.createLinearGradient(x,140,x-250,690);beam.addColorStop(0,'rgba(222,234,243,.18)');beam.addColorStop(.7,'rgba(222,225,218,.08)');beam.addColorStop(1,'rgba(222,225,218,0)');
                p.fillStyle=beam;p.beginPath();p.moveTo(x,153);p.lineTo(x+85,153);p.lineTo(x-150,720);p.lineTo(x-450,720);p.closePath();p.fill();
            }
            for(let n=0;n<55;n++){const x=(n*199+Math.sin(t*.14+n)*25)%1280,y=110+(n*83+t*(2+n%3))%570;p.globalAlpha=.07+.12*(.5+.5*Math.sin(t*.3+n));p.fillStyle='#f3dfbf';p.beginPath();p.arc(x,y,.7+n%3*.3,0,Math.PI*2);p.fill();}p.globalAlpha=1;
        }else if(mode==='prism'){
            const glow=p.createRadialGradient(850+Math.sin(t*.12)*100,360,0,640,360,830);glow.addColorStop(0,'#582394');glow.addColorStop(.55,'#142044');glow.addColorStop(1,'#080b1c');p.fillStyle=glow;p.fillRect(0,0,1280,720);
            for(let n=0;n<12;n++){
                const y=235+n*26,phase=t*.22+n*.22;
                const color=p.createLinearGradient(0,0,1280,0);color.addColorStop(0,'#2479b900');color.addColorStop(.3,'#28bdd66b');color.addColorStop(.65,'#bd46ed8f');color.addColorStop(1,'#e652b000');
                p.strokeStyle=color;p.lineWidth=2+n%3*.6;p.beginPath();p.moveTo(-30,y+Math.sin(phase)*70);p.bezierCurveTo(320,y-180+Math.cos(phase)*90,800,y+180+Math.sin(phase)*70,1310,y-70);p.stroke();
            }
            for(let n=0;n<20;n++){p.globalAlpha=.15+.2*(.5+.5*Math.sin(t*.4+n));p.fillStyle=n%2?'#88bdec':'#c794ee';p.beginPath();p.arc((n*193+t*3)%1280,100+n*127%570,1,0,Math.PI*2);p.fill();}p.globalAlpha=1;
        }
        if(mode==='sunset'){
            for(let n=0;n<6;n++){const x=Math.floor(((n*273+t*5)%1500)-150),y=155+n%3*44;p.globalAlpha=.11;p.fillStyle='#ffd6be';for(let k=0;k<7;k++)p.fillRect(x+k*12,y+((k+n)%3)*4,16,4);}p.globalAlpha=1;
            for(let n=0;n<12;n++){p.fillStyle='#e5bc7370';p.fillRect(Math.floor((n*173+Math.sin(t*.3+n)*16)%1280/2)*2,Math.floor(525+n*17+Math.sin(t*.6+n)*8),2,2);}
        }else if(mode==='space'){
            for(let n=0;n<20;n++){p.globalAlpha=.15+.6*(.5+.5*Math.sin(t*.7+n));p.fillStyle='#efddff';const x=n*193%1280,y=90+n*83%470;p.fillRect(x,y,2,2);if(n%4===0){p.fillRect(x-2,y,6,1);p.fillRect(x,y-2,1,6);}}p.globalAlpha=1;
            const x=Math.floor((t*32+200)%1600)-160,y=150+Math.floor((x+160)*.13);for(let n=0;n<14;n++){p.globalAlpha=(1-n/14)*.7;p.fillStyle='#a5d6ef';p.fillRect(x-n*6,y-n,5,2);}p.globalAlpha=1;
        }else if(mode==='arcade'){
            const step=Math.floor(t*2)%16;
            for(const x of [50,1130])for(let n=0;n<16;n++){p.fillStyle=n===step?'#f1c993':n%4===step%4?'#8b66bf':'#3b3553';p.fillRect(x+n%4*24,336+Math.floor(n/4)*15,19,10);}
            for(const x of [571,704]){p.fillStyle='#29263f';p.fillRect(x-8,249,16,26);p.fillStyle='#b09bbf';const dx=Math.floor(Math.cos(t*.6)*6),dy=Math.floor(Math.sin(t*.6)*6);p.fillRect(x+dx-2,262+dy-2,4,4);}
        }
        if(audio?.level>0&&!['scope','embers','ribbons','cubes','tunnel'].includes(mode)){
            p.globalAlpha=Math.min(.2,audio.level*.2);p.fillStyle=gradient(p,640,470,700,mode==='warehouse'?'#e8cda4':'#a168e8');p.fillRect(0,0,1280,720);p.globalAlpha=1;
        }
        // Quiet header for logos and a restrained edge vignette.
        const shade=p.createLinearGradient(0,0,0,140);shade.addColorStop(0,'#02051399');shade.addColorStop(1,'#02051300');p.fillStyle=shade;p.fillRect(0,0,1280,140);p.restore();return true;
    }
    function waveform(p,left,right,box,light=false){
        p.save();p.beginPath();p.rect(box.x,box.y,box.width,box.height);p.clip();
        p.lineWidth=1.1;p.strokeStyle=light?'#65708340':'#c2d0e52e';
        for(let ch=0;ch<2;ch++){
            const data=ch?right:left,mid=box.y+box.height*(ch ? .75 : .25),height=box.height*.21;
            p.beginPath();p.moveTo(box.x,mid);p.lineTo(box.x+box.width,mid);p.stroke();
            if(!data?.length)continue;
            p.strokeStyle=ch?(light?'#694389':'#c18af5'):(light?'#176b7b':'#58d9e9');
            p.beginPath();const count=Math.min(512,data.length);
            for(let n=0;n<count;n++){
                const sample=Number(data[Math.floor(n*data.length/count)]),value=Number.isFinite(sample)?Math.max(-1,Math.min(1,sample)):0;
                const x=box.x+n/Math.max(1,count-1)*box.width,y=mid-value*height;
                if(n)p.lineTo(x,y);else p.moveTo(x,y);
            }
            p.stroke();p.strokeStyle=light?'#65708340':'#c2d0e52e';
        }
        p.restore();
    }
    const api={waveform,draw,isAnimated:mode=>modes.includes(mode),cacheSize:()=>layers.size};
    if(typeof module!=='undefined')module.exports=api;root.WorkshopScenes=api;
})(typeof window==='undefined'?globalThis:window);
