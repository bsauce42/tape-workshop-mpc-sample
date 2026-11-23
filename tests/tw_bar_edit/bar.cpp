// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/bar_edit.h"
#include "melodee/model/scales.h"
#include <cassert>
#include <iostream>
using namespace melodee;
int main(){
 for(unsigned root=0;root<12;++root)for(auto scale:scales()){auto chords=bar_chords(root,scale.mask);for(auto c:chords)for(unsigned n=0;n<c.count;++n)assert(scale.mask&(1u<<((c.notes[n]+12-root)%12)));}
 Pattern src;src.length=1536;Event cross;cross.tick=100;cross.length=1300;cross.key=60;cross.ratchet=3;src.events.push_back(cross);Event other;other.tick=1200;other.length=96;other.key=70;src.events.push_back(other);auto hash=bar_fingerprint(src);
 Event added;added.tick=1152;added.length=384;added.key=65;Pattern out;std::string error;assert(bar_replace(src,1152,1536,-1,{added},out,error));assert(out.events.size()==2&&out.events[0].tick==100&&out.events[0].length==1052&&out.events[0].ratchet==3);assert(bar_fingerprint(src)==hash);
 // A held note crossing both edges leaves both outside tails unchanged.
 assert(bar_replace(src,384,768,-1,{},out,error));assert(out.events[0].tick==100&&out.events[0].length==284&&out.events[1].tick==768&&out.events[1].length==632);
 Pattern shifted;assert(bar_octave(src,{1},-1,-1,shifted,error));bool moved=false;for(auto e:shifted.events)if(e.key==58)moved=true;assert(moved);auto before=shifted;assert(!bar_octave(src,{1},8,-1,shifted,error)&&shifted.events==before.events);
 assert(!bar_replace(src,384,768,-1,{added},out,error));assert(bar_fingerprint(src)==hash);
 Pattern pad;pad.length=384;Event e;e.key=40;e.pitch=0;pad.events.push_back(e);assert(bar_octave(pad,{0},-2,40,out,error)&&out.events[0].key==40&&out.events[0].pitch==-24);
 Event untouched=e;untouched.key=41;pad.events.push_back(untouched);assert(bar_replace(pad,0,384,40,{},out,error)&&out.events.size()==1&&out.events[0].key==41);
 std::cout<<"All roots/scales constrain every chord tone; bar replacement preserves outside tails/metadata; octave bounds, pitched pads, source immutability and safe refusal passed.\n";
}
