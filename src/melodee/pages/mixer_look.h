// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/pages/common.h"
#include "melodee/shell/bindings.h"
#include <cmath>

namespace melodee::mixer_ui {
using namespace ui;
inline juce::Colour ink() { return hex(0xC47BFF); }
inline juce::Colour teal() { return hex(0x55E0C7); }
struct Look {
    std::array<juce::Colour*, 15> fields {&colour::bg,&colour::panel,&colour::surf,&colour::raised,&colour::quiet,&colour::line,&colour::text,&colour::sec,&colour::mid,&colour::dim,&colour::rec,&colour::amber,&colour::play,&colour::ice,&colour::text};
    std::array<juce::Colour, 15> saved;
    Look() {
        for (unsigned i=0;i<fields.size();++i) saved[i]=*fields[i];
        colour::bg=hex(0x09090E);colour::panel=colour::surf=hex(0x15151F);colour::raised=hex(0x251A35);colour::quiet=hex(0x111119);
        colour::line=hex(0x343343);colour::text=hex(0xF4F2FB);colour::sec=hex(0xD1CBDF);colour::mid=hex(0xA8A3B9);colour::dim=hex(0x777183);
        colour::rec=hex(0xE95389);colour::amber=hex(0xF4C86A);colour::play=teal();colour::ice=ink();
    }
    ~Look() { for (int i=int(fields.size())-1;i>=0;--i) *fields[unsigned(i)]=saved[unsigned(i)]; }
};
inline void label(Paint& p,const juce::String& s,float x,float y,float w,float h,float size=11,juce::Colour c=colour::text,juce::Justification align=juce::Justification::centredLeft) {
    auto style=plain(c,size,Weight::Bold); p.text(s,x,y,w,h,p.fit(s,style,w,type::floor),align);
}
inline void header(Paint& p,const juce::String& title,const juce::String& where,const std::array<juce::String,3>& keys) {
    p.g.fillAll(colour::bg);soft_keys(p,SoftKeys(keys,-1,ink()));
    label(p,title,8,28,178,19,13,ink());label(p,where,188,29,124,18,11,colour::text,juce::Justification::centredRight);
}
inline juce::String level_text(int level,bool pad=false) {
    if (level<=0) return "-inf";
    return juce::String(pad ? volume_db(level) : 40.f*std::log10(float(level)/100.f),1);
}
inline juce::String pan_text(int pan) { return !pan ? "CENTER" : (pan<0?"L":"R")+juce::String(std::abs(pan)); }
inline void meter(Paint& p,float x,float y,float w,float h,float peak) {
    p.fill(x,y,w,h,hex(0x292937),1);float shown=peak>1e-5f?std::clamp((20.f*std::log10(peak)+48.f)/48.f,0.f,1.f):0.f;
    unsigned steps=unsigned(h/4);for(unsigned i=0;i<unsigned(shown*steps);++i)p.fill(x,y+h-3-i*4,w,2,i+1==steps?colour::rec:i>steps*8/10?colour::amber:teal());
}
inline void cards(Paint& p,const Snapshot& s) {
    auto cards=cards_of(s);auto bindings=page_bindings(s.project,s.ui,s.pattern);
    for(unsigned i=0;i<3;++i){cards[i].shade=cards[i].track=ink();cards[i].shift=0;
        if(bindings[i].kind==Binding::Kind::Level || bindings[i].kind==Binding::Kind::MasterLevel){cards[i].value=level_text(bindings[i].value);cards[i].unit="dB";}
    }
    knob_strip(p,cards);
}
}
