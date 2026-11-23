// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Tape Workshop contributors
#include "melodee/pages/plugin_pro_ui.h"
#include "melodee/pages/bloom_view.h"
#include "melodee/pages/prism_view.h"
#include "melodee/pages/plugin_pro_layout.h"
#include "melodee/pages/plugin_waveform.h"
#include "melodee/pages/analog_meter.h"
#include "melodee/pages/layer_house_shapes.h"
#include "melodee/plugin/gate_curve.h"
#include "melodee/model/scales.h"
#include "melodee/shell/stage_map.h"
#include <cctype>

namespace melodee {
namespace {
using namespace ui;
using C=juce::Colour;
constexpr float tau=6.283185307f;
bool equal_ascii(const char* a,const char* b) {
    while(*a&&*b) {if(std::tolower(static_cast<unsigned char>(*a++))!=std::tolower(static_cast<unsigned char>(*b++)))return false;}
    return *a==*b;
}
float control(const PluginPanel& p,const char* name,float fallback=0.f) {
    for(unsigned n=0;n<p.visual_count;++n)if(equal_ascii(p.visual_controls[n].name,name))return p.visual_controls[n].value;
    for(const auto& value:p.params)if(value.used&&equal_ascii(value.name,name))return value.value/float(plugin_param_max);
    return fallback;
}
float peak(const PluginPanel& p,bool right=false) {
    float result=0;for(auto pair:right?p.scope_right:p.scope)
        result=std::max({result,std::abs(int16_t(pair)/32768.f),std::abs(int16_t(pair>>16)/32768.f)});
    return result;
}
void stroke(Paint& p,const juce::Path& path,C c,float thickness=1.f) {
    p.g.setColour(c);p.g.strokePath(path,juce::PathStrokeType(thickness,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
}
void panel(Paint& p,float x,float y,float w,float h,C base,C edge,bool inset=true) {
    p.fill(x+.5f,y+1.5f,w,h,C(0xff000000).withAlpha(.5f),3);
    p.g.setGradientFill(juce::ColourGradient(base.brighter(inset?.03f:.18f),x,y,base.darker(.22f),x,y+h,false));
    p.g.fillRoundedRectangle(x,y,w,h,3);p.ring(x,y,w,h,edge.withAlpha(.35f),3);
    p.fill(x+3,y+1,w-6,.65f,C(0xffffffff).withAlpha(inset?.04f:.2f));
}
void chassis(Paint& p,float x,float y,float w,float h,const ProStyle& style) {
    const C bg(style.background);panel(p,x,y,w,h,bg,C(0xff777970),false);
    const bool wood=style.material==ProMaterial::Walnut;
    if(wood)for(int n=0;n<28;++n) {
        const float yy=y+2+n*(h-4)/28;juce::Path grain;
        grain.startNewSubPath(x+2,yy);grain.cubicTo(x+w*.23f,yy+2,x+w*.71f,yy-2,x+w-2,yy+1);
        stroke(p,grain,C(n%3?0xff170c06:0xffd78e53).withAlpha(.18f),.55f);
    }
    if(style.material==ProMaterial::Brushed||style.material==ProMaterial::BlueSteel)
        for(int n=2;n<int(h)-2;n+=3)p.fill(x+3,y+n,w-6,.4f,C(0xffffffff).withAlpha(n%2?.045f:.025f));
    if(style.material==ProMaterial::Ebony) {
        p.g.setGradientFill(juce::ColourGradient(C(0xffffffff).withAlpha(.12f),x,y,C(0xffffffff).withAlpha(0.f),x,y+h*.4f,false));
        p.g.fillRoundedRectangle(x+2,y+2,w-4,h*.38f,3);
    }
    if(style.instrument==ProInstrument::Hera) {
        p.fill(x+2,y+3,7,h-6,C(0xff75472b),2);p.fill(x+w-9,y+3,7,h-6,C(0xff75472b),2);
        for(int n=6;n<int(h)-4;n+=6){p.fill(x+3,y+n,4,.5f,C(0xffba8147).withAlpha(.4f));p.fill(x+w-8,y+n,4,.5f,C(0xffba8147).withAlpha(.4f));}
    }
    for(float xx:{x+4,x+w-4})for(float yy:{y+4,y+h-4}) {
        p.circle(xx,yy,1.6f,C(0xff0b0c0c));p.circle(xx,yy,1.1f,C(0xff7c7d70));p.fill(xx-.8f,yy,1.6f,.5f,C(0xff151616));
    }
}
void knob(Paint& p,const PluginPanelParam& value,float x,float y,float w,float h,const ProStyle& style,unsigned k) {
    if(!value.used)return;
    const C ink(style.accent),text=style.dark_text?C(0xff242824):C(0xfff0f0e7);
    const float radius=std::min(h>50?24.f:14.f,h*.33f),cx=x+radius+3,cy=y+h*.5f;
    p.circle(cx+.6f,cy+1.3f,radius+1,C(0xff000000).withAlpha(.55f));
    p.g.setGradientFill(juce::ColourGradient(C(0xffa0a49d),cx-radius,cy-radius,C(0xff1a1c1d),cx+radius,cy+radius,false));
    p.g.fillEllipse(cx-radius,cy-radius,2*radius,2*radius);
    p.circle(cx,cy,radius-1.5f,C(0xff202529));
    p.g.setGradientFill(juce::ColourGradient(C(style.material==ProMaterial::Cream?0xffccccbf:0xff747a7b),cx-radius,cy-radius,
                                           C(0xff202325),cx+radius,cy+radius,false));
    p.g.fillEllipse(cx-radius+3,cy-radius+3,2*(radius-3),2*(radius-3));
    if(style.instrument==ProInstrument::Hera||style.instrument==ProInstrument::Grand) {
        juce::Path reflection;reflection.startNewSubPath(cx-radius*.55f,cy-radius*.2f);
        reflection.lineTo(cx+radius*.32f,cy-radius*.6f);reflection.lineTo(cx+radius*.6f,cy+radius*.2f);reflection.closeSubPath();
        p.g.setColour(C(0xffffffff).withAlpha(.18f));p.g.fillPath(reflection);
    }
    const float value01=std::clamp(value.value/float(plugin_param_max),0.f,1.f),angle=-2.35f+4.7f*value01;
    juce::Path ring;ring.addCentredArc(cx,cy,radius+2,radius+2,0,-2.35f,-2.35f+4.7f*value01,true);stroke(p,ring,ink,1.1f);
    p.g.setColour(style.instrument==ProInstrument::Rack?C(0xffd84936):ink);
    p.g.drawLine(cx+std::sin(angle)*radius*.2f,cy-std::cos(angle)*radius*.2f,cx+std::sin(angle)*radius*.67f,cy-std::cos(angle)*radius*.67f,1.5f);
    const float tx=x+2*radius+8,tw=w-2*radius-10;
    const auto label=juce::String(value.name);auto label_style=plain(text,11,Weight::Bold);
    if(p.width(label,label_style)>tw) {
        const auto lines=p.wrap(label,label_style,tw,2);
        for(int n=0;n<lines.size();++n)p.text(lines[n],tx,y+2+n*11,tw,12,label_style);
    } else {
        p.text(label,tx,y+(h<28?(h-13)/2:1),tw,13,label_style);
        if(h>=28)p.text(value.text,tx,y+14,tw,13,plain(ink,11,Weight::Bold));
    }
    (void)k;
}
void knob_column(Paint& p,const PluginPanel& panel_data,float x,float y,float w,float h,const ProStyle& style) {
    panel(p,x,y,w,h,C(style.background).darker(.28f),C(style.accent));
    unsigned count=0;for(const auto& value:panel_data.params)if(value.used)++count;
    if(!count)return;const float lane=h/count;
    for(unsigned k=0,at=0;k<3;++k)if(panel_data.params[k].used)knob(p,panel_data.params[k],x+4,y+(at++)*lane+1,w-8,lane-2,style,k);
}
void knob_row(Paint& p,const PluginPanel& data,float x,float y,float w,float h,const ProStyle& style) {
    const float cw=w/3;
    for(unsigned k=0;k<3;++k)if(data.params[k].used) {
        const auto& value=data.params[k];const float left=x+k*cw,cx=left+cw*.5f,cy=y+h*.48f,r=std::min(13.f,h*.32f);
        const C ink(style.accent),text=style.dark_text?C(0xff232820):C(0xfff1f0e7);
        p.circle(cx+.8f,cy+1.2f,r+1,C(0xff000000).withAlpha(.55f));
        p.g.setGradientFill(juce::ColourGradient(C(0xffc4c7bd),cx-r,cy-r,C(0xff1d2021),cx+r,cy+r,false));p.g.fillEllipse(cx-r,cy-r,2*r,2*r);
        p.circle(cx,cy,r-2,C(0xff23282a));
        p.g.setGradientFill(juce::ColourGradient(C(0xff888e8c),cx-r,cy-r,C(0xff242829),cx+r,cy+r,false));p.g.fillEllipse(cx-r+4,cy-r+4,2*(r-4),2*(r-4));
        const float a=-2.35f+4.7f*std::clamp(value.value/float(plugin_param_max),0.f,1.f);
        p.g.setColour(ink);p.g.drawLine(cx,cy,cx+std::sin(a)*(r-4),cy-std::cos(a)*(r-4),1.6f);
        const auto label=juce::String(value.name);auto st=plain(text,11,Weight::Bold);
        p.text(label,left+2,y+h-13,cw-4,13,p.fit(label,st,cw-4,type::floor),juce::Justification::centred);
    }
}
struct History {
    std::array<std::array<float,76>,6> spectrum{};
    std::array<float,64> cents{};
    uint32_t name_hash=0,last_ms=0,stamp=0;
    unsigned count=0,cents_at=0;
};
History& history(const Snapshot& s) {
    static std::array<History,max_plugins> states;
    const auto& panel_data=s.ui.plugin_panel;auto& state=states[std::min<unsigned>(panel_data.entry,max_plugins-1)];
    uint32_t hash=2166136261u;for(const char* c=panel_data.name;*c;++c)hash=(hash^static_cast<unsigned char>(*c))*16777619u;
    if(state.name_hash!=hash){state={};state.name_hash=hash;}
    if((!state.count||s.now-state.last_ms>=40)&&(!state.count||panel_data.scope_stamp!=state.stamp)) {
        for(unsigned n=state.spectrum.size()-1;n>0;--n)state.spectrum[n]=state.spectrum[n-1];
        state.spectrum[0]=panel_data.eq_spectrum;state.count=std::min<unsigned>(state.count+1,state.spectrum.size());
        state.cents[state.cents_at++%state.cents.size()]=panel_data.vocal_cents;
        state.last_ms=s.now;state.stamp=panel_data.scope_stamp;
    }
    return state;
}
void screen(Paint& p,float x,float y,float w,float h,C ink,const char* title) {
    panel(p,x,y,w,h,C(0xff080d10),ink);
    for(int n=1;n<6;++n)p.fill(x+4+n*(w-8)/6,y+15,.5f,h-19,ink.withAlpha(.10f));
    p.text(title,x+5,y+2,w-10,12,plain(ink,11,Weight::Bold));
}
void waveform(Paint& p,const PluginPanel& data,float x,float y,float w,float h,C ink,bool bypass,bool stereo=false) {
    screen(p,x,y,w,h,ink,stereo?"STEREO OUTPUT":"AUDIO OUTPUT");
    const unsigned channels=stereo?2:1;
    for(unsigned side=0;side<channels;++side) {
        const float lane=(h-17)/channels,mid=y+16+side*lane+lane/2;
        p.fill(x+4,mid,w-8,.5f,ink.withAlpha(.23f));
        juce::Path trace;
        for(unsigned n=0;n<128;++n) {
            float sample=0;
            if(!bypass) {
                if(data.signal_available)sample=data.signal[side][n]/32768.f;
                else {const auto v=(side?data.scope_right:data.scope)[n/2];sample=(int16_t(v)+int16_t(v>>16))/65536.f;}
            }
            const float px=x+5+(w-10)*n/127,py=mid-sample*(lane*.44f);
            if(n)trace.lineTo(px,py);else trace.startNewSubPath(px,py);
        }
        stroke(p,trace,side?ink.withRotatedHue(.12f):ink,1.2f);
    }
}
void spectrum_plot(Paint& p,const PluginPanel& data,float x,float y,float w,float h,C ink,bool bypass,const char* title="OUTPUT SPECTRUM") {
    screen(p,x,y,w,h,ink,title);juce::Path line,area;
    const float floor=y+h-4;area.startNewSubPath(x+4,floor);
    for(unsigned n=0;n<data.eq_spectrum.size();++n) {
        const float xx=x+4+n*(w-8)/float(data.eq_spectrum.size()-1),yy=floor-(h-22)*(bypass?0.f:std::clamp(data.eq_spectrum[n],0.f,1.f));
        if(n)line.lineTo(xx,yy);else line.startNewSubPath(xx,yy);area.lineTo(xx,yy);
    }
    area.lineTo(x+w-4,floor);area.closeSubPath();
    p.g.setGradientFill(juce::ColourGradient(ink.withAlpha(.48f),x,y+15,ink.withAlpha(.03f),x,floor,false));p.g.fillPath(area);stroke(p,line,ink,1.4f);
}
void waterfall(Paint& p,const Snapshot& s,float x,float y,float w,float h,C ink,bool bypass) {
    screen(p,x,y,w,h,ink,"SPECTRAL HISTORY");auto& state=history(s);
    for(int depth=int(state.count)-1;depth>=0;--depth) {
        juce::Path line;const float dx=depth*3.5f,dy=depth*3.f;
        for(unsigned n=0;n<76;++n) {
            const float xx=x+5+dx+n*(w-31)/75,yy=y+h-5-dy-(h-37)*(bypass?0.f:state.spectrum[unsigned(depth)][n]);
            if(n)line.lineTo(xx,yy);else line.startNewSubPath(xx,yy);
        }
        stroke(p,line,ink.interpolatedWith(C(0xffb9c6ce),depth/10.f).withAlpha(1.f-depth*.12f),depth?1.f:1.6f);
    }
}
void envelope(Paint& p,const PluginPanel& data,float x,float y,float w,float h,C ink,const char* prefix="") {
    screen(p,x,y,w,h,ink,"ENVELOPE CONTROLS");
    const auto v=[&](const char* key,float fallback){const auto name=juce::String(prefix)+key;return control(data,name.toRawUTF8(),control(data,key,fallback));};
    const float a=v("Attack",0),d=v("Decay",.4f),sus=v("Sustain",.75f),r=v("Release",.3f);
    const float left=x+8,span=w-16,base=y+h-8,top=y+20,range=base-top;
    const float ax=left+span*(.07f+.22f*a),dx=ax+span*(.08f+.22f*d),rx=left+span*(.77f+.2f*r),sy=base-range*sus;
    juce::Path line;line.startNewSubPath(left,base);line.quadraticTo(left+(ax-left)*.6f,top,ax,top);
    line.quadraticTo(dx,sy,dx,sy);line.lineTo(std::max(dx+2,rx-span*.17f),sy);line.quadraticTo(rx,base,rx,base);
    auto area=line;area.lineTo(left,base);area.closeSubPath();p.g.setColour(ink.withAlpha(.16f));p.g.fillPath(area);stroke(p,line,ink,1.5f);
    for(const auto point:{juce::Point<float>(ax,top),juce::Point<float>(dx,sy),juce::Point<float>(rx,base)})p.circle(point.x,point.y,2.2f,ink.brighter(.3f));
}
void keyboard(Paint& p,const Snapshot& s,float x,float y,float w,float h,C ink,bool grand,bool bypass) {
    const float key_h=std::min(25.f,h*.37f),ky=y+h-key_h;
    screen(p,x,y,w,h,ink,grand?"GRAND / HELD NOTES":"PIANO / HELD NOTES");
    const float gh=std::max(3.f,h-key_h-20),mid=y+17+gh/2;
    juce::Path trace;
    for(unsigned n=0;n<128;++n) {
        const float value=!bypass&&s.ui.plugin_panel.signal_available?s.ui.plugin_panel.signal[0][n]/32768.f:0.f;
        const float xx=x+5+n*(w-10)/127,yy=mid-value*gh*.45f;
        if(n)trace.lineTo(xx,yy);else trace.startNewSubPath(xx,yy);
    }
    stroke(p,trace,ink,1.f);
    constexpr int white[]{0,2,4,5,7,9,11},black[]{1,3,-1,6,8,10,-1};
    unsigned held=0;for(int key:s.held_keys)if(key>=0)held|=1u<<(key%12);
    const float kw=(w-6)/14;
    for(int key=0;key<14;++key) {
        const float xx=x+3+key*kw;
        p.g.setGradientFill(juce::ColourGradient(C(0xfff4eddb),xx,ky,C(0xffa9a58f),xx,ky+key_h,false));
        p.g.fillRect(xx,ky,kw-1,key_h);
        if(held&(1u<<white[key%7]))p.fill(xx,ky+key_h-4,kw-1,4,ink);
        if(black[key%7]>=0) {
            const float bx=xx+kw*.7f;p.fill(bx,ky,kw*.55f,key_h*.61f,C(0xff080a0b),1);
            p.fill(bx+1,ky+1,kw*.55f-2,key_h*.45f,C(0xff30383b),1);
            if(held&(1u<<black[key%7]))p.fill(bx,ky+key_h*.46f,kw*.55f,3,ink);
        }
    }
}
void phase_plot(Paint& p,const PluginPanel& data,float x,float y,float w,float h,C ink,bool bypass) {
    screen(p,x,y,w,h,ink,"STEREO FIELD");const float cx=x+w/2,cy=y+16+(h-20)/2,rx=(w-16)/2,ry=(h-24)/2;
    p.g.setColour(ink.withAlpha(.18f));p.g.drawEllipse(cx-rx,cy-ry,2*rx,2*ry,.7f);p.g.drawLine(cx-rx,cy,cx+rx,cy,.6f);p.g.drawLine(cx,cy-ry,cx,cy+ry,.6f);
    if(bypass||!data.signal_available)return;
    juce::Path line;for(unsigned n=0;n<128;++n) {
        const float xx=cx+rx*data.signal[0][n]/32768.f,yy=cy-ry*data.signal[1][n]/32768.f;
        if(n)line.lineTo(xx,yy);else line.startNewSubPath(xx,yy);
    }stroke(p,line,ink,1.1f);
}
float table_sample(const PluginPanel& data,unsigned layer,float t,float morph_delta) {
    constexpr unsigned partners[]{1,5,4,11,8,12,9,10,15,14,8,12,5,11,9,8};
    const unsigned shape=std::min(data.layer_shape[layer],15u);
    const float pivot=.5f-.33984375f*std::clamp(data.layer_warp[layer],0.f,1.f);
    const float skew=t<pivot?.5f*t/pivot:.5f+.5f*(t-pivot)/(1-pivot),pos=skew*8;
    const unsigned at=unsigned(pos)%8,next=(at+1)%8;const float fraction=pos-std::floor(pos);
    const auto& base=layer_house_shapes[shape];const auto& alt=layer_house_shapes[partners[shape]];
    const float a=base[at]+fraction*(base[next]-base[at]),b=alt[at]+fraction*(alt[next]-alt[at]);
    const float morph=std::clamp(data.layer_morph[layer]+morph_delta,0.f,1.f);
    return (a+morph*(b-a))/128.f;
}
void wavetable(Paint& p,const PluginPanel& data,float x,float y,float w,float h,C ink,unsigned layer) {
    screen(p,x,y,w,h,ink,layer?"WAVETABLE B":"WAVETABLE A");
    for(int depth=6;depth>=0;--depth) {
        juce::Path line;for(int n=0;n<=64;++n) {
            const float u=n/64.f,xx=x+5+depth*2.5f+u*(w-26),yy=y+h-12-depth*3.8f-table_sample(data,layer,u,(depth-3)*.075f)*(h-37)*.44f;
            if(n)line.lineTo(xx,yy);else line.startNewSubPath(xx,yy);
        }stroke(p,line,depth==3?C(0xffe4ffff):ink.withAlpha(.25f+.09f*(6-depth)),depth==3?1.5f:1.f);
    }
}
void operators(Paint& p,const PluginPanel& data,float x,float y,float w,float h,C ink,ProInstrument instrument) {
    const bool dexed=instrument==ProInstrument::Dexed;
    screen(p,x,y,w,h,ink,dexed?"OPERATOR ENABLE":"OP LEVEL OFFSETS");
    const unsigned selected=data.row>=4?std::min<unsigned>((data.row-4)/2,5):6;
    const float cw=(w-10)/6;
    for(unsigned n=0;n<6;++n) {
        const auto name="OP"+juce::String(n+1)+(dexed?"":" Level");
        const float v=control(data,name.toRawUTF8(),dexed?1.f:.5f),xx=x+5+n*cw;
        panel(p,xx,y+19,cw-2,h-25,n==selected?ink.withAlpha(.24f):C(0xff14231b),ink);
        p.text(juce::String(n+1),xx,y+20,cw-2,12,plain(ink,11,Weight::Bold),juce::Justification::centred);
        if(dexed) p.circle(xx+cw/2-1,y+h-15,3,v>.5f?ink:C(0xff344037));
        else {
            const float middle=y+19+(h-25)*.64f,offset=(v-.5f)*(h-40);
            p.fill(xx+4,middle,cw-10,1,ink.withAlpha(.3f));
            p.fill(xx+4,middle-std::max(0.f,offset),cw-10,std::max(1.f,std::abs(offset)),ink.withAlpha(.75f));
        }
    }
}
void offsets(Paint& p,const PluginPanel& data,float x,float y,float w,float h,C ink) {
    screen(p,x,y,w,h,ink,"FACTORY ENVELOPE OFFSETS");const float lane=(h-20)/3;
    for(unsigned k=0;k<3;++k)if(data.params[k].used) {
        const float yy=y+18+k*lane,mid=x+w*.55f,v=data.params[k].value/float(plugin_param_max)-.5f;
        p.text(k==0?"ATK":k==1?"DEC":"REL",x+5,yy,w*.2f,12,plain(ink,11,Weight::Bold));
        p.fill(mid,yy+2,1,lane-2,ink.withAlpha(.3f));const float span=(w*.70f)*v;
        p.fill(mid+std::min(0.f,span),yy+6,std::max(1.f,std::abs(span)),3,ink,1);
    }
}
void choir(Paint& p,const PluginPanel& data,float x,float y,float w,float h,C ink,bool bypass) {
    panel(p,x,y,w,h,C(0xffd9cbac),ink);const C low(0xff679c78),high(0xffcb8d60),dark(0xff3f463b);
    const float cx=x+w/2,cy=y+h-8,rx=w*.44f,ry=(h-23)*.85f,cross=control(data,"Crossfade",.5f);
    for(int row=0;row<3;++row) {
        juce::Path arc;arc.addCentredArc(cx,cy,rx-row*7,ry-row*7,0,-1.2f,1.2f,true);stroke(p,arc,row%2?low:high,5.f);
    }
    for(unsigned singer=0;singer<12;++singer) {
        const float angle=-1.1f+2.2f*singer/11,energy=bypass?0.f:singer<6?(1-cross)*peak(data):cross*peak(data,true);
        p.circle(cx+std::sin(angle)*(rx-13),cy-std::cos(angle)*(ry-13),2.3f,energy>.02f?(singer<6?low:high):dark.withAlpha(.55f));
    }
    p.text("LOW",x+5,y+3,w/2-5,12,plain(dark,11,Weight::Bold));p.text("HIGH",x+w/2,y+3,w/2-5,12,plain(dark,11,Weight::Bold),juce::Justification::centredRight);
    const float marker=x+9+(w-18)*cross;p.fill(x+8,y+h-4,w-16,2,dark.withAlpha(.25f));p.circle(marker,y+h-3,3,ink);
}
void room(Paint& p,const PluginPanel& data,float x,float y,float w,float h,C ink) {
    screen(p,x,y,w,h,ink,"ROOM / REFLECTION CONTROLS");
    const float size=control(data,"Room Size",control(data,"Room",.4f)),damp=control(data,"Room Damping",.5f);
    const float cx=x+w/2,front=y+h-6,back=y+23,side=w*(.18f+.15f*size);
    juce::Path floor;floor.startNewSubPath(x+6,front);floor.lineTo(cx-side,back);floor.lineTo(cx+side,back);floor.lineTo(x+w-6,front);floor.closeSubPath();
    p.g.setColour(ink.withAlpha(.09f));p.g.fillPath(floor);stroke(p,floor,ink.withAlpha(.6f));
    for(int wall=1;wall<=5;++wall){const float t=wall/6.f,yy=back+(front-back)*t;stroke(p,[&]{juce::Path l;l.startNewSubPath(cx-side-(w/2-side-6)*t,yy);l.lineTo(cx+side+(w/2-side-6)*t,yy);return l;}(),ink.withAlpha(.45f*(1-damp)+.06f));}
    p.circle(cx,back+9,3,ink);p.fill(cx-12,front-12,24,7,C(0xffe3d9bd),2);
}
void lfo(Paint& p,const PluginPanel& data,float x,float y,float w,float h,C ink) {
    screen(p,x,y,w,h,ink,"MODULATION CONTROL SHAPE");
    float amount=control(data,"Vibrato",control(data,"Drift",control(data,"Tremolo",.5f)));
    float rate=control(data,"LFO Speed",control(data,"Rate",control(data,"Vib Rate",control(data,"Drift Rate",.3f))));
    for(const auto& c:data.params)if(c.used){const auto name=juce::String(c.name).toLowerCase();if(name.contains("rate")||name.contains("speed"))rate=c.value/float(plugin_param_max);if(name.contains("depth")||name.contains("amount"))amount=c.value/float(plugin_param_max);}
    juce::Path line;const float mid=y+18+(h-22)/2;
    for(int n=0;n<=96;++n){const float u=n/96.f,xx=x+5+u*(w-10),yy=mid-std::sin(u*tau*(1+rate*4))*(h-25)*.4f*amount;if(n)line.lineTo(xx,yy);else line.startNewSubPath(xx,yy);}stroke(p,line,ink,1.5f);
}
void spatial(Paint& p,const PluginPanel& data,float x,float y,float w,float h,C ink,bool reverb) {
    screen(p,x,y,w,h,ink,reverb?"REVERB CONTROL SHAPE":"DELAY CONTROL SHAPE");
    const float feedback=control(data,"Feedback",control(data,"Room Tail",.4f)),time=control(data,"Delay Time",control(data,"Echo Time",.35f));
    const float mix=control(data,"Reverb",control(data,"Echo",control(data,"Delay",.4f))),base=y+h-5;
    if(reverb) {
        juce::Path tail;for(int n=0;n<=80;++n){const float t=n/80.f,xx=x+6+t*(w-12),yy=base-(h-25)*(.2f+.8f*mix)*std::exp(-t*(8-6*feedback));if(n)tail.lineTo(xx,yy);else tail.startNewSubPath(xx,yy);}stroke(p,tail,ink,1.6f);
    } else for(int n=0;n<7;++n){const float xx=x+7+n*(w-14)*(.07f+.06f*time),height=(h-24)*mix*std::pow(.1f+.85f*feedback,float(n));p.fill(xx,base-height,3,height,ink.withAlpha(1.f-n*.1f),1);}
}
void mixer(Paint& p,const PluginPanel& data,float x,float y,float w,float h,C ink) {
    screen(p,x,y,w,h,ink,"CURRENT BANK / LEVELS");const float cw=(w-12)/3;
    for(unsigned k=0;k<3;++k)if(data.params[k].used) {
        const float xx=x+6+k*cw,v=data.params[k].value/float(plugin_param_max),rail=y+19,travel=h-31;
        for(int tick=0;tick<6;++tick)p.fill(xx+3,rail+tick*travel/5,cw-8,.5f,ink.withAlpha(.15f));
        p.fill(xx+cw/2-1,rail,2,travel,C(0xff030405));
        const float pos=rail+(1-v)*travel;
        p.g.setGradientFill(juce::ColourGradient(C(0xffc2c7bf),xx,pos-2,C(0xff41494a),xx,pos+3,false));p.g.fillRoundedRectangle(xx+cw*.25f,pos-2,cw*.5f,5,1);
        p.fill(xx+cw*.26f,pos,cw*.48f,.8f,ink);
    }
}
void gate(Paint& p,const PluginPanel& data,float x,float y,float w,float h,C ink) {
    screen(p,x,y,w,h,ink,"GATE INPUT / OUTPUT");const auto& c=data.contour_curve;
    juce::Path line;for(int n=0;n<=80;++n) {
        const float in=-72+72*n/80.f,out=in+gate_gain_db(in,c[0],std::max(1.f,c[1]),c[3],c[2],int(c[4]));
        const float xx=x+6+n*(w-12)/80,yy=y+h-5-std::clamp((out+72)/72,0.f,1.f)*(h-24);
        if(n)line.lineTo(xx,yy);else line.startNewSubPath(xx,yy);
    }stroke(p,line,ink,1.6f);
    const float tx=x+6+std::clamp((c[0]+72)/72,0.f,1.f)*(w-12);p.fill(tx,y+19,1,h-24,ink.withAlpha(.5f));
}
void compression(Paint& p,const PluginPanel& data,float x,float y,float w,float h,C ink,bool bypass) {
    screen(p,x,y,w,h,ink,"GAIN REDUCTION HISTORY");juce::Path line;
    for(unsigned n=0;n<data.compressor_history.size();++n) {
        const float reduction=bypass?0.f:float(data.compressor_history[n]>>20)/1023.f;
        const float xx=x+5+n*(w-10)/float(data.compressor_history.size()-1),yy=y+19+reduction*(h-24);
        if(n)line.lineTo(xx,yy);else line.startNewSubPath(xx,yy);
    }stroke(p,line,C(0xffffbe80),1.5f);
}
void pitch(Paint& p,const Snapshot& s,float x,float y,float w,float h,C ink,bool bypass) {
    const auto& data=s.ui.plugin_panel;screen(p,x,y,w,h,ink,"PITCH / TARGET / CORRECTION");auto& state=history(s);
    const float mid=y+18+(h-22)/2;p.fill(x+4,mid,w-8,.6f,ink.withAlpha(.35f));
    juce::Path line;for(unsigned n=0;n<64;++n){const float xx=x+5+n*(w-10)/63,yy=mid-(bypass?0.f:std::clamp(state.cents[(state.cents_at+n)%64],-100.f,100.f))*(h-24)/200;if(n)line.lineTo(xx,yy);else line.startNewSubPath(xx,yy);}stroke(p,line,ink,1.2f);
    const auto note=data.vocal_tune_ready&&!bypass&&peak(data)>.0003f?juce::String(note_name(std::clamp(data.vocal_target,0,127))):juce::String("--");
    p.text(note,x+w-47,y+16,40,19,plain(C(0xfff0dd9d),16,Weight::Bold),juce::Justification::centredRight);
}
void model(Paint& p,const PluginPanel& data,float x,float y,float w,float h,C ink,bool plaits) {
    panel(p,x,y,w,h,C(plaits?0xff172622:0xff171d20),ink);
    const auto number=int(std::lround(control(data,plaits?"Engine":"ALGO",0)*float(plaits?15:46)))+1;
    p.text(juce::String(number).paddedLeft('0',2),x+5,y+5,w*.45f,h*.6f,plain(plaits?C(0xffeea058):C(0xffb8e3ce),std::min(34.f,h*.45f),Weight::Bold),juce::Justification::centred);
    p.text(plaits?"ENGINE":"MODEL",x+5,y+h-17,w*.45f,14,plain(ink,11,Weight::Bold),juce::Justification::centred);
    const float px=x+w*.52f,pw=w*.44f;
    for(int n=0;n<8;++n){const auto level=std::clamp(data.eq_spectrum[unsigned(n)*8],0.f,1.f);p.fill(px+n*pw/8,y+h-8-level*(h-14),pw/8-2,std::max(1.f,level*(h-14)),ink.withAlpha(.6f),1);}
}
void plot(Paint& p,const Snapshot& s,ProPlan plan,const ProStyle& style,float x,float y,float w,float h,bool bypass) {
    const auto& data=s.ui.plugin_panel;const C ink(style.accent);
    switch(plan.plot) {
    case ProPlot::Keys:keyboard(p,s,x,y,w,h,ink,style.instrument==ProInstrument::Grand,bypass);break;
    case ProPlot::Room:room(p,data,x,y,w,h,ink);break;
    case ProPlot::Choir:choir(p,data,x,y,w,h,ink,bypass);break;
    case ProPlot::Sources:waveform(p,data,x,y,w,h,ink,bypass,true);break;
    case ProPlot::Waterfall:waterfall(p,s,x,y,w,h,ink,bypass);break;
    case ProPlot::Operators:
        if(h>75) {operators(p,data,x,y,w,h*.60f,ink,style.instrument);waveform(p,data,x,y+h*.60f+3,w,h*.40f-3,ink,bypass);}
        else operators(p,data,x,y,w,h,ink,style.instrument);
        break;
    case ProPlot::Algorithm: {
        operators(p,data,x,y,w,h,ink,style.instrument);
        const bool offsets=style.instrument==ProInstrument::Dex||style.instrument==ProInstrument::Sloop;
        const float value=control(data,offsets?"Algorithm":"ALGO",0);
        const auto label=offsets&&value<.001f?juce::String("FACTORY"):"ALG "+juce::String(int(std::lround(value*(offsets?32:31)))+(offsets?0:1));
        p.text(label,x+w-60,y+2,55,12,plain(ink,11,Weight::Bold),juce::Justification::centredRight);break;}
    case ProPlot::Wavetable:wavetable(p,data,x,y,w,h,ink,data.row==2||data.row==7?1:0);break;
    case ProPlot::Phase:phase_plot(p,data,x,y,w,h,ink,bypass);break;
    case ProPlot::Filter:spectrum_plot(p,data,x,y,w,h,ink,bypass,"FILTER / OUTPUT SPECTRUM");break;
    case ProPlot::Envelope: {
        const auto prefix=style.instrument==ProInstrument::Loom&&data.row>=2&&data.row<=8?juce::String::charToString(juce::juce_wchar('A'+(data.row-1)/2))+" ":juce::String();
        envelope(p,data,x,y,w,h,ink,prefix.toRawUTF8());break;}
    case ProPlot::Offsets:offsets(p,data,x,y,w,h,ink);break;
    case ProPlot::Lfo:lfo(p,data,x,y,w,h,ink);break;
    case ProPlot::Delay:spatial(p,data,x,y,w,h,ink,false);break;
    case ProPlot::Reverb:spatial(p,data,x,y,w,h,ink,true);break;
    case ProPlot::Mixer:mixer(p,data,x,y,w,h,ink);break;
    case ProPlot::Drums:waveform(p,data,x,y,w,h,ink,bypass);break;
    case ProPlot::Gate:gate(p,data,x,y,w,h,ink);break;
    case ProPlot::Compression:compression(p,data,x,y,w,h,ink,bypass);break;
    case ProPlot::Pitch:pitch(p,s,x,y,w,h,ink,bypass);break;
    case ProPlot::Model:model(p,data,x,y,w,h,ink,style.instrument==ProInstrument::Plaits);break;
    }
}
void presets(Paint& p,const Snapshot& s,const ProStyle& style,float x,float y,float w,float h,bool bypass) {
    const auto& data=s.ui.plugin_panel;const C ink(style.accent);const float list_w=w*.46f;
    panel(p,x,y,list_w,h,C(0xff101517),ink);p.text("PRESETS / DATA",x+5,y+2,list_w-10,12,plain(ink,11,Weight::Bold));
    int selected=-1;
    if(s.presets.entry==data.entry)for(unsigned n=0;n<s.presets.list.size();++n)if(s.presets.list[n].index==s.presets.current)selected=int(n);
    if(s.ui.preset_due&&s.ui.presets_entry==data.entry&&s.ui.presets_cursor<s.presets.list.size())selected=s.ui.presets_cursor;
    const int rows=std::max(1,int((h-19)/17)),first=std::max(0,selected-rows/2);
    for(int n=0;n<rows&&first+n<int(s.presets.list.size());++n) {
        const float yy=y+18+n*17;const bool active=first+n==selected;
        if(active)p.fill(x+3,yy,list_w-6,16,ink.withAlpha(.24f),2);
        const auto name=juce::String(s.presets.list[unsigned(first+n)].name);
        p.text(name,x+6,yy,list_w-12,16,p.fit(name,plain(active?C(0xfff6f1d9):ink,11,Weight::Bold),list_w-12,type::floor));
    }
    plot(p,s,pro_plan(style.instrument,0),style,x+list_w+4,y,w-list_w-4,h,bypass);
}
}

bool draw_plugin_pro_ui(ui::Paint& p,const Snapshot& s,float x,float y,float w,float h,bool listening,bool bypass) {
    if(prism_view(p,s,x,y,w,h,bypass))return true;
    if(bloom_view(p,s,x,y,w,h,bypass))return true;
    using namespace ui;const auto& data=s.ui.plugin_panel;const auto style=pro_style(data.name);
    if(style.instrument==ProInstrument::None||data.status!=PluginStatus::Ready||h<65)return false;
    juce::Graphics::ScopedSaveState saved(p.g);p.g.reduceClipRegion(juce::Rectangle<float>(x,y,w,h).getSmallestIntegerContainer());
    const C ink(style.accent),text=style.dark_text?C(0xff252923):C(0xfff0f0e6);
    chassis(p,x,y+1,w,h-2,style);
    if(style.instrument==ProInstrument::Afterglow) {
        for(int n=4;n>0;--n) {
            p.g.setColour(ink.withAlpha(.04f*float(5-n)));
            p.g.drawRoundedRectangle(x+3+n*.5f,y+3+n*.5f,w-6-n,h-6-n,3.f,float(n));
        }
    }
    const auto name=juce::String(data.name);p.text(name,x+10,y+3,w-112,21,p.fit(name,plain(text,17,Weight::Bold),w-112,type::floor));
    for(unsigned side=0;side<2;++side) {
        const float energy=bypass?0.f:peak(data,side!=0);
        for(unsigned segment=0;segment<6;++segment) {
            const auto tone=segment==5?C(0xffed7964):segment==4?C(0xffe4c46c):C(0xff8ccb9b);
            p.fill(x+w-100+segment*5.2f,y+7+side*5,4,3,tone.withAlpha(energy*6>segment?1.f:.15f),.5f);
        }
    }
    p.text(juce::String(data.row+1)+"/"+juce::String(data.rows),x+w-59,y+5,49,16,plain(style.dark_text?C(0xff4a4f43):ink,11,Weight::Bold),juce::Justification::centredRight);
    const bool browse=s.ui.page==PageId::Stage&&stage_page_now(s.ui,s.track(),false).kind==StageKind::PluginSound;
    if(data.meter_page&&!browse) {draw_analog_output_meter(p,data,x+6,y+28,w-12,h-35,bypass);return true;}
    const auto plan=pro_plan(style.instrument,data.row);
    const float tab_y=y+26;panel(p,x+7,tab_y,w-14,15,style.dark_text?C(0xffb8bbac):C(0xff11191d),ink);
    juce::String caption=browse?"PRESET BROWSER":listening?"LIVE / "+juce::String(plan.title):juce::String(plan.title);
    if(!browse && data.programs) {
        juce::String preset(data.preset_name);
        if(s.ui.preset_due && s.ui.presets_entry==data.entry && s.presets.entry==data.entry && s.ui.presets_cursor<s.presets.list.size())
            preset=s.presets.list[s.ui.presets_cursor].name;
        if(preset.isNotEmpty())caption="PRESET / "+preset;
    }
    const auto caption_style=plain(style.dark_text?C(0xff30362e):ink,11,Weight::Bold);
    p.text(caption,x+12,tab_y+1,w-24,13,p.fit(caption,caption_style,w-24,type::floor));
    const float bx=x+7,by=y+45,bw=w-14,bh=h-51;
    if(browse){presets(p,s,style,bx,by,bw,bh,bypass);return true;}
    if(listening) {plot(p,s,plan,style,bx,by,bw,bh,bypass);return true;}
    // Different hardware layouts per instrument, rather than one recolored card.
    switch(style.instrument) {
    case ProInstrument::Hera:case ProInstrument::Sloop:case ProInstrument::Break:case ProInstrument::Plaits: {
        const float kh=std::min(43.f,bh*.48f),gh=bh-kh-3;
        plot(p,s,plan,style,bx,by,bw,gh,bypass);knob_row(p,data,bx,by+gh+3,bw,kh,style);break;}
    case ProInstrument::Choir: {
        const float cw=bw*.39f;knob_column(p,data,bx,by,cw,bh,style);
        plot(p,s,plan,style,bx+cw+4,by,bw-cw-4,bh,bypass);break;}
    case ProInstrument::Stillwater: {
        const float kw=bw*.36f,gw=bw-kw-4;plot(p,s,plan,style,bx,by,gw,bh,bypass);
        knob_column(p,data,bx+gw+4,by,kw,bh,style);break;}
    case ProInstrument::Dexed: {
        const float kw=bw*.37f;knob_column(p,data,bx,by,kw,bh,style);
        plot(p,s,plan,style,bx+kw+4,by,bw-kw-4,bh,bypass);break;}
    case ProInstrument::Braids: {
        const float gw=bw*.47f;plot(p,s,plan,style,bx,by,gw,bh,bypass);
        knob_column(p,data,bx+gw+4,by,bw-gw-4,bh,style);break;}
    case ProInstrument::Layer: {
        const float kw=bw*.35f,gw=bw-kw-4;
        if(plan.plot==ProPlot::Wavetable&&data.row==0) {
            wavetable(p,data,bx,by,gw,(bh-3)/2,ink,0);wavetable(p,data,bx,by+(bh+3)/2,gw,(bh-3)/2,ink.withRotatedHue(.1f),1);
        }else plot(p,s,plan,style,bx,by,gw,bh,bypass);
        knob_column(p,data,bx+gw+4,by,kw,bh,style);break;}
    case ProInstrument::Loom: {
        const float kw=bw*.37f,gw=bw-kw-4;
        plot(p,s,plan,style,bx,by,gw,bh,bypass);
        knob_column(p,data,bx+gw+4,by,kw,bh,style);break;}
    case ProInstrument::Rack: {
        const float kw=bw*.37f,gw=bw-kw-4;
        if(data.row==0)draw_analog_output_meter(p,data,bx,by,gw,bh,bypass);else plot(p,s,plan,style,bx,by,gw,bh,bypass);
        knob_column(p,data,bx+gw+4,by,kw,bh,style);break;}
    default: {
        const float kw=bw*.38f,gw=bw-kw-4;plot(p,s,plan,style,bx,by,gw,bh,bypass);
        knob_column(p,data,bx+gw+4,by,kw,bh,style);break;}
    }
    return true;
}
}
