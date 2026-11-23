// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop melody controls; host based on Melodee by Kerem Kiliç (Ellic Studio).
#pragma once
#include "melodee/pages/common.h"
#include "melodee/shell/stage_map.h"
#include "melodee/plugin/melody_pattern.h"
namespace melodee {
inline bool draw_melody_view(ui::Paint& p,const Snapshot& s,float top) {
    using namespace ui;
    if(s.ui.page==PageId::Stage&&stage_page_now(s.ui,s.track(),false).picture!=StagePicture::PluginEdit)return false;
    const auto& panel=s.ui.plugin_panel;const int base=melody::parameter_base(panel.name,panel.visual_count);
    if(base<0||panel.row*3u+2<unsigned(base))return false;
    std::array<float,melody::parameters> values{};for(unsigned n=0;n<values.size();++n)values[n]=panel.visual_controls[base+n].value;
    if(values[12]>.5f){values[2]=float(s.project.root%12)/11;values[3]=float(s.project.scale%13)/12;}
    const auto settings=melody::settings(values);
    constexpr float x=zone::pad,w=zone::width-2*x;
    const auto purple=juce::Colour(0xffc18cff),cyan=juce::Colour(0xff62e0d5),white=juce::Colour(0xfffaf5ff),dim=juce::Colour(0xffb7a4c9);
    p.fill(x,top+1,w,140,juce::Colour(0xff0b0812),5);p.ring(x,top+1,w,140,purple.withAlpha(.35f),5);
    p.text(std::string_view(panel.name)=="Tape Workshop Strings"?"STRINGS / MELODY PLAYER":"ORCHESTRA / MELODY PLAYER",x+7,top+4,w-88,15,plain(white,11,Weight::Bold));
    bool transport=s.track().plugin<max_plugins&&s.project.plugins[s.track().plugin].transport;
    p.text(!settings.enabled?"OFF":!transport?"LINK OFF":s.playing?"PLAY":"READY",x+w-82,top+6,75,12,plain(cyan,11,Weight::Bold),juce::Justification::centredRight);
    const juce::String harmony=juce::String(melody::keys[settings.key])+" "+melody::scale_names[settings.scale]+(values[12]>.5f?" / PROJECT":" / LOCAL");
    p.text(harmony,x+7,top+24,w-78,14,p.fit(harmony,plain(purple,11,Weight::Bold),w-78,type::floor));
    p.text("VAR "+juce::String(settings.variation),x+w-72,top+24,65,14,plain(dim,11,Weight::Bold),juce::Justification::centredRight);
    const float left=x+7,gy=top+46,gw=w-14,gh=70;
    p.fill(left,gy,gw,gh,juce::Colour(0xff191122),3);
    std::array<int,32> notes{};int low=127,high=0;
    for(unsigned n=0;n<settings.steps;++n){notes[n]=melody::note(settings,n);low=std::min(low,notes[n]);high=std::max(high,notes[n]);}
    const float span=float(std::max(7,high-low));const float cell=gw/settings.steps;
    const auto step=int64_t(std::floor((s.ticks/96.)/melody::beats(settings)));
    const unsigned playhead=unsigned(std::max<int64_t>(0,step)%settings.steps);
    for(unsigned n=0;n<=settings.steps;++n)p.fill(left+n*cell,gy, .5f,gh,purple.withAlpha(n%4?.12f:.3f));
    for(unsigned n=0;n<5;++n)p.fill(left,gy+n*gh/4,gw,.5f,purple.withAlpha(.1f));
    for(unsigned n=0;n<settings.steps;++n){float px=left+n*cell+1,py=gy+gh-7-(notes[n]-low)/span*(gh-14);
        if(melody::sounding(settings,n))p.fill(px,py,std::max(2.f,cell*float(settings.gate)-1),4,n==playhead&&s.playing&&settings.enabled&&transport?cyan:purple,1);
        else p.circle(px+cell*.3f,gy+gh-5,1,dim.withAlpha(.5f));
    }
    if(settings.enabled&&s.playing&&transport)p.fill(left+playhead*cell,gy,1,gh,white.withAlpha(.8f));
    float peak=0;for(auto pair:panel.scope)peak=std::max({peak,std::abs(int16_t(pair)/32768.f),std::abs(int16_t(pair>>16)/32768.f)});
    p.fill(left,top+121,gw,2,purple.withAlpha(.15f),1);p.fill(left,top+121,gw*std::clamp(peak,0.f,1.f),2,cyan,1);
    p.text("SHIFT+B3: SAMPLE KEY",left,top+127,204,12,plain(dim,11,Weight::Bold));
    p.text(juce::String(settings.steps)+" STEPS",x+w-76,top+127,69,12,plain(purple,11,Weight::Bold),juce::Justification::centredRight);
    return true;
}
}
