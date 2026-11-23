// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Tape Workshop contributors
#pragma once
#include "melodee/pages/plugin_waveform.h"
#include "melodee/pages/analog_meter.h"
#include "melodee/plugin/audio_visual_motion.h"

namespace melodee {
inline void draw_second_spin_vintage(ui::Paint& p, const PluginPanel& panel, float x, float y,
                                    float w, float h, uint32_t now, bool moving, bool bypass, bool waveform = false) {
    using namespace ui;
    if (h < 40 || w < 100) return;
    const auto charcoal = juce::Colour(0xff20201e), cream = juce::Colour(0xffdfcfaa);
    const auto rust = juce::Colour(0xffae5039), teal = juce::Colour(0xff366c68);
    const auto black = juce::Colour(0xff0c0d0c), metal = juce::Colour(0xffa9a598);
    // A compact cassette is approximately 100 x 64 mm, not a wide rack panel.
    // Preserve that silhouette; the output instruments occupy the side bay.
    const float cw = std::min(w * .58f, h * 1.58f), ch = cw / 1.58f;
    const float meter_x = x + cw + 5, meter_w = w - cw - 5;
    const float wave_h = waveform && h >= 66 ? 28.f : 0.f;
    const float meter_h = h - wave_h - (wave_h ? 3 : 0);
    p.g.setGradientFill(juce::ColourGradient(juce::Colour(0xff4b4942),x,y,charcoal,x,y+ch,false));
    p.g.fillRoundedRectangle(x,y,cw,ch,4);p.ring(x,y,cw,ch,metal.withAlpha(.65f),4);
    p.ring(x+2,y+2,cw-4,ch-4,black,3);
    p.g.setGradientFill(juce::ColourGradient(cream.brighter(.08f),x,y,cream.darker(.12f),x,y+ch,false));
    p.g.fillRoundedRectangle(x+6,y+4,cw-12,ch-11,2);
    for(unsigned n=0;n<48;++n) {
        const unsigned grain=(n+3)*2246822519u;
        p.fill(x+8+(grain%997)/997.f*(cw-16),y+5+((grain>>12)%991)/991.f*(ch-18),.6f,.4f,charcoal.withAlpha(.1f));
    }
    p.fill(x + 7, y + ch * .37f, cw - 14, 3, rust);
    p.fill(x + 7, y + ch - 13, cw - 14, 4, teal);
    p.fill(x + 7, y + ch - 13, (cw - 14) * .35f, 4, rust);
    p.text("A", x + 9, y + 3, 12, 11, plain(charcoal, 9, Weight::Bold));
    p.text("TAPE",x+17,y+3,cw-34,12,plain(charcoal,11,Weight::Bold),juce::Justification::centred);
    p.text("WORKSHOP",x+9,y+15,cw-18,12,plain(charcoal,11,Weight::Bold),juce::Justification::centred);
    const float wy = y + ch * .48f, wh = ch * .25f;
    p.fill(x + cw * .20f, wy, cw * .60f, wh, black, wh / 2);
    p.fill(x + cw * .30f, wy + wh * .4f, cw * .40f, wh * .22f, juce::Colour(0xff795936));
    const float radius = std::min(ch * .145f, cw * .075f);
    float recent_peak=0;
    for(unsigned n=panel.scope.size()-8;n<panel.scope.size();++n)
        for(auto pair:{panel.scope[n],panel.scope_right[n]})
            recent_peak=std::max({recent_peak,std::abs(int16_t(pair)/32768.f),std::abs(int16_t(pair>>16)/32768.f)});
    static std::array<PluginAudioMotion,max_plugins> motion;
    const bool tape_path=!panel.tape_deck_ready || (panel.tape_path!=1 && panel.tape_path!=2);
    const float spin=motion[std::min<unsigned>(panel.entry,max_plugins-1)].update(now,panel.scope_stamp,recent_peak,
        !bypass && tape_path,1.f+std::min<unsigned>(panel.tape_speed,2));
    (void)moving; // transport messages alone never animate the cassette
    for (int reel = 0; reel < 2; ++reel) {
        const float cx = x + cw * (reel ? .71f : .29f), cy = wy + wh / 2;
        p.circle(cx, cy, radius, metal); p.circle(cx, cy, radius * .78f, charcoal);
        p.circle(cx, cy, radius * .3f, cream);
        for (int hole = 0; hole < 3; ++hole) {
            const float angle = spin * (reel ? -.8f : -1.f) + hole * 2.0943951f;
            p.circle(cx + radius * .56f * std::cos(angle), cy + radius * .56f * std::sin(angle), radius * .17f, cream);
        }
    }
    juce::Path foot;
    foot.startNewSubPath(x + cw * .32f, y + ch - 9);foot.lineTo(x + cw * .68f, y + ch - 9);
    foot.lineTo(x + cw * .74f, y + ch - 1);foot.lineTo(x + cw * .26f, y + ch - 1);foot.closeSubPath();
    p.g.setColour(metal);p.g.fillPath(foot);
    for (float at : {.34f,.44f,.56f,.66f}) p.circle(x + cw * at, y + ch - 4, 1.2f, black);
    for (float at : {5.f, cw - 5}) {
        p.circle(x + at, y + 5, 1.5f, metal);p.circle(x + at, y + ch - 5, 1.5f, metal);
        p.fill(x + at - 1, y + 5, 2, .6f, black);
    }
    draw_analog_output_meter(p,panel,meter_x,y,meter_w,meter_h,bypass);
    if (wave_h) draw_workshop_waveform(p, panel, meter_x, y + meter_h + 3, meter_w, wave_h, bypass, juce::Colour(0xffe9bf89));
}
} // namespace melodee
