// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Tape Workshop contributors
#pragma once
#include "melodee/pages/common.h"
#include "melodee/pages/plugin_waveform.h"
#include "melodee/pages/second_spin_vintage.h"

namespace melodee {
enum class PluginPageKind { Envelope, Filter, Modulation, Pitch, Space, Blend, Timing, Voice };
inline PluginPageKind plugin_page_kind(const PluginPanel& panel) {
    juce::String labels;
    for (const auto& c : panel.params) if (c.used) labels += juce::String(c.name).toLowerCase() + " ";
    const auto has = [&](const char* text) { return labels.contains(text); };
    if (has("attack") || has("decay") || has("sustain") || has("release") || has("env ")) return PluginPageKind::Envelope;
    if (has("cutoff") || has("reson") || has("filter") || has("treble") || has("bass") || has("freq") || has("high pass") || has("low pass")) return PluginPageKind::Filter;
    if (has("lfo") || has("mod ") || has("modulation") || has("drift") || has("vibrato") || has("wobble")) return PluginPageKind::Modulation;
    if (has("pitch") || has("tune") || has("detune") || has("coarse") || has("fine") || has("octave") || has("chord") || has("scale") || has("key ")) return PluginPageKind::Pitch;
    if (has("reverb") || has("room") || has("echo") || has("delay") || has("space") || has("damp")) return PluginPageKind::Space;
    if (has("mix") || has("level") || has("volume") || has("gain") || has("pan") || has("width") || has("balance") || has("input") || has("output")) return PluginPageKind::Blend;
    if (has("time") || has("rate") || has("speed") || has("sync") || has("tempo") || has("swing")) return PluginPageKind::Timing;
    return PluginPageKind::Voice;
}
inline const char* plugin_page_title(PluginPageKind kind) {
    switch (kind) {
    case PluginPageKind::Envelope: return "ENVELOPE";
    case PluginPageKind::Filter: return "FILTER / TONE";
    case PluginPageKind::Modulation: return "MODULATION";
    case PluginPageKind::Pitch: return "PITCH / HARMONY";
    case PluginPageKind::Space: return "SPACE / TIME";
    case PluginPageKind::Blend: return "MIX / OUTPUT";
    case PluginPageKind::Timing: return "TIMING";
    case PluginPageKind::Voice: return "VOICE / CHARACTER";
    }
    return "CONTROLS";
}
inline void parameter_tile(ui::Paint& p, const PluginPanelParam& control, unsigned k,
                           float x, float y, float w, float h, juce::Colour ink, bool dial = false) {
    using namespace ui;
    if (!control.used) return;
    p.fill(x, y, w, h, juce::Colour(0xff101116), 3);
    const auto name = "K" + juce::String(k + 1) + " " + juce::String(control.name);
    p.text(name, x + 4, y + 2, w - 8, 12, p.fit(name, plain(colour::text, 8, Weight::Bold), w - 8, type::floor));
    if (h < 30) {
        p.fill(x,y,w,h,juce::Colour(0xff101116),3);
        p.text(name,x+4,y+2,w*.64f-5,h-6,p.fit(name,plain(colour::text,8,Weight::Bold),w*.64f-5,type::floor));
        p.text(control.text,x+w*.64f,y+2,w*.36f-4,h-6,p.fit(control.text,plain(ink,9,Weight::Bold),w*.36f-4,type::floor),juce::Justification::centredRight);
        const float amount=std::clamp(control.value/float(plugin_param_max),0.f,1.f);
        p.fill(x+4,y+h-3,(w-8)*amount,1.5f,ink);
        return;
    }
    const float amount = std::clamp(control.value / float(plugin_param_max), 0.f, 1.f);
    const auto text = juce::String(control.text);
    const float offset = dial ? std::min(29.f, w * .32f) : 0.f;
    if (dial) {
        const float cx = x + 15, cy = y + h * .64f, r = std::min(10.f, h * .22f);
        juce::Path ring, value;ring.addCentredArc(cx,cy,r,r,0,-2.35f,2.35f,true);
        value.addCentredArc(cx,cy,r,r,0,-2.35f,-2.35f+4.7f*amount,true);
        p.g.setColour(ink.withAlpha(.2f));p.g.strokePath(ring,juce::PathStrokeType(2));
        p.g.setColour(ink);p.g.strokePath(value,juce::PathStrokeType(2));
        const float a=-2.35f+4.7f*amount;p.g.drawLine(cx,cy,cx+std::sin(a)*(r-2),cy-std::cos(a)*(r-2),1.5f);
    } else {
        p.fill(x+4,y+h-4,w-8,2,ink.withAlpha(.18f));p.fill(x+4,y+h-4,(w-8)*amount,2,ink);
    }
    p.text(text, x + 4 + offset, y + 15, w - 8 - offset, h - 19,
           p.fit(text, plain(ink, h > 45 ? 15 : 10, Weight::Bold), w - 8 - offset, type::floor), juce::Justification::centred);
}

// Page layouts follow the loaded parameter names, not invented audio responses.
// Shared widgets are deliberate; layout, focus and signal placement change with
// the job of the current bank. Values always come from the host's current row.
inline bool draw_plugin_parameter_page(ui::Paint& p, const Snapshot& s, float x, float y, float w, float h,
                                      juce::Colour ink, bool bypass = false) {
    using namespace ui;
    const auto& panel = s.ui.plugin_panel;
    if(panel.meter_page) { draw_analog_output_meter(p,panel,x,y,w,h,bypass); return true; }
    if (!panel.row || panel.rows < 2 || h < 45 || keep_workshop_plugin_ui(panel)) return false;
    const auto kind = plugin_page_kind(panel);
    p.fill(x,y,w,h,juce::Colour(0xff090b10),3);
    const auto title = juce::String(plugin_page_title(kind));
    p.text(title,x+5,y+1,w-10,11,plain(ink,8,Weight::Bold));
    y+=15;h-=15;
    const bool reverse = (panel.row & 1u) == 0;
    if (juce::String(panel.name).startsWith("Second Spin")) {
        // Keep the cassette on each bank, with a different control arrangement
        // for blend faders, transport selectors, tone controls and voice choices.
        const bool transport = kind == PluginPageKind::Timing;
        const float deck_w=w*(transport?.52f:.48f),bank_w=w-deck_w-4;
        const float deck_x=transport?x+w-deck_w:x,bank_x=transport?x:x+deck_w+4;
        draw_second_spin_vintage(p,panel,deck_x,y,deck_w,h,s.now,s.playing||panel.tape_vu>.001f,bypass);
        if(kind == PluginPageKind::Blend) {
            const float cw=(bank_w-6)/3;
            for(unsigned k=0;k<3;++k) {
                const auto& control=panel.params[k];if(!control.used)continue;
                const float left=bank_x+k*(cw+3),amount=std::clamp(control.value/float(plugin_param_max),0.f,1.f);
                p.text(control.name,left+2,y,cw-4,12,p.fit(control.name,plain(ink,11,Weight::Bold),cw-4,type::floor));
                p.fill(left+cw/2-1,y+16,2,h-31,ink.withAlpha(.25f));
                p.fill(left+cw*.22f,y+16+(1-amount)*(h-31),cw*.56f,3,ink,1);
                p.text(control.text,left+2,y+h-12,cw-4,12,p.fit(control.text,plain(ink,11,Weight::Bold),cw-4,type::floor),juce::Justification::centred);
            }
        } else if(kind == PluginPageKind::Voice || kind == PluginPageKind::Modulation) {
            const float primary_h=h*.57f, half=(bank_w-3)/2;
            parameter_tile(p,panel.params[0],0,bank_x,y,bank_w,primary_h,ink,true);
            for(unsigned k=1;k<3;++k)parameter_tile(p,panel.params[k],k,bank_x+(k-1)*(half+3),y+primary_h+3,half,h-primary_h-3,ink);
        } else {
            const float lane=(h-4)/3;
            for(unsigned k=0;k<3;++k)parameter_tile(p,panel.params[k],k,bank_x,y+k*(lane+2),bank_w,lane,ink);
        }
        return true;
    }
    if (kind == PluginPageKind::Envelope) {
        // Stage controls laid along a timeline, with the measured output beneath.
        const float cw=(w-8)/3, th=std::max(25.f,h*.52f);
        for(unsigned k=0;k<3;++k) parameter_tile(p,panel.params[k],k,x+k*(cw+4),y,cw,th,ink);
        draw_workshop_waveform(p,panel,x,y+th+3,w,h-th-3,bypass,ink);
    } else if (kind == PluginPageKind::Timing) {
        const float ruler_w=w*.56f,lane=(h-4)/3;
        for(unsigned k=0;k<3;++k) {
            const auto& control=panel.params[k];if(!control.used)continue;
            const float yy=y+k*(lane+2),amount=std::clamp(control.value/float(plugin_param_max),0.f,1.f);
            const auto label=juce::String(control.name)+"  "+juce::String(control.text);
            p.text(label,x+3,yy,ruler_w-6,11,p.fit(label,plain(ink,11,Weight::Bold),ruler_w-6,type::floor));
            p.fill(x+4,yy+lane-4,ruler_w-8,1,ink.withAlpha(.25f));
            for(int tick=0;tick<=8;++tick)p.fill(x+4+tick*(ruler_w-8)/8,yy+lane-6,1,4,ink.withAlpha(.4f));
            p.circle(x+4+amount*(ruler_w-8),yy+lane-4,2,ink);
        }
        draw_workshop_waveform(p,panel,x+ruler_w+4,y,w-ruler_w-4,h,bypass,ink);
    } else if (kind == PluginPageKind::Blend) {
        const float rack=w*.53f,cw=(rack-8)/3;
        for(unsigned k=0;k<3;++k) {
            const auto& c=panel.params[k];if(!c.used)continue;
            const float left=x+k*(cw+4),value=std::clamp(c.value/float(plugin_param_max),0.f,1.f);
            p.fill(left,y,cw,h,juce::Colour(0xff171a20),2);
            const auto label=juce::String(c.name);
            p.text(label,left+2,y+1,cw-4,12,p.fit(label,plain(colour::text,8,Weight::Bold),cw-4,type::floor));
            p.fill(left+cw*.5f-1,y+16,2,std::max(5.f,h-33),ink.withAlpha(.25f));
            p.fill(left+cw*.24f,y+16+(1-value)*std::max(5.f,h-33),cw*.52f,3,ink,1);
            p.text(c.text,left+2,y+h-13,cw-4,12,p.fit(c.text,plain(ink,9,Weight::Bold),cw-4,type::floor),juce::Justification::centred);
        }
        draw_workshop_waveform(p,panel,x+rack+4,y,w-rack-4,h,bypass,ink);
    } else if (kind == PluginPageKind::Filter) {
        const float control_w=w*.42f, plot_w=w-control_w-4;
        const float cx=reverse?x+plot_w+4:x,px=reverse?x:x+control_w+4;
        const float lane=(h-4)/3;
        for(unsigned k=0;k<3;++k) parameter_tile(p,panel.params[k],k,cx,y+k*(lane+2),control_w,lane,ink);
        p.fill(px,y,plot_w,h,juce::Colour(0xff111820),3);
        p.text("OUTPUT SPECTRUM",px+4,y+1,plot_w-8,11,p.fit("OUTPUT SPECTRUM",plain(ink,8,Weight::Bold),plot_w-8,type::floor));
        for(unsigned n=0;n<panel.eq_spectrum.size();++n) {
            const float level=bypass?0.f:std::clamp(panel.eq_spectrum[n],0.f,1.f);
            p.fill(px+4+n*(plot_w-8)/panel.eq_spectrum.size(),y+h-3-level*(h-18),
                   std::max(.7f,(plot_w-8)/panel.eq_spectrum.size()-.3f),std::max(.4f,level*(h-18)),ink.withAlpha(.7f));
        }
    } else if (kind == PluginPageKind::Modulation) {
        const float th=h*.55f,cw=(w-8)/3;
        // Full-width measured stereo strip over a bank of physical-style dials.
        draw_workshop_waveform(p,panel,x,y,w,h-th-3,bypass,ink);
        for(unsigned k=0;k<3;++k) parameter_tile(p,panel.params[k],k,x+k*(cw+4),y+h-th,cw,th,ink,true);
    } else if (kind == PluginPageKind::Space) {
        const float wave_w=w*.55f, lane=(h-4)/3;
        draw_workshop_waveform(p,panel,x,y,wave_w,h,bypass,ink);
        for(unsigned k=0;k<3;++k)parameter_tile(p,panel.params[k],k,x+wave_w+4,y+k*(lane+2),w-wave_w-4,lane,ink);
    } else if (kind == PluginPageKind::Pitch) {
        const float primary=w*.43f;
        parameter_tile(p,panel.params[0],0,x,y,primary,h,ink,true);
        const float rest=w-primary-4, half=(rest-3)/2;
        for(unsigned k=1;k<3;++k) parameter_tile(p,panel.params[k],k,x+primary+4+(k-1)*(half+3),y,half,h*.5f,ink);
        draw_workshop_waveform(p,panel,x+primary+4,y+h*.5f+3,rest,h*.5f-3,bypass,ink);
    } else {
        // Patch/model/operator pages focus on the primary selection, with the
        // other two controls in a side rack rather than another generic scope.
        const unsigned focus=panel.row%3;const float primary=w*.52f, half=(h-3)/2;
        const float left=reverse?x+w-primary:x,right=reverse?x:x+primary+4;
        parameter_tile(p,panel.params[focus],focus,left,y,primary,h,ink,true);
        for(unsigned k=0,at=0;k<3;++k)if(k!=focus)
            parameter_tile(p,panel.params[k],k,right,y+(at++)*(half+3),w-primary-4,half,ink);
    }
    return true;
}
} // namespace melodee
