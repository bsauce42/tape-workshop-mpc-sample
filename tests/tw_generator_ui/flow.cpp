// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/shell/drum_tool.h"
#include "melodee/model/project.h"
#include <cassert>
#include <memory>
#include <iostream>
using namespace melodee;
int main(){DrumTool d;d.open=true;for(unsigned n=0;n<5;++n){assert(d.page==0&&d.row==n);drum_next(d);}assert(d.row==5);drum_next(d);assert(d.page==1&&d.row==0);for(unsigned role=0;role<drums::roles;++role){assert(d.page==1&&d.row==0&&d.role==role);drum_next(d);}assert(d.page==1&&d.row==1);drum_next(d);assert(d.page==2);drum_back(d);assert(d.page==1&&d.row==0);for(unsigned role=drums::roles-1;role>0;--role){assert(d.role==role);drum_back(d);}drum_back(d);assert(d.page==0&&d.row==5);for(unsigned n=0;n<5;++n)drum_back(d);assert(d.open&&d.row==0);drum_back(d);assert(!d.open);
 auto t=std::make_unique<Track>();t->engine=EngineKind::Sampler;t->pads[0].sample=0;d=DrumTool{};d.open=true;d.owner=3;d.role=2;d.pads[0]=1;assert(drum_sound_destination(d,*t)==1);d.pads[2]=128;assert(drum_sound_destination(d,*t)==127);d.pads[2]=0;for(auto& p:t->pads)p.sample=0;assert(drum_sound_destination(d,*t)<0);d.sound_pad=127;d.sound_role=2;d.sound_load_requested=true;assert(drum_sound_matches(d,3,127));assert(!drum_sound_matches(d,2,127)&&!drum_sound_matches(d,3,126));d.sound_load_requested=false;assert(!drum_sound_matches(d,3,127));
 d=DrumTool{};d.pads={1,2,3,4,5,6,7,8};d.muted[1]=true;auto active=drum_active_pads(d);assert(active[0]==1&&active[1]==0&&d.pads[1]==2);
 d.role=2;auto lane=drum_lane_settings(d);for(unsigned n=0;n<drums::roles;++n)assert(lane.locked[n]==(n!=2));d.settings.locked[2]=true;lane=drum_lane_settings(d);for(bool locked:lane.locked)assert(locked);
 std::cout<<"One-setting drum navigation, assigned/empty/full destination selection and async load-owner guards passed.\n";
}
