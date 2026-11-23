// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/shell/drum_tool.h"
#include <cassert>
#include <iostream>
using namespace melodee;
int main(){DrumTool d;d.pads={1,2,3,4,5,6,7,8};std::string error;assert(drums::generate(d.settings,{},d.hits,error));auto original=d.hits;
 for(unsigned selected=0;selected<8;++selected){d.role=selected;auto settings=drum_lane_settings(d);++settings.seed;std::vector<drums::Hit> result;assert(drums::generate(settings,original,result,error));for(unsigned role=0;role<8;++role)if(role!=selected){std::vector<drums::Hit>a,b;for(auto h:original)if(h.role==role)a.push_back(h);for(auto h:result)if(h.role==role)b.push_back(h);assert(a==b);}}
 d.muted[0]=true;auto pads=drum_active_pads(d);std::vector<Event> notes;assert(drums::notes(original,pads,d.settings.bars*384,notes,error));for(auto n:notes)assert(n.key!=36);assert(d.hits==original&&d.pads[0]==1);d.muted.fill(true);auto before=notes;assert(!drums::notes(original,drum_active_pads(d),d.settings.bars*384,notes,error));assert(notes.size()==before.size());
 for(unsigned bars:{1,2,4,8,16}){d.settings.bars=bars;assert(drums::generate(d.settings,{},d.hits,error));unsigned seen=0;for(unsigned pair=0;pair<(bars+1)/2;++pair)for(auto h:d.hits)if(h.tick/768==pair){assert(h.tick-pair*768<768);++seen;}assert(seen==d.hits.size());}
 std::cout<<"PASS: single-lane regeneration preserves all other lanes; mute excludes notes without deleting hits; all bars covered\n";
}
