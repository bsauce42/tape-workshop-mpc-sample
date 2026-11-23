// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Tape Workshop contributors
#pragma once
#include "melodee/plugin/plugin_panel.h"
#include "melodee/ui/paint.h"

namespace melodee {
inline float plugin_output_peak_db(const PluginPanel& panel, bool bypass = false) {
    float peak=0;
    if (!bypass) for(unsigned n=0;n<panel.scope.size();++n)
        for(auto pair:{panel.scope[n],panel.scope_right[n]})
            peak=std::max({peak,std::abs(int16_t(pair)/32768.f),std::abs(int16_t(pair>>16)/32768.f)});
    return peak>.00001f?20.f*std::log10(peak):-100.f;
}
inline void draw_analog_output_meter(ui::Paint& p,const PluginPanel& panel,float x,float y,float w,float h,bool bypass=false) {
    using namespace ui;
    if(w<60||h<32)return;
    juce::Graphics::ScopedSaveState saved(p.g);
    p.g.reduceClipRegion(juce::Rectangle<float>(x,y,w,h).getSmallestIntegerContainer());
    const auto steel=juce::Colour(0xff373631),black=juce::Colour(0xff0a0b0a),ink=juce::Colour(0xff39382f);
    p.g.setGradientFill(juce::ColourGradient(juce::Colour(0xff4a4840),x,y,black,x,y+h,false));
    p.g.fillRoundedRectangle(x,y,w,h,5);p.ring(x+1,y+1,w-2,h-2,steel,5);
    p.ring(x+3,y+3,w-6,h-6,black,4);
    const float left=x+7,right=x+w-7,top=y+7,bottom=y+h-7;
    juce::Path face;face.startNewSubPath(left,bottom-3);face.lineTo(left,top+h*.20f);
    face.cubicTo(x+w*.22f,top-h*.03f,x+w*.78f,top-h*.03f,right,top+h*.20f);
    face.lineTo(right,bottom-3);face.quadraticTo(x+w*.85f,bottom,x+w*.64f,bottom);
    face.quadraticTo(x+w*.5f,bottom-h*.20f,x+w*.36f,bottom);
    face.quadraticTo(x+w*.15f,bottom,left,bottom-3);face.closeSubPath();
    p.g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe1d7bd),x+w*.5f,top,
                                           juce::Colour(0xffa79b7e),x+w*.5f,bottom,false));p.g.fillPath(face);
    p.g.setColour(black);p.g.strokePath(face,juce::PathStrokeType(2));
    // Fixed fine paper grain: decoration never changes with the audio or clock.
    for(unsigned i=0;i<132;++i) {
        const unsigned hash=(i+17)*2654435761u;
        const float px=left+3+(hash%997)/997.f*(w-20),py=top+5+((hash>>12)%991)/991.f*(h-20);
        if(face.contains(px,py))p.fill(px,py,.65f,.45f,ink.withAlpha(.11f));
    }
    const float cx=x+w*.5f,cy=y+h*.87f,rx=w*.43f,ry=h*.69f;
    juce::Path arc;arc.addCentredArc(cx,cy,rx,ry,0,-1.18f,1.18f,true);
    p.g.setColour(ink.withAlpha(.7f));p.g.strokePath(arc,juce::PathStrokeType(.7f));
    for(int tick=0;tick<=30;++tick) {
        const float angle=-1.18f+2.36f*tick/30;
        const bool major=tick%5==0;
        p.g.setColour(tick>=28?juce::Colour(0xff8e3528):ink.withAlpha(major?.85f:.50f));
        p.g.drawLine(cx+std::sin(angle)*(rx-(major?7:4)),cy-std::cos(angle)*(ry-(major?7:4)),
                     cx+std::sin(angle)*rx,cy-std::cos(angle)*ry,major?1.f:.6f);
        if(major&&w>=240) {
            const auto label=juce::String(-60+tick*2);
            p.text(label,cx+std::sin(angle)*(rx-18)-12,cy-std::cos(angle)*(ry-18)-5,24,11,plain(ink,11,Weight::Bold),juce::Justification::centred);
        }
    }
    const float db=plugin_output_peak_db(panel,bypass);
    const auto level=db>-99?juce::String(db,1)+" dB":juce::String("-inf dB");
    p.text(level,x+w*.22f,y+h*.42f,w*.56f,h*.23f,plain(ink,w>=240?20:14,Weight::Bold),juce::Justification::centred);
    const char* label=bypass?"BYPASSED":w>=240?"OUTPUT PEAK / dBFS":"dBFS";
    p.text(label,x+8,y+h*.65f,w-16,12,plain(ink,11,Weight::Bold),juce::Justification::centred);
    const float angle=-1.18f+2.36f*std::clamp((db+60)/60,0.f,1.f);
    const float tipx=cx+std::sin(angle)*(rx-5),tipy=cy-std::cos(angle)*(ry-5);
    p.g.setColour(black.withAlpha(.22f));p.g.drawLine(cx+1,cy+1,tipx+1,tipy+1,2.5f);
    p.g.setColour(black);p.g.drawLine(cx,cy,tipx,tipy,1.2f);
    p.g.setColour(juce::Colour(0xffbb3529));p.g.drawLine(cx+(tipx-cx)*.75f,cy+(tipy-cy)*.75f,tipx,tipy,1.6f);
    p.circle(cx,cy,5,black);p.circle(cx,cy,3,steel);p.fill(cx-1,cy-.4f,2,.8f,black);
    for(float sx:{x+4,x+w-4}) {
        p.circle(sx,y+h-4,1.4f,steel);p.fill(sx-.8f,y+h-4,1.6f,.5f,black);
    }
    // Thin reflected-glass highlight and a recessed bottom lip.
    p.g.setColour(juce::Colour(0xffffffff).withAlpha(.1f));p.g.strokePath(face,juce::PathStrokeType(.5f));
    p.fill(x+8,y+h-4,w-16,1,black.withAlpha(.7f));
}
}
