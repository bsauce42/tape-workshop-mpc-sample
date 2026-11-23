// SPDX-License-Identifier: AGPL-3.0-only
const presentation=typeof require==='function'?require('../src/melodee/web/static/tutorial-presentation.js'):module.exports;
function assert(value,message){if(!value)throw Error(message);}
const box={x:20,y:40,width:400,height:500};
const full=presentation.framing(box,1199,1312,{zoom:1,x:0,y:0});
assert(Math.abs(full.width/full.height-1199/1312)<1e-9,'Upright device must retain its proportions');
const left=presentation.framing(box,1199,1312,{zoom:2,x:-1,y:-1}),right=presentation.framing(box,1199,1312,{zoom:2,x:1,y:1});
assert(Math.abs(left.x-box.x)<1e-9&&Math.abs(left.y-box.y)<1e-9,'Top-left framing must reach the device edge');
assert(Math.abs(right.x+right.width-box.x-box.width)<1e-9,'Right framing must reach the right edge');
const unchanged=presentation.framing(box,4,3,{zoom:1,x:0,y:0});
presentation.framing(box,1199,1312,{zoom:4,x:1,y:1});
assert(JSON.stringify(unchanged)===JSON.stringify(presentation.framing(box,4,3,{zoom:1,x:0,y:0})),'MPC framing must not change screen framing');
for(const mode of ['both','device','screen']){
    const start=presentation.layout('both'),end=presentation.layout(mode);
    const final=presentation.interpolate(start,end,1),middle=presentation.interpolate(start,end,.5);
    assert(final.screen.width===end.screen.width&&final.device.opacity===end.device.opacity,'Transition must arrive exactly');
    assert(middle.device.width>=Math.min(start.device.width,end.device.width)&&middle.device.width<=Math.max(start.device.width,end.device.width),'Interrupted transitions stay bounded');
    for(const panel of [final.screen,final.device])assert(panel.width>0&&panel.height>0&&panel.opacity>=0&&panel.opacity<=1,'No invalid intermediate panels');
}

for(const yaw of [-30,30]){
    const a=presentation.project(box,box.x,box.y,yaw),b=presentation.project(box,box.x+box.width,box.y,yaw);
    const c=presentation.project(box,box.x,box.y+box.height,yaw),d=presentation.project(box,box.x+box.width,box.y+box.height,yaw);
    assert(yaw<0?c.y-a.y<d.y-b.y:c.y-a.y>d.y-b.y,'Requested side must recede in true perspective');
}
const straight=presentation.project(box,box.x,box.y,0);assert(straight.x===box.x&&straight.y===box.y,'Zero angle must be unchanged');

const covered=presentation.imagePlacement(600,1000,'cover'),contained=presentation.imagePlacement(600,1000,'contain');
assert(covered.width>=1280&&covered.height>=720,'Uploaded image fill must cover the video');
assert(contained.width<=1280&&contained.height<=720,'Uploaded image fit must preserve the whole image');
assert(Math.abs(covered.width/covered.height-.6)<1e-9&&Math.abs(contained.width/contained.height-.6)<1e-9,'Image fitting must not stretch the upload');

for(const mode of ['both','screen','device'])for(const yaw of [-35,0,35]){
    const safe=presentation.safeScreenBox(presentation.layout(mode).screen,yaw);
    for(const x of [safe.x-12,safe.x+safe.width+12])for(const y of [safe.y-12,safe.y+safe.height+12])assert(presentation.project(safe,x,y,yaw).y>=106-1e-6,'Raised screen must stay below the protected header at every angle');
}
