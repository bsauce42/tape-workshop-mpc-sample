// SPDX-License-Identifier: AGPL-3.0-only
// Original graphics. Host: Melodee by Kerem Kiliç (Ellic Studio).
#pragma once
#include "melodee/pages/common.h"
namespace melodee {
inline bool prism_view(ui::Paint& p,const Snapshot& s,float x,float y,float w,float h,bool bypass){
    using namespace ui;const auto& panel=s.ui.plugin_panel;if(!panel.prism||panel.status!=PluginStatus::Ready||h<100)return false;
    juce::Graphics::ScopedSaveState saved(p.g);p.g.reduceClipRegion(juce::Rectangle<float>(x,y,w,h).getSmallestIntegerContainer());
    const auto cyan=juce::Colour(0xff65eadb),purple=juce::Colour(0xffba8aff),white=juce::Colour(0xfff3f4fa),muted=juce::Colour(0xffa5afc5),bg=juce::Colour(0xff0a1020);
    auto v=[&](unsigned n){return n<panel.visual_count?std::clamp(panel.visual_controls[n].value,0.f,1.f):0.f;};
    auto text=[&](juce::String t,float xx,float yy,float ww,juce::Colour c,float size=11){p.text(t,xx,yy,ww,15,p.fit(t,plain(c,size,Weight::Bold),ww,type::floor));};
    const unsigned row=panel.row;const int group=row<8?0:row<32?1:row<38?2:row<50?3:row<62?4:5;
    p.fill(x,y,w,h,bg,5);p.ring(x,y,w,h,purple.withAlpha(.5f),5);
    text("PRISM / FM",x+8,y+3,107,white,13);text(prism_ui::section(row),x+116,y+3,w-169,cyan);
    text(juce::String(row+1)+"/64",x+w-48,y+3,42,muted);
    constexpr const char* tabs[]={"MAIN","OP","FILTER","FM","RM","EDIT"};
    for(int n=0;n<6;++n){float xx=x+7+n*(w-14)/6;p.fill(xx,y+22,(w-20)/6,14,n==group?purple.withAlpha(.3f):white.withAlpha(.035f),2);text(tabs[n],xx+3,y+21,(w-20)/6-4,n==group?white:muted);}
    const float gx=x+8,gy=y+41,gw=w-16,gh=h-61;
    if(group==3||group==4){
        const unsigned base=group==3?102:138,start=(row-(group==3?38:50))*3;const float cell=std::min(10.f,gh/6),mx=gx+24,my=gy+1;
        for(unsigned dest=0;dest<6;++dest)for(unsigned source=0;source<6;++source){unsigned at=dest*6+source;float a=v(base+at);bool selected=at>=start&&at<start+3;float xx=mx+source*cell,yy=my+dest*cell;p.fill(xx,yy,cell-1,cell-1,(group==3?cyan:purple).withAlpha(.08f+.8f*a),1);if(selected)p.ring(xx,yy,cell-1,cell-1,white,1);}
        text("1",gx+10,my-1,12,muted);text("6",gx+10,my+cell*5-2,12,muted);
        float tx=mx+cell*6+12;text(group==3?"FREQUENCY MOD":"RING MOD",tx,gy,gw-(tx-gx),white,12);
        
        for(unsigned k=0;k<3;++k){unsigned at=start+k;auto label="OP "+juce::String(at%6+1)+" > OP "+juce::String(at/6+1)+"    "+juce::String(int(v(base+at)*100))+"%";text(label,tx,gy+22+k*15,gw-(tx-gx),k==0?cyan:white);}
    }else if(row==62){
        text("OP "+juce::String(1+int(std::lround(v(174)*5)))+" / 128 PARTIALS",gx,gy,gw,white,12);
        if(panel.prism_editor_known){for(unsigned n=0;n<128;++n){float xx=gx+n*gw/128,bar=panel.prism_editor.harmonics[n]*std::max(6.f,gh-29);bool selected=n==unsigned(std::lround(v(175)*127));p.fill(xx,gy+gh-13-bar,std::max(.8f,gw/128-0.5f),std::max(.5f,bar),selected?white:cyan.withAlpha(.7f));}text("1                      64                     128",gx,gy+gh-14,gw,muted);}
        else text("Update Prism plug-in for editor data",gx,gy+28,gw,muted);
    }else if(row==63){
        text("OP "+juce::String(1+int(std::lround(v(177)*5)))+" / MOTION CURVE",gx,gy,gw,white,12);
        if(panel.prism_editor_known){juce::Path curve;for(unsigned n=0;n<=140;++n){float pos=n/140.f*7;unsigned a=std::min(6u,unsigned(pos));float t=pos-a;t=t*t*(3-2*t);float value=panel.prism_editor.curve[a]+(panel.prism_editor.curve[a+1]-panel.prism_editor.curve[a])*t;float xx=gx+n*gw/140,yy=gy+19+(1-value)*(gh-23);if(n)curve.lineTo(xx,yy);else curve.startNewSubPath(xx,yy);}p.g.setColour(cyan);p.g.strokePath(curve,juce::PathStrokeType(1.7f));for(unsigned n=0;n<8;++n)p.circle(gx+n*gw/7,gy+19+(1-panel.prism_editor.curve[n])*(gh-23),n==unsigned(std::lround(v(178)*7))?3.f:2.f,purple);}
        else text("Update Prism plug-in for editor data",gx,gy+28,gw,muted);
    }else if(group==1){
        unsigned op=(row-8)/4,b=24+op*10,part=(row-8)%4;
        constexpr const char* pages[]={"TUNING / LEVEL","WAVE / ROUTING","AMP ENVELOPE","RELEASE / MOTION"};
        text("OPERATOR "+juce::String(op+1)+" / "+pages[part],gx,gy,gw,white,12);
        if(part<2){juce::Path wave;unsigned shape=unsigned(std::lround(v(b+3)*5));for(unsigned n=0;n<=120;++n){float t=n/120.f*2,z=t-std::floor(t),a=shape==0?std::sin(t*6.283185f):shape==1?2*z-1:shape==2?(z<.5f?1.f:-1.f):shape==3?1-4*std::abs(z-.5f):0; // Additive/noise use actual output below, not invented partials.
            if(shape>=4){unsigned at=std::min(63u,n*64/121);a=bypass?0:float(int16_t(panel.scope[at]>>16))/32768.f;}else a=std::tanh(a*(1+v(b+4)*7))/(1+v(b+4)*1.7f);
            float xx=gx+t*gw/2,yy=gy+25+(gh-29)*.5f-a*(gh-29)*.42f;if(n)wave.lineTo(xx,yy);else wave.startNewSubPath(xx,yy);}p.g.setColour(cyan);p.g.strokePath(wave,juce::PathStrokeType(1.6f));
        }else{float a=.002f*std::pow(4000.f,v(b+5)),d=.002f*std::pow(4000.f,v(b+6)),r=.002f*std::pow(4000.f,v(b+8));float total=a+d+r+.5f;float xx[]={gx,gx+gw*a/total,gx+gw*(a+d)/total,gx+gw*(a+d+.5f)/total,gx+gw};float yy[]={gy+gh-3,gy+23,gy+23+(gh-26)*(1-v(b+7)),gy+23+(gh-26)*(1-v(b+7)),gy+gh-3};juce::Path env;env.startNewSubPath(xx[0],yy[0]);for(int n=1;n<5;++n)env.lineTo(xx[n],yy[n]);p.g.setColour(cyan);p.g.strokePath(env,juce::PathStrokeType(1.8f));for(int n=1;n<4;++n)p.circle(xx[n],yy[n],2,purple);}
    }else if(group==2){
        unsigned f=(row-32)/2,b=84+f*6,mode=unsigned(std::lround(v(b+2)*2));text("FILTER "+juce::String(f+1)+" / "+(mode==0?"LOW PASS":mode==1?"HIGH PASS":"BAND PASS"),gx,gy,gw,white,12);
        juce::Path path;for(unsigned n=0;n<=100;++n){float u=n/100.f,ratio=std::pow(600.f,u-v(b));float low=1/std::sqrt(1+std::pow(ratio,4.f)),high=1/std::sqrt(1+std::pow(1/ratio,4.f));float response=mode==0?low:mode==1?high:low*high*2;response=std::clamp(response+v(b+1)*.25f*std::exp(-std::pow((u-v(b))*14,2.f)),0.f,1.f);float xx=gx+u*gw,yy=gy+23+(1-response)*(gh-27);if(n)path.lineTo(xx,yy);else path.startNewSubPath(xx,yy);}p.g.setColour(cyan);p.g.strokePath(path,juce::PathStrokeType(1.8f));
    }else{
        text(panel.preset_name[0]?juce::String(panel.preset_name):"CUSTOM SOUND",gx,gy,gw,white,12);
        const float cy=gy+36;for(unsigned op=0;op<6;++op){float xx=gx+op*gw/6;float amount=v(24+op*10+2);p.fill(xx+1,cy-11,gw/6-5,27,purple.withAlpha(.1f+amount*.25f),3);p.ring(xx+1,cy-11,gw/6-5,27,op%2?purple:cyan,3);text("OP "+juce::String(op+1),xx+5,cy-8,gw/6-10,white);p.fill(xx+5,cy+9,(gw/6-13)*amount,2,cyan);}
        juce::Path wave;for(unsigned n=0;n<64;++n){float value=bypass?0:float(int16_t(panel.scope[n]>>16))/32768.f;float xx=gx+n*gw/63,yy=gy+gh-11-std::clamp(value,-1.f,1.f)*10;if(n)wave.lineTo(xx,yy);else wave.startNewSubPath(xx,yy);}p.g.setColour(cyan);p.g.strokePath(wave,juce::PathStrokeType(1.3f));
    }
    text(bypass?"BYPASS":"SHIFT +/- SECTION / DATA PRESET",gx,y+h-16,gw,muted);return true;
}
}
