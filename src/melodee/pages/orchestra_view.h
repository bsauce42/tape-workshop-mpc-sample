// SPDX-License-Identifier: AGPL-3.0-only
// Original Tape Workshop screen. Host based on Melodee by Kerem Kiliç (Ellic Studio).
#pragma once
#include "melodee/pages/melody_view.h"
#include "melodee/pages/strings_view.h"
#include "melodee/plugin/orchestra_info.h"

namespace melodee {
inline bool is_workshop_orchestra(const PluginPanel& panel) {
    return std::string_view(panel.name)=="Tape Workshop Orchestra";
}
inline void draw_orchestra_symbol(ui::Paint& p,const PluginPanel& panel,unsigned family,
                                 float x,float y,float width,float height,float level,float peak,uint32_t now) {
    using namespace ui;
    const auto gold=juce::Colour(0xffedbe79),cyan=juce::Colour(0xff6ce0d5),purple=juce::Colour(0xffc18cff);
    const float cx=x+width*.5f,cy=y+height*.55f;
    p.g.setColour(purple.withAlpha(std::clamp(peak*level*.5f,0.f,.25f)));
    p.g.fillEllipse(x+3,y+3,width-6,height-6);
    if(family==0) {
        const float scale=height/96;
        draw_string_instrument(p,panel,cx-34*scale,y,scale,level,peak,now,false);
    } else if(family==1) {
        juce::Path tube;tube.addEllipse(cx-15,cy-13,27,27);p.g.setColour(gold.darker((1-level)*1.4f));p.g.strokePath(tube,juce::PathStrokeType(4));
        juce::Path inner;inner.addEllipse(cx-10,cy-8,17,17);p.g.setColour(gold.withAlpha(.6f));p.g.strokePath(inner,juce::PathStrokeType(1));
        juce::Path bell;bell.startNewSubPath(cx+7,cy-4);bell.lineTo(cx+23,cy-12);bell.lineTo(cx+23,cy+7);bell.lineTo(cx+7,cy+3);bell.closeSubPath();
        p.g.setGradientFill(juce::ColourGradient(juce::Colour(0xff6b4117),cx+7,cy,gold,cx+23,cy,false));p.g.fillPath(bell);
        p.fill(cx-10,cy-20,23,3,gold,1);for(int n=0;n<3;++n)p.fill(cx-6+n*6,cy-20,2,13,purple,1);
    } else if(family==2) {
        p.g.setColour(cyan.darker((1-level)*1.1f));p.g.drawLine(cx-18,cy+17,cx+14,cy-18,6);
        p.g.setColour(juce::Colour(0xfff0e2d2));p.g.drawLine(cx-19,cy+16,cx+13,cy-19,1);
        for(unsigned n=0;n<6;++n)p.circle(cx-12+n*4.2f,cy+10-n*4.5f,2,juce::Colour(0xff10131c));
        p.circle(cx+13,cy-18,4,purple);p.circle(cx+13,cy-18,1.6f,juce::Colour(0xff17101c));
    } else {
        for(unsigned n=0;n<2;++n) {
            const float xx=cx-21+n*22,yy=cy-5+n*4;
            juce::Path shell;shell.startNewSubPath(xx,yy);shell.quadraticTo(xx+3,yy+23,xx+20,yy);shell.closeSubPath();
            p.g.setGradientFill(juce::ColourGradient(juce::Colour(0xff713917),xx,yy,gold,xx+20,yy,false));p.g.fillPath(shell);
            p.g.setColour(juce::Colour(0xfff4e2ce));p.g.fillEllipse(xx,yy-3,20,6);
            p.g.setColour(purple);p.g.drawLine(xx+4,yy+14,xx+2,yy+23,1);p.g.drawLine(xx+16,yy+14,xx+18,yy+23,1);
        }
        p.g.setColour(gold);p.g.drawLine(cx-17,cy-15,cx+7,cy-4,1.5f);p.circle(cx-18,cy-15,3,purple);
    }
}
inline void draw_orchestra_view(ui::Paint& p,const Snapshot& s,float top,bool bypass=false) {
    if(draw_melody_view(p,s,top))return;
    using namespace ui;
    const auto& panel=s.ui.plugin_panel;
    auto value=[&](unsigned n){return n<panel.visual_count?std::clamp(panel.visual_controls[n].value,0.f,1.f):0.f;};
    constexpr float x=zone::pad,w=zone::width-2*x;
    const auto purple=juce::Colour(0xffc18cff),cyan=juce::Colour(0xff6ce0d5),gold=juce::Colour(0xfff4c581),white=juce::Colour(0xfffaf5ff),dim=juce::Colour(0xffb9a6c9);
    float peak=0;if(!bypass)for(auto v:panel.scope)peak=std::max({peak,std::abs(int16_t(v)/32768.f),std::abs(int16_t(v>>16)/32768.f)});
    p.fill(x,top+1,w,140,juce::Colour(0xff09070e),5);p.ring(x,top+1,w,140,purple.withAlpha(.3f),5);
    p.text("TAPE WORKSHOP / ORCHESTRA",x+7,top+3,w-48,14,plain(white,10,Weight::Bold));
    p.text(juce::String(panel.row+1)+"/"+juce::String(panel.rows),x+w-42,top+3,34,14,plain(purple,9,Weight::Bold),juce::Justification::centredRight);
    p.text(juce::String::fromUTF8(panel.preset_name),x+7,top+19,w-62,13,p.fit(juce::String::fromUTF8(panel.preset_name),plain(gold,10,Weight::Bold),w-62,type::floor));
    p.text(bypass?"BYPASS":peak>.001f?"LIVE":"READY",x+w-54,top+20,46,11,plain(cyan,8,Weight::Bold),juce::Justification::centredRight);
    p.fill(x+7,top+32,w-14,1,purple.withAlpha(.15f));
    const float db=peak>.00001f?20*std::log10(peak):-100;
    p.fill(x+7,top+32,(w-14)*std::clamp((db+60)/60,0.f,1.f),1,cyan);
    const unsigned row=panel.row;
    if(row<2||row==10) {
        constexpr const char* sections[]={"STRINGS","BRASS","WINDS","PERC"};
        for(unsigned family=0;family<4;++family) {
            float left=x+5+family*(w-8)/4,cw=(w-16)/4;
            p.fill(left,top+35,cw,89,juce::Colour(0xff1c1224),3);
            p.text(sections[family],left+2,top+37,cw-4,11,plain(purple,8,Weight::Bold),juce::Justification::centred);
            draw_orchestra_symbol(p,panel,family,left+2,top+49,cw-4,48,value(1+family),peak,s.now);
            const auto name=juce::String(orchestra_short_names[orchestra_player(family,value(30+family))]);
            p.text(name,left+3,top+99,cw-6,18,p.fit(name,plain(white,8,Weight::Bold),cw-6,type::floor),juce::Justification::centred);
            p.fill(left+6,top+119,cw-12,2,purple.withAlpha(.2f),1);p.fill(left+6,top+119,(cw-12)*value(1+family),2,cyan,1);
        }
    } else {
        unsigned focus=0;for(unsigned n=1;n<4;++n)if(value(1+n)>value(1+focus))focus=n;
        if(row==11)focus=3;
        draw_orchestra_symbol(p,panel,focus,x+7,top+38,74,65,value(1+focus),peak,s.now);
        const auto name=juce::String(orchestra_short_names[orchestra_player(focus,value(30+focus))]);
        p.text(name,x+5,top+108,78,15,p.fit(name,plain(gold,9,Weight::Bold),78,type::floor),juce::Justification::centred);
        const float gx=x+91,gy=top+36,gw=w-99;
        p.fill(gx,gy,gw,47,juce::Colour(0xff171020),3);
        const char* title=row==9?"SOLO LEGATO / GLIDE":row==8?"ROOM ECHO / REFLECTIONS":row==3?"AMPLITUDE ENVELOPE":row==4?"FILTER / RESONANCE":row==11?"DYNAMICS / VELOCITY LAYERS":row==5?"EXPRESSION / REVERB":"SOUND / MOVEMENT";
        p.text(title,gx+5,gy+2,gw-10,10,p.fit(title,plain(purple,8,Weight::Bold),gw-10,type::floor));
        juce::Path curve;const float left=gx+7,right=gx+gw-7,floor=gy+41;
        if(row==3){float span=right-left;curve.startNewSubPath(left,floor);curve.lineTo(left+5+value(8)*span*.25f,gy+17);curve.lineTo(left+span*.45f,floor-value(10)*22);curve.lineTo(right-8-value(11)*span*.2f,floor-value(10)*22);curve.lineTo(right,floor);}
        else if(row==9){curve.startNewSubPath(left,floor);curve.lineTo(left+12,floor);curve.lineTo(left+12+value(28)*(gw-32),gy+27);curve.lineTo(right,gy+27);p.text(value(27)>.5f?juce::String(int(std::lround(value(28)*1000)))+" ms / connected":"OFF / POLYPHONIC",left,gy+14,gw-14,10,plain(gold,8,Weight::Bold));}
        else if(row==8){for(unsigned n=0;n<7;++n){float xx=left+n*(8+value(25)*12);if(xx>right)break;float amount=n?value(24)*std::pow(.15f+.7f*value(26),float(n-1)):1;p.fill(xx,floor-amount*24,3,amount*24,n?purple:cyan,1);}}
        else if(row==11){p.text("SOFT",left,gy+17,40,10,plain(dim,8,Weight::Bold));p.text("LOUD",right-40,gy+17,40,10,plain(dim,8,Weight::Bold),juce::Justification::centredRight);p.fill(left,gy+34,right-left,4,purple.withAlpha(.2f),1);p.fill(left,gy+34,(right-left)*value(34),4,cyan,1);}
        else for(unsigned n=0;n<72;++n){float t=n/71.f;float height=row==4?1/(1+std::exp((t-value(12))*20)):.5f+.4f*value(21)*std::sin(t*(1+value(20)*6)*6.283185f);float px=left+t*(right-left),py=floor-height*24;if(!n)curve.startNewSubPath(px,py);else curve.lineTo(px,py);}
        p.g.setColour(cyan);p.g.strokePath(curve,juce::PathStrokeType(1.5f));
        draw_workshop_waveform(p,panel,gx,top+87,gw,38,bypass,purple);
    }
    constexpr const char* tabs[]={"PLAYERS","SOUND","LEGATO","FX"};
    unsigned group=row<2||row>=10?0:row==9?2:row==5||row==8?3:1;
    for(unsigned n=0;n<4;++n){float left=x+5+n*(w-8)/4;p.fill(left,top+129,(w-16)/4,10,juce::Colour(n==group?0xffc18cff:0xff21192d),2);p.text(tabs[n],left,top+129,(w-16)/4,10,plain(n==group?juce::Colour(0xff100b18):dim,8,Weight::Bold),juce::Justification::centred);}
}
}
