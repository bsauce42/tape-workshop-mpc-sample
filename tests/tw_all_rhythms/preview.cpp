// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/shell/harmony_preview.h"
#include <cassert>
#include <set>
#include <vector>
#include <iostream>
using namespace melodee;
int main(){
 auto prepare=[](){HarmonyTool h;h.rhythm_preview_count=6;h.rhythm_preview_tempo=1200;for(unsigned n=0;n<6;++n)h.rhythm_preview[n]={n<3?0u:96u,96,uint8_t(48+n%3*4),100};return h;};
 auto h=prepare();std::set<unsigned> held;std::vector<int> events;auto on=[&](unsigned key,unsigned){assert(held.insert(key).second);events.push_back(int(key));};auto off=[&](unsigned key){assert(held.erase(key)==1);events.push_back(-int(key));};
 advance_harmony_preview(h,0,on,off);assert(held.size()==3);advance_harmony_preview(h,500,on,off);assert(held.size()==3&&events.size()==9);for(unsigned n=3;n<6;++n)assert(events[n]<0);advance_harmony_preview(h,1000,on,off);assert(held.empty()&&!h.rhythm_preview_count);
 h=prepare();advance_harmony_preview(h,0,on,off);stop_harmony_preview(h,off);assert(held.empty()&&!h.rhythm_preview_count);advance_harmony_preview(h,700,on,off);assert(held.empty());
 h=prepare();advance_harmony_preview(h,1200,on,off);assert(held.empty()&&!h.rhythm_preview_count); // delayed tick never emits old notes
 h=prepare();advance_harmony_preview(h,0,on,off);advance_harmony_preview(h,750,on,off);assert(held.size()==3);stop_harmony_preview(h,off);assert(held.empty());
 std::cout<<"Polyphonic preview note-offs/onsets, shared pitches, cancellation, finish and delayed-tick skipping passed.\n";
}
