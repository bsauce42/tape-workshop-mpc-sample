// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/pages/mixer_look.h"
#include "melodee/shell/mix_suite.h"
namespace melodee {
inline juce::String mixer_fx_context(const Snapshot& s) {
    if(s.ui.page==PageId::Master)return "MASTER";
    juce::String name="T"+juce::String(s.ui.track+1);
    if(s.ui.page==PageId::PadFx){char pad[4];pad_label(s.track().last_pad,pad);name+=" / "+juce::String(pad);}return name;
}
inline juce::String mixer_effect_name(const Project& p,const EffectSlot& slot){
    if(slot.kind==plugin_effect_kind&&unsigned(slot.params[0])<max_plugins)return p.plugins[unsigned(slot.params[0])].name;
    if(const auto* name=mix_suite_name(p,slot))return name;
    return name_case(effect_info(EffectKind(slot.kind)).name);
}
inline void mixer_fx_list(ui::Paint& p,const Snapshot& s){
    using namespace mixer_ui;using namespace ui;
    const auto n=page_chain(s.project,s.ui);const auto& chain=s.project.chain(n);unsigned cursor=std::min<unsigned>(s.ui.fx_cursor[UiState::cursor_of(n)],chain.count());
    header(p,s.ui.page==PageId::PadFx?"PAD FX CHAIN":s.ui.page==PageId::Master?"MASTER FX":"TRACK FX CHAIN",mixer_fx_context(s),{"BACK","ADD FX","OPEN"});
    label(p,juce::String(chain.count())+" EFFECTS",8,51,150,18,11,colour::mid);
    if(s.ui.page==PageId::PadFx)label(p,"BUILT-IN FX",162,51,150,18,11,colour::mid,juce::Justification::centredRight);
    unsigned first=cursor/3*3;unsigned count=std::min<unsigned>(chain.count()+1,chain.size);
    for(unsigned row=0;row<3&&first+row<count;++row){unsigned at=first+row;float y=74+row*36;bool selected=at==cursor; p.fill(8,y,304,30,selected?colour::raised:colour::panel,4);p.ring(8,y,304,30,selected?ink():colour::line,4);
        label(p,juce::String(at+1).paddedLeft('0',2),18,y+5,25,20,12,ink());label(p,at<chain.count()?mixer_effect_name(s.project,chain.slots[at]):"Add effect",49,y+5,209,20,13);
        label(p,at>=chain.count()?"+":chain.slots[at].bypass?"OFF":"ON",264,y+5,36,20,11,teal(),juce::Justification::centredRight);
    }
    cards(p,s);
}
inline void mixer_fx_picker(ui::Paint& p,const Snapshot& s){
    using namespace mixer_ui;using namespace ui;
    header(p,"ADD EFFECT",mixer_fx_context(s),{"BACK","CATEGORY","LOAD"});
    label(p,family_name(Family(s.ui.picker_family)),8,51,304,18,11,colour::mid);
    FamilyList list {};unsigned count=family_effects(Family(s.ui.picker_family),list);bool plugins=Family(s.ui.picker_family)==Family::Plugins;
    if(plugins){label(p,"OPEN PLUG-IN LIBRARY >",16,85,288,25,13,ink());}
    else {unsigned first=s.ui.picker_item/4*4;for(unsigned row=0;row<4&&first+row<count;++row){unsigned at=first+row;const auto& info=effect_info(list[at]);float y=73+row*27;bool selected=at==s.ui.picker_item;
        p.fill(8,y,304,23,selected?colour::raised:colour::panel,3);if(selected)p.ring(8,y,304,23,ink(),3);
        label(p,name_case(info.name),18,y+2,280,20,12,info.built?colour::text:colour::dim);
    }}cards(p,s);
}
}
