// SPDX-License-Identifier: AGPL-3.0-only
// Original Tape Workshop graphics; Melodee host by Kerem Kiliç (Ellic Studio).
#pragma once
#include "melodee/pages/common.h"
#include <string_view>
namespace melodee {
inline bool bloom_view(ui::Paint& p,const Snapshot& s,float x,float y,float w,float h,bool bypass){
    using namespace ui;const auto& panel=s.ui.plugin_panel;
    std::string_view name(panel.name);if(name.size()>5&&name.substr(name.size()-5)==" (TW)")name.remove_suffix(5);
    if(name!="Bloom Reverb"||panel.status!=PluginStatus::Ready||h<115)return false;
    juce::Graphics::ScopedSaveState saved(p.g);p.g.reduceClipRegion(juce::Rectangle<float>(x,y,w,h).getSmallestIntegerContainer());
    auto value=[&](unsigned n){return n<panel.visual_count?std::clamp(panel.visual_controls[n].value,0.f,1.f):0.f;};
    const auto gold=juce::Colour(0xffffb86c),cyan=juce::Colour(0xff6bdedb),white=juce::Colour(0xfff3eee8),dim=juce::Colour(0xffaaa8b5),dark=juce::Colour(0xff10121a);
    const unsigned shown=std::min(7u,unsigned(panel.row)),row=panel.bloom_ir_first?(shown==0?6:shown<=6?shown-1:7):shown;constexpr const char* pages[]={"SPACE / DECAY","COLOUR / DISTANCE","TIMING / STEREO","DECAY EQ","POST TONE","OTHER WORLDS","IMPULSE RESPONSE","MODULATION"};
    p.fill(x,y+1,w,h-2,dark,4);p.ring(x,y+1,w,h-2,gold.withAlpha(.35f),4);
    p.text("BLOOM REVERB",x+7,y+4,w-110,15,plain(gold,12,Weight::Bold));
    p.text(bypass?"BYPASS":value(17)>.5f?"FROZEN":"TAPE WORKSHOP",x+w-105,y+5,98,13,p.fit("TAPE WORKSHOP",plain(bypass?dim:cyan,11,Weight::Bold),98,type::floor),juce::Justification::centredRight);
    p.text(pages[row],x+7,y+21,w-16,13,plain(white,11,Weight::Bold));
    const float gx=x+8,gy=y+39,gw=w-16,gh=std::max(20.f,h-64);
    p.fill(gx,gy,gw,gh,juce::Colour(0xff080a10),3);
    float peak=0;if(!bypass)for(auto v:panel.scope)peak=std::max(peak,std::abs(int16_t(v))/32768.f);
    for(unsigned n=1;n<6;++n)p.fill(gx+gw*n/6,gy,.5f,gh,white.withAlpha(.06f));
    if(row==3||row==4){
        juce::Path curve;for(unsigned n=0;n<=100;++n){float u=n/100.f,t;
            if(row==3){float a=.25f+1.75f*value(9),b=.25f+1.75f*value(10),c=.25f+1.75f*value(11);t=u<.5f?a+(b-a)*u*2:b+(c-b)*(u-.5f)*2;t=std::clamp(t/2,0.f,1.f);}
            else {float freq=20*std::pow(1000.f,u),lo=20*std::pow(40.f,value(12)),hi=1000*std::pow(20.f,value(13));float response=1/std::sqrt(1+std::pow(lo/freq,2.f))/std::sqrt(1+std::pow(freq/hi,2.f));t=std::clamp(.3f+.55f*response+(value(14)-.5f)*(u-.5f)*.7f,0.f,1.f);}
            float xx=gx+u*gw,yy=gy+gh*(1-.9f*t);if(!n)curve.startNewSubPath(xx,yy);else curve.lineTo(xx,yy);}
        p.g.setColour(row==3?gold:cyan);p.g.strokePath(curve,juce::PathStrokeType(1.7f));
        p.text(row==3?"LOW         MID         HIGH":"20 Hz                 20 kHz",gx+4,gy+gh-12,gw-8,11,plain(dim,11,Weight::Medium));
    }else if(row==2){
        float px=gx+gw*(.12f+.45f*value(6));p.fill(gx+8,gy+gh*.5f,gw-16,1,dim.withAlpha(.4f));p.fill(gx+8,gy+gh*.2f,2,gh*.6f,white);p.fill(px,gy+gh*.15f,2,gh*.7f,gold);
        for(unsigned n=0;n<8;++n)p.fill(px+10+n*(gw-px+gx-20)/8,gy+gh*.3f,1,std::max(2.f,gh*.4f*(1-n/9.f)),cyan.withAlpha(.7f));
        p.text("DIRECT      PRE-DELAY      TAIL",gx+4,gy+gh-13,gw-8,12,plain(dim,11,Weight::Bold));
    }else if(row==6){
        const unsigned slot=unsigned(std::lround(value(18)*8));
        constexpr const char* states[]={"ALGORITHMIC","LOADING","READY","BAD IR","NO IR"};
        const auto state=std::min(4u,unsigned(panel.bloom_ir_status));
        const auto heading=slot?juce::String("IR ")+juce::String(slot)+" / 8  -  "+(panel.bloom_ir_known?states[state]:"USER WAV"):juce::String("ALGORITHMIC SPACE");
        p.text(heading,gx+6,gy+3,gw-12,14,p.fit(heading,plain(slot?cyan:gold,12,Weight::Bold),gw-12,type::floor));
        juce::String filename=slot?juce::String::fromUTF8(panel.bloom_ir_filename):"Select an IR with K1";
        if(slot&&filename.isEmpty())filename=panel.bloom_ir_known?"No WAV in this slot":"Update Bloom for filenames";
        auto style=plain(white,13,Weight::Bold);
        // Scroll long filenames at readable size instead of clipping or shrinking below the UI floor.
        if(p.width(filename,style)>gw-12){const auto loop=filename+"      "+filename;const int at=int(s.now/350)%(filename.length()+6);filename=loop.substring(at);while(filename.isNotEmpty()&&p.width(filename,style)>gw-12)filename=filename.dropLastCharacters(1);}
        p.text(filename,gx+6,gy+22,gw-12,16,style);
        p.text("K1 SELECT IR / K2 IR BLEND",gx+6,gy+gh-14,gw-12,12,plain(dim,11,Weight::Medium));
    }else if(row==5){
        constexpr const char* labels[]={"SHIMMER","REVERSE","FREEZE"};for(unsigned n=0;n<3;++n){float bx=gx+n*gw/3;p.text(labels[n],bx+3,gy+5,gw/3-6,12,plain(n==2&&value(17)>.5f?gold:cyan,11,Weight::Bold));float amount=value(15+n);p.fill(bx+5,gy+25,gw/3-12,3,dim.withAlpha(.2f));p.fill(bx+5,gy+25,(gw/3-12)*amount,3,n==2?gold:cyan);}
        p.text("OCTAVE UP / REVERSED GRAINS / HOLD",gx+4,gy+gh-13,gw-8,12,p.fit("OCTAVE UP / REVERSED GRAINS / HOLD",plain(dim,11,Weight::Medium),gw-8,type::floor));
    }else if(row==7){
        for(unsigned n=0;n<30;++n){float u=n/29.f,angle=float(s.now)*.0005f*(.1f+value(21)*2)+u*9;float yy=gy+gh*.5f+std::sin(angle)*gh*.35f*value(22);p.circle(gx+6+u*(gw-12),yy,1+value(23),n%2?gold:cyan.withAlpha(.4f+std::min(1.f,peak)));}
    }else{
        // Predicted decay envelope is controlled by Space/Decay, not a fake audio meter.
        float decay=.25f*std::pow(120.f,value(1));for(unsigned line=0;line<8;++line){juce::Path wave;for(unsigned n=0;n<=80;++n){float u=n/80.f,env=std::exp(-u*(2+6/(1+decay)));float yy=gy+gh*.8f-env*gh*(.16f+line*.075f)*(1+.08f*std::sin(n*.35f+line));float xx=gx+u*gw;if(!n)wave.startNewSubPath(xx,yy);else wave.lineTo(xx,yy);}p.g.setColour((line%2?gold:cyan).withAlpha(.22f+line*.07f));p.g.strokePath(wave,juce::PathStrokeType(1));}
        p.text(juce::String(decay,1)+" s / "+juce::String(int(value(2)*100))+"% wet",gx+5,gy+3,gw-10,14,plain(white,12,Weight::Bold));
    }
    const float wy=y+h-15;p.text("OUT",x+7,wy-1,27,11,plain(dim,11,Weight::Bold));
    juce::Path output;for(unsigned n=0;n<64;++n){float v=bypass?0:int16_t(panel.scope[n])/32768.f;float xx=x+36+(w-47)*n/63.f,yy=wy+4-std::clamp(v,-1.f,1.f)*5;if(!n)output.startNewSubPath(xx,yy);else output.lineTo(xx,yy);}p.g.setColour(cyan);p.g.strokePath(output,juce::PathStrokeType(1));return true;
}
}
