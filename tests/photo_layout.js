// SPDX-License-Identifier: AGPL-3.0-only
const photo=typeof require==='function'?require('../src/melodee/web/static/photo-layout.js'):module.exports;
function assert(v,text){if(!v)throw Error(text);}
for(const box of [{x:0,y:0,width:400,height:500},{x:0,y:0,width:270,height:296},{x:850,y:100,width:400,height:500},{x:0,y:0,width:900,height:440}]){
    const p=photo.placement(box);
    assert(p.photo.width<=box.width+.001&&p.photo.height<=box.height+.001,'Photo must fit');
    assert(Math.abs(p.display.width/p.display.height-4/3)<.0001,'Live image must preserve 4:3');
    assert(p.display.x>=p.slot.x-.001&&p.display.y>=p.slot.y-.001&&p.display.x+p.display.width<=p.slot.x+p.slot.width+.001&&p.display.y+p.display.height<=p.slot.y+p.slot.height+.001,'Display must stay inside photographed LCD');
    assert(Math.abs((p.slot.x-p.photo.x)/p.photo.scale-photo.screen.x)<.001,'Recording and DOM must share image coordinates');
}
for(let i=0;i<16;i++){
    const p=photo.pad(i);assert(p.x>=0&&p.y>=0&&p.x+p.width<=photo.width&&p.y+p.height<=photo.height,'Pad must stay inside photo');
}
assert(photo.pad(1).x===473&&photo.pad(1).y===961,'A02 must be second pad from left on bottom row');
for(const index of [0,1,2,3]){
 const low=photo.knobPose(index,0,0),high=photo.knobPose(index,127,0);
 assert(low&&high&&low.angle<high.angle,'Knob must move across its range');
 assert(Math.abs(Math.hypot(low.x2-low.x,low.y2-low.y)-31)<.001,'Indicator length must match recorder');
 assert(photo.knobPose(index,-1,0)===null,'Unknown values must leave the photo untouched');
}
assert(photo.knobPose(4,64,0)===null,'Fader must not become a knob');
assert(Math.abs(photo.knobPose(5,-1,20).angle-photo.knobPose(5,-1,0).angle)<.001,'Data wheel must wrap');
