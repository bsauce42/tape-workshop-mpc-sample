// SPDX-License-Identifier: AGPL-3.0-only
// Original Tape Workshop graphics; host Melodee by Kerem Kiliç (Ellic Studio).
#pragma once
#include "melodee/pages/common.h"
#include "melodee/pages/plugin_waveform.h"
#include "melodee/pages/analog_meter.h"
#include <string_view>
namespace melodee {
inline bool is_workshop_electric(const PluginPanel& panel) {
    return std::string_view(panel.name)=="Tape Workshop Electric";
}
inline void electric_guitar(ui::Paint& p,float x,float y,float level,float pickup,uint32_t now) {
    using namespace ui;
    juce::Graphics::ScopedSaveState save(p.g);
    p.g.addTransform(juce::AffineTransform::translation(x,y));
    const auto white=juce::Colour(0xfffaf5ff),purple=juce::Colour(0xffbc65ff),cyan=juce::Colour(0xff59e0ce);
    juce::Path body;body.startNewSubPath(26,45);body.cubicTo(16,32,17,23,12,26);
    body.cubicTo(3,35,9,44,5,54);body.cubicTo(-5,75,8,87,25,85);
    body.cubicTo(49,87,52,69,42,56);body.cubicTo(34,47,41,34,36,31);
    body.cubicTo(30,34,36,40,28,45);body.closeSubPath();
    p.g.setGradientFill(juce::ColourGradient(juce::Colour(0xff5e168c),2,67,purple,42,54,false));p.g.fillPath(body);
    p.g.setColour(white.withAlpha(.7f));p.g.strokePath(body,juce::PathStrokeType(.8f));
    p.fill(21,12,7,50,juce::Colour(0xff392619),1);p.fill(22,13,5,49,juce::Colour(0xff5f3f29),1);
    p.fill(21,1,9,15,juce::Colour(0xffc38f55),2);
    for(unsigned n=0;n<6;++n){p.circle(n%2?31:19,4+n*1.8f,1.5f,white);}
    for(unsigned n=0;n<12;++n)p.fill(21,18+n*3,7,.45f,white.withAlpha(.7f));
    p.fill(16,52,19,19,juce::Colour(0xffece4d6),3);
    for(unsigned n=0;n<3;++n)p.fill(20,55+n*5,12,2,pickup<.5f?juce::Colour(0xff15141b):juce::Colour(0xff8f819a),1);
    p.fill(19,74,15,3,white,1);p.circle(38,71,2.8f,juce::Colour(0xff15141b));p.circle(36,77,2.8f,juce::Colour(0xff15141b));
    for(unsigned n=0;n<6;++n){juce::Path string;float sx=23+n*.6f;
        string.startNewSubPath(sx,5);string.lineTo(sx,40);string.quadraticTo(sx+std::sin(float(now%10000)*.02f+n)*level*2,58,sx,76);
        p.g.setColour((level>.002f?cyan:white).withAlpha(.65f));p.g.strokePath(string,juce::PathStrokeType(.35f));}
}
inline void draw_electric_view(ui::Paint& p,const Snapshot& s,float top,bool bypass=false) {
    using namespace ui;
    const auto& panel=s.ui.plugin_panel;
    const auto purple=juce::Colour(0xffba68ff),cyan=juce::Colour(0xff56e1cf),white=juce::Colour(0xfffcf8ff),muted=juce::Colour(0xffb6a6c5);
    constexpr float x=zone::pad,w=zone::width-2*x;
    auto value=[&](unsigned n){return n<panel.visual_count?std::clamp(panel.visual_controls[n].value,0.f,1.f):0.f;};
    auto pick=[&](unsigned n,unsigned max){return std::min(max,unsigned(std::lround(value(n)*max)));};
    float peak=0;if(!bypass)for(auto sample:panel.scope)peak=std::max({peak,std::abs(int16_t(sample)/32768.f),std::abs(int16_t(sample>>16)/32768.f)});
    p.fill(x,top+1,w,140,juce::Colour(0xff08060c),5);p.ring(x,top+1,w,140,purple.withAlpha(.5f),5);
    p.text("TAPE WORKSHOP / ELECTRIC",x+7,top+4,w-45,14,plain(white,11,Weight::Bold));
    p.text(juce::String(panel.row+1)+"/"+juce::String(panel.rows),x+w-49,top+5,42,12,plain(purple,11,Weight::Bold),juce::Justification::centredRight);
    juce::String preset=panel.preset_name[0]?juce::String::fromUTF8(panel.preset_name):"User sound";
    if(s.ui.preset_due&&s.ui.presets_entry==panel.entry&&s.presets.entry==panel.entry&&s.ui.presets_cursor<s.presets.list.size())
        preset=juce::String::fromUTF8(s.presets.list[s.ui.presets_cursor].name.c_str());
    p.text(preset,x+7,top+19,w-57,12,p.fit(preset,plain(cyan,11,Weight::Bold),w-57,type::floor));
    p.text(bypass?"BYPASS":peak>.001f?"LIVE":"READY",x+w-49,top+20,42,11,plain(peak>.001f?cyan:muted,11,Weight::Bold),juce::Justification::centredRight);
    if(panel.meter_page){draw_analog_output_meter(p,panel,x+6,top+33,w-12,103,bypass);return;}
    const unsigned row=panel.row;
    const float gx=x+6,gy=top+34;
    p.fill(gx,gy,66,100,juce::Colour(0xff17101e),3);
    electric_guitar(p,gx+10,gy+2,std::min(1.f,peak*3),value(0),s.now);
    p.text("GUITAR",gx+2,gy+87,62,11,plain(row<6?cyan:muted,11,Weight::Bold),juce::Justification::centred);
    // Amp head and cabinet are original vector illustrations, not third-party product photos.
    const float ax=x+82,ay=top+36,aw=134;
    p.fill(ax,ay,aw,61,juce::Colour(0xff27212d),3);p.ring(ax,ay,aw,61,row>=6&&row<=8?cyan:purple.withAlpha(.5f),3);
    p.fill(ax+5,ay+4,aw-10,16,juce::Colour(0xff08060b),2);
    constexpr const char* amps[]={"CLEAN","JAZZ","BRIT CRUNCH","HIGH GAIN"};
    p.text(amps[pick(18,3)],ax+8,ay+5,aw-16,12,plain(white,11,Weight::Bold));
    for(unsigned n=0;n<3;++n){float cx=ax+12+n*16;float v=value(19+n*2);p.circle(cx,ay+26,4,juce::Colour(0xff0b080e));
        float angle=-2.3f+4.6f*v;p.g.setColour(n?white:purple);p.g.drawLine(cx,ay+26,cx+std::sin(angle)*3,ay+26-std::cos(angle)*3,.8f);}
    p.circle(ax+aw-12,ay+26,2.4f,bypass?muted:cyan.withAlpha(.4f+std::min(1.f,peak)*.6f));
    p.fill(ax+5,ay+33,aw-10,23,juce::Colour(0xff0d0b10),2);
    for(unsigned n=0;n<2;++n){float cx=ax+aw*(n?.72f:.28f);p.circle(cx,ay+44,9+peak*.5f,juce::Colour(0xff423746));p.circle(cx,ay+44,6,juce::Colour(0xff211a27));p.circle(cx,ay+44,2,muted.withAlpha(.7f));}
    for(unsigned n=0;n<12;++n)p.fill(ax+7+n*10,ay+34,.4f,21,white.withAlpha(.09f));
    constexpr const char* cabs[]={"DIRECT / NO CAB","OPEN 1x12","CLOSED 2x12","STACK 4x12"};
    p.text(cabs[pick(24,3)],ax+2,ay+62,aw-4,10,plain(muted,11,Weight::Bold),juce::Justification::centred);
    // Measured stereo waveform remains flat when no audio is present.
    const float wx=x+224,wy=top+36;
    p.fill(wx,wy,72,72,juce::Colour(0xff120c1a),3);
    p.text("OUTPUT",wx+4,wy+2,64,11,plain(white,11,Weight::Bold));
    for(unsigned channel=0;channel<2;++channel){juce::Path trace;const auto& data=channel?panel.scope_right:panel.scope;
        float centre=wy+25+channel*25;
        p.fill(wx+4,centre,64,.5f,muted.withAlpha(.25f));
        for(unsigned n=0;n<64;++n){float sample=bypass?0.f:float(int16_t(data[n]>>16))/32768.f;
            float px=wx+4+n*64.f/63,py=centre-std::clamp(sample,-1.f,1.f)*10;
            if(n)trace.lineTo(px,py);else trace.startNewSubPath(px,py);}
        p.g.setColour(channel?purple:cyan);p.g.strokePath(trace,juce::PathStrokeType(.9f));
    }
    // Three independent, selectable effects in signal-chain order.
    constexpr const char* effects[]={"OFF","COMP","DRIVE","FUZZ","WAH","CHORUS","PHASER","TREM","DELAY","REVERB"};
    constexpr uint32_t colors[]={0xffb36bff,0xffffa34e,0xff5ddbc5,0xffec789d,0xff72b0ff,0xffc178ff,0xffefc55e,0xfff17c60,0xff61d9ed,0xffb887ed};
    for(unsigned n=0;n<3;++n){unsigned effect=pick(27+n*3,9);const auto ink=juce::Colour(colors[effect]);
        const float px=x+81+n*72,py=top+111;bool selected=row==9+n;
        p.fill(px,py,65,25,effect?ink.darker(2.6f):juce::Colour(0xff18131f),3);p.ring(px,py,65,25,selected?white:ink.withAlpha(effect?.6f:.25f),3);
        p.text(juce::String(n+1)+" "+effects[effect],px+3,py+2,59,10,p.fit(juce::String(n+1)+" "+effects[effect],plain(effect?white:muted,11,Weight::Bold),59,type::floor));
        for(unsigned knob=0;knob<2;++knob){float cx=px+12+knob*19;float v=value(28+n*3+knob);p.circle(cx,py+18,4,juce::Colour(0xff08060b));float a=-2.3f+4.6f*v;p.g.setColour(ink);p.g.drawLine(cx,py+18,cx+std::sin(a)*3,py+18-std::cos(a)*3,.8f);}
        p.circle(px+52,py+18,2,effect&&!bypass?cyan:muted.withAlpha(.3f));
        if(n<2){p.g.setColour(purple.withAlpha(.8f));p.g.drawArrow({px+66,py+13,px+71,py+13},.8f,2,2);}
    }

}
}
