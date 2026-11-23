// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/chord_roll.h"
#include <cassert>
#include <iostream>
using namespace melodee;
int main(){
    Pattern original;original.length=384;Event melody;melody.key=80;melody.tick=24;melody.length=48;original.events.push_back(melody);
    harmony::Settings s;s.root=2;s.scale_mask=0x5ad;auto chords=harmony::progression(4,384,s);
    Pattern kept,replaced;std::string error;
    assert(harmony::roll_edit(original,chords,1536,49152,true,kept,error));assert(kept.length==1536);
    bool found=false;for(auto& e:kept.events)found|=e==melody;assert(found&&original.events.size()==1&&original.length==384);
    assert(harmony::roll_edit(original,chords,1536,49152,false,replaced,error));for(auto& e:replaced.events)assert(e!=melody);
    assert(kept.events.size()==replaced.events.size()+1);
    Pattern twice;assert(harmony::roll_edit(kept,chords,1536,49152,true,twice,error));assert(twice.events==kept.events);
    Pattern out=original;assert(!harmony::roll_edit(original,chords,49153,49152,false,out,error));assert(out.events==original.events);
    auto bad=chords;bad.chords[0].tick=1536;assert(!harmony::roll_edit(original,bad,1536,49152,true,out,error));assert(out.events==original.events);
    harmony::Result empty;assert(!harmony::roll_edit(original,empty,1536,49152,false,out,error));
    std::cout<<"Keep, replace, source preservation, duplicate prevention, bounded length and failed edits leave notes unchanged.\n";
}
