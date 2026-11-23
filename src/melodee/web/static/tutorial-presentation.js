// SPDX-License-Identifier: AGPL-3.0-only
(function(root){
    'use strict';
    // Upright, independent camera crops, shared by browser and video export.
    function framing(box,nativeWidth,nativeHeight,camera={zoom:1,x:0,y:0}){
        const zoom=Math.max(1,Math.min(4,Number(camera.zoom)||1));
        const x=Math.max(-1,Math.min(1,Number(camera.x)||0)),y=Math.max(-1,Math.min(1,Number(camera.y)||0));
        const scale=Math.min(box.width/nativeWidth,box.height/nativeHeight)*zoom;
        const width=nativeWidth*scale,height=nativeHeight*scale;
        return {x:box.x+(box.width-width)/2-x*Math.max(0,(width-box.width)/2),
            y:box.y+(box.height-height)/2-y*Math.max(0,(height-box.height)/2),width,height};
    }
    // Shared room lighting for the preview and recorded output.
    function background(paint,studio,color){
        if(!studio){paint.fillStyle=color;paint.fillRect(0,0,1280,720);return;}
        paint.save();
        const wall=paint.createLinearGradient(0,0,0,720);
        wall.addColorStop(0,'#fbfcfd');wall.addColorStop(.73,'#e8ebee');wall.addColorStop(.735,'#d8dde2');wall.addColorStop(1,'#fafbfc');
        paint.fillStyle=wall;paint.fillRect(0,0,1280,720);
        const light=paint.createRadialGradient(440,160,0,640,340,830);
        light.addColorStop(0,'rgba(255,255,255,.8)');light.addColorStop(1,'rgba(170,181,192,.18)');
        paint.fillStyle=light;paint.fillRect(0,0,1280,720);
        paint.strokeStyle='rgba(110,125,140,.09)';paint.lineWidth=1;
        for(const x of [-640,0,1280,1920]){paint.beginPath();paint.moveTo(640+(x-640)*.18,529);paint.lineTo(x,720);paint.stroke();}
        paint.restore();
    }
    function imagePlacement(width,height,fit='cover'){
        const scale=(fit==='contain'?Math.min:Math.max)(1280/Math.max(1,width),720/Math.max(1,height));
        return {x:(1280-width*scale)/2,y:(720-height*scale)/2,width:width*scale,height:height*scale};
    }
    function contactShadow(paint,box){
        if(box.opacity===0)return;
        paint.save();paint.globalAlpha=box.opacity??1;
        const x=box.x+box.width*.53,y=Math.min(692,box.y+box.height+14);
        paint.translate(x,y);paint.scale(box.width*.57,Math.max(7,box.height*.045));
        const shade=paint.createRadialGradient(0,0,0,0,0,1);
        shade.addColorStop(0,'rgba(31,43,56,.25)');shade.addColorStop(.42,'rgba(31,43,56,.13)');shade.addColorStop(1,'rgba(31,43,56,0)');
        paint.fillStyle=shade;paint.beginPath();paint.arc(0,0,1,0,Math.PI*2);paint.fill();paint.restore();
    }
    function flatScreen(paint,live,box,tablet,camera){
        paint.save();
        if(tablet){
            const {x,y,width,height}=box;
            paint.shadowColor='rgba(0,0,0,.65)';paint.shadowBlur=16;paint.shadowOffsetY=8;
            paint.fillStyle='#08090d';paint.beginPath();paint.roundRect(x-10,y-10,width+20,height+20,16);paint.fill();
            paint.shadowColor='transparent';paint.strokeStyle='#454650';paint.lineWidth=1.5;paint.stroke();
            paint.beginPath();paint.roundRect(x,y,width,height,7);paint.clip();
        }
        paint.beginPath();paint.rect(box.x,box.y,box.width,box.height);paint.clip();
        const crop=framing(box,4,3,camera);
        paint.imageSmoothingEnabled=camera?.smooth!==false;paint.imageSmoothingQuality="high";paint.drawImage(live,crop.x,crop.y,crop.width,crop.height);paint.restore();
    }
    function project(box,x,y,yaw){
        const angle=Math.max(-35,Math.min(35,Number(yaw)||0))*Math.PI/180;
        const dx=x-box.x-box.width/2,dy=y-box.y-box.height/2;
        const w=1+Math.sin(angle)*dx/(box.width*2);
        return {x:box.x+box.width/2+Math.cos(angle)*dx/w,y:box.y+box.height/2+dy/w,w};
    }
    function safeScreenBox(box,yaw){
        let top=Infinity;
        for(const x of [box.x-12,box.x+box.width+12])for(const y of [box.y-12,box.y+box.height+12])top=Math.min(top,project(box,x,y,yaw).y);
        return {...box,y:box.y+Math.max(0,106-top)};
    }
    let flatCanvas=null,projectCanvas=null,projectGL=null,projectProgram=null,projectTexture=null;
    function shader(gl,kind,source){
        const part=gl.createShader(kind);gl.shaderSource(part,source);gl.compileShader(part);
        if(!gl.getShaderParameter(part,gl.COMPILE_STATUS))throw Error('Perspective shader unavailable');
        return part;
    }
    function initProjection(){
        flatCanvas=document.createElement('canvas');flatCanvas.width=1280;flatCanvas.height=1024;
        projectCanvas=document.createElement('canvas');projectCanvas.width=1280;projectCanvas.height=1024;
        const gl=projectCanvas.getContext('webgl',{alpha:true,premultipliedAlpha:true,preserveDrawingBuffer:true,antialias:true});
        if(!gl)return;
        try{
            const program=gl.createProgram();
            gl.attachShader(program,shader(gl,gl.VERTEX_SHADER,'attribute vec4 p;attribute vec2 uv;varying vec2 v;void main(){gl_Position=p;v=uv;}'));
            gl.attachShader(program,shader(gl,gl.FRAGMENT_SHADER,'precision mediump float;varying vec2 v;uniform sampler2D image;void main(){gl_FragColor=texture2D(image,v);}'));
            gl.linkProgram(program);if(!gl.getProgramParameter(program,gl.LINK_STATUS))throw Error('Perspective shader unavailable');
            gl.useProgram(program);projectProgram=program;
            gl.bindBuffer(gl.ARRAY_BUFFER,gl.createBuffer());
            projectTexture=gl.createTexture();gl.bindTexture(gl.TEXTURE_2D,projectTexture);
            gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.LINEAR);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.LINEAR);
            gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_S,gl.CLAMP_TO_EDGE);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_T,gl.CLAMP_TO_EDGE);
            gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL,true);projectGL=gl;
        }catch{projectGL=null;}
    }
    // Affine triangles are only a fallback on devices without WebGL.
    function fallbackProjection(paint,texture,points,width,height){
        for(let n=0;n<32;n++){
            const u=n/32,v=(n+1)/32,tl=points(u,0),tr=points(v,0),bl=points(u,1),br=points(v,1),sx=u*width,dx=(v-u)*width;
            for(const [a,b,c,first] of [[tl,tr,br,true],[tl,br,bl,false]]){
                paint.save();paint.beginPath();paint.moveTo(a.x,a.y);paint.lineTo(b.x,b.y);paint.lineTo(c.x,c.y);paint.closePath();paint.clip();
                const aa=first?(tr.x-tl.x)/dx:(br.x-bl.x)/dx,bb=first?(tr.y-tl.y)/dx:(br.y-bl.y)/dx;
                const cc=first?(br.x-tr.x)/height:(bl.x-tl.x)/height,dd=first?(br.y-tr.y)/height:(bl.y-tl.y)/height;
                paint.transform(aa,bb,cc,dd,tl.x-aa*sx,tl.y-bb*sx);paint.drawImage(texture,0,0,width,height);paint.restore();
            }
        }
    }
    function screen(paint,live,box,tablet,camera){
        const yaw=Math.max(-35,Math.min(35,Number(camera?.yaw)||0));
        if(Math.abs(yaw)<.01){flatScreen(paint,live,box,tablet,camera);return;}
        if(!flatCanvas)initProjection();
        const pad=24,w=box.width+pad*2,h=box.height+pad*2,flat=flatCanvas.getContext('2d');
        flat.setTransform(1,0,0,1,0,0);flat.clearRect(0,0,1280,1024);
        flat.setTransform(1280/w,0,0,1024/h,0,0);
        flatScreen(flat,live,{x:pad,y:pad,width:box.width,height:box.height},tablet,camera);
        const point=(u,v)=>project(box,box.x-pad+u*w,box.y-pad+v*h,yaw);
        const corners=[point(0,0),point(1,0),point(0,1),point(1,1)];
        if(!projectGL||projectGL.isContextLost()){fallbackProjection(paint,flatCanvas,point,1280,1024);return;}
        const left=Math.min(...corners.map(p=>p.x)),top=Math.min(...corners.map(p=>p.y));
        const width=Math.max(...corners.map(p=>p.x))-left,height=Math.max(...corners.map(p=>p.y))-top;
        const gl=projectGL,vertices=[];
        corners.forEach((p,n)=>vertices.push(((p.x-left)/width*2-1)*p.w,(1-(p.y-top)/height*2)*p.w,0,p.w,n%2,Math.floor(n/2)));
        gl.viewport(0,0,1280,1024);gl.clearColor(0,0,0,0);gl.clear(gl.COLOR_BUFFER_BIT);
        gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(vertices),gl.DYNAMIC_DRAW);
        for(const [name,count,offset] of [['p',4,0],['uv',2,16]]){
            const at=gl.getAttribLocation(projectProgram,name);gl.enableVertexAttribArray(at);gl.vertexAttribPointer(at,count,gl.FLOAT,false,24,offset);
        }
        gl.bindTexture(gl.TEXTURE_2D,projectTexture);
        const filter=camera?.smooth===false?gl.NEAREST:gl.LINEAR;
        gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,filter);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,filter);
        gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,gl.RGBA,gl.UNSIGNED_BYTE,flatCanvas);
        gl.drawArrays(gl.TRIANGLE_STRIP,0,4);
        paint.drawImage(projectCanvas,left,top,width,height);
    }
    const layouts={
        both:{screen:{x:40,y:112,width:680,height:510,opacity:1},device:{x:848,y:144,width:368,height:460,opacity:1}},
        device:{screen:{x:40,y:112,width:680,height:510,opacity:0},device:{x:428,y:128,width:424,height:530,opacity:1}},
        screen:{screen:{x:300,y:112,width:680,height:510,opacity:1},device:{x:848,y:144,width:368,height:460,opacity:0}}
    };
    function layout(mode){return layouts[mode]||layouts.both;}
    function interpolate(from,to,progress){
        const t=(1-Math.cos(Math.PI*Math.max(0,Math.min(1,progress))))/2,result={};
        for(const panel of ['screen','device']){
            result[panel]={};
            for(const key of ['x','y','width','height','opacity'])result[panel][key]=from[panel][key]+(to[panel][key]-from[panel][key])*t;
        }
        return result;
    }
    const api={safeScreenBox,imagePlacement,background,contactShadow,framing,screen,project,layout,interpolate};
    if(typeof module!=='undefined')module.exports=api;root.WorkshopPresentation=api;
})(typeof window==='undefined'?globalThis:window);
