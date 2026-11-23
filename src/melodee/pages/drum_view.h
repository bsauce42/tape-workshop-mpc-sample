// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/pages/common.h"
#include "melodee/pages/generator_focus.h"
#include "melodee/ui/widgets.h"
namespace melodee {
inline void draw_drums(ui::Paint& p,const Snapshot& s){
 using namespace ui;const auto& d=s.ui.drums;const auto accent=generator_accent(),mint=generator_activity();
 p.fill(0,0,320,240,colour::bg);
 soft_keys(p,d.confirm?SoftKeys{{"CANCEL","CONFIRM",""},-1,accent}:SoftKeys{{d.page==2&&s.ui.shift?"MUTE":"BACK",d.page==2&&s.ui.shift?"NEW LANE":d.page==1?"AUTO MAP":d.stale?"GENERATE":"NEW BEAT",d.page==2&&s.ui.shift?"LOCK":d.page==3?"REVIEW":d.page==1&&!d.row?(d.role+1<drums::roles?"NEXT DRUM":"LOCKS"):"NEXT"},-1,accent});
 p.text("DRUM GENERATOR",8,27,304,22,plain(colour::text,17,Weight::Bold));
 const char* tabs[]={"STYLE","PADS","PREVIEW","ADD"};for(unsigned n=0;n<4;++n){if(n==d.page)p.fill(6+n*78,52,75,20,accent,3);p.text(tabs[n],9+n*78,54,70,16,plain(n==d.page?colour::bg:colour::mid,12,Weight::Bold));}
 auto text=[&](juce::String value,float y,juce::Colour color,float size=14){p.text(value,10,y,300,21,p.fit(value,plain(color,size,Weight::Bold),300,12));};
 if(d.confirm){const char* names[]={"Cancel","Keep + Add","Replace Roll"};generator_focus(p,"Confirm beat edit",names[d.confirm_pick],d.confirm_pick,3,s.now,d.field_changed_at);text(d.confirm_pick==2?"Replaces this Roll's existing notes":d.confirm_pick==1?"Keeps notes and adds the generated beat":"Leaves your Roll unchanged",199,d.confirm_pick==2?accent:mint,13);text("Data: choose / Push or B2: confirm",220,mint,13);return;}
 if(d.page==0){const char* labels[]={"Genre","Loop length","Density","Swing","Fills","Variation"};juce::String values[]={drums::genre_name(d.settings.genre),juce::String(d.settings.bars)+" bars",juce::String(d.settings.density)+"%",juce::String(d.settings.swing)+"%",juce::String(d.settings.fills)+"%",juce::String(d.settings.seed%1000)};
  generator_focus(p,labels[d.row%6],values[d.row%6],d.row%6,6,s.now,d.field_changed_at);
  text("K1: setting / K2 or Data: change",199,mint,13);text("Push: generate / B3: next setting",220,colour::mid,12);
 }else if(d.page==1){unsigned pad=d.pads[d.role];juce::String address=pad?juce::String::charToString(char('A'+(pad-1)/16))+juce::String((pad-1)%16+1).paddedLeft('0',2):"New pad";
  juce::String label=d.row?juce::String(drums::role_name(d.role))+" / Lock":juce::String(d.role+1)+". Load "+drums::role_name(d.role)+" / "+address;juce::String name=juce::String("Choose ")+drums::role_name(d.role);if(pad){auto slot=s.track().pads[pad-1].sample;name=slot<sample_slots&&s.project.samples[slot].used()?juce::String(s.project.samples[slot].name):"Empty pad";}
  generator_focus(p,label,d.row?(d.settings.locked[d.role]?"LOCKED":"FREE"):name,d.row?1:d.role,d.row?2:drums::roles,s.now,d.field_changed_at);
  text(d.row?"K1: Pad / Lock / K3: drum role":juce::String("Push: choose ")+drums::role_name(d.role)+" sample",199,mint,13);text(d.row?"Data or Push: change lock":"B3: next drum / K3: choose drum",220,colour::mid,12);
 }else if(d.page==2){
  unsigned first=d.preview_pair*2,last=std::min<unsigned>(first+2,d.settings.bars);
  text(juce::String(drums::genre_name(d.settings.genre))+" / bars "+juce::String(first+1)+"-"+juce::String(last),76,mint,13);
  constexpr const char* short_names[]={"KICK","SNARE","CLOSED HH","OPEN HH","CLAP","PERC","TOM","CYMBAL"};
  constexpr float grid_x=97,cell=6.6f,lane_y=101,lane_h=12.5f;
  unsigned play_tick=0;if(d.rhythm_preview_count)play_tick=unsigned(uint64_t(s.now-d.rhythm_preview_start)*d.rhythm_preview_tempo*96/600000);
  for(unsigned role=0;role<drums::roles;++role){float y=lane_y+role*lane_h;bool selected=role==d.role,loaded=d.pads[role]!=0;
   if(selected)p.fill(5,y-1,310,lane_h,accent.withAlpha(.18f),2);
   p.text(short_names[role],8,y,76,12,plain(d.muted[role]?colour::dim:selected?mint:colour::text,11,Weight::Bold));
   p.text(d.muted[role]?"M":d.settings.locked[role]?"L":loaded?"+":"-",83,y,12,12,plain(d.muted[role]?colour::dim:accent,11,Weight::Bold));
   for(unsigned step=0;step<32;++step){bool valid=first+step/16<d.settings.bars;p.fill(grid_x+step*cell,y+1,5.5f,9,valid?accent.withAlpha(.12f):colour::bg,1);if(valid)p.ring(grid_x+step*cell,y+1,5.5f,9,accent.withAlpha(.22f),1);}
   for(auto hit:d.hits)if(hit.role==role&&hit.tick>=first*384&&hit.tick<last*384){unsigned step=(hit.tick-first*384)/24;auto color=d.muted[role]||!loaded?colour::dim:d.settings.locked[role]?accent:mint;p.fill(grid_x+step*cell,y+1,5.5f,9,color.withAlpha(.45f+.55f*hit.velocity/127.f),1);}
  }
  if(d.rhythm_preview_count&&play_tick<768){float px=grid_x+play_tick/24.f*cell;p.fill(px,lane_y,1,lane_h*8,colour::text.withAlpha(.65f));}
  text(d.stale?"B2: generate beat first":juce::String(drums::role_name(d.role))+" / "+(d.muted[d.role]?"MUTED":d.settings.locked[d.role]?"LOCKED":d.pads[d.role]?"READY":"LOAD SOUND")+" / SHIFT: tools",201,d.stale?accent:mint,12);
  text(s.ui.shift?"Tools affect the selected drum lane":"Push: listen / K1: bars / K3: lane",220,colour::mid,12);
 }else{generator_focus(p,"Add to drum Roll",juce::String(d.settings.bars)+" bars / "+juce::String(d.hits.size())+" hits",0,1,s.now,d.field_changed_at);text("Review Keep or Replace before adding",199,mint,13);text("Push or B3: review / B1: preview",220,colour::mid,12);}
}
}
