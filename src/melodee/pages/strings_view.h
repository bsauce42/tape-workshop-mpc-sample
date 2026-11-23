// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop Strings display, hosted by Melodee by Kerem Kiliç (Ellic Studio).
#pragma once
#include "melodee/pages/melody_view.h"
#include "melodee/pages/common.h"
#include "melodee/pages/plugin_waveform.h"
#include <string_view>

namespace melodee {
inline bool is_workshop_strings(const PluginPanel& panel) {
    return std::string_view(panel.name) == "Tape Workshop Strings";
}

inline void draw_string_instrument(ui::Paint& p, const PluginPanel& panel,
                                   float left, float top, float scale, float level,
                                   float peak, uint32_t now, bool roomRings = true, unsigned family = 0) {
    using namespace ui;
    const auto gold = juce::Colour(0xffffc780), white = juce::Colour(0xfffaf5ff);
    const auto purple = juce::Colour(0xffc18cff), cyan = juce::Colour(0xff61e1d2);
    auto value = [&](unsigned n) { return n < panel.visual_count ? std::clamp(panel.visual_controls[n].value,0.f,1.f) : 0.f; };
    const bool active = peak*level>.001f;
    const float motion = active ? std::sin(float(now%60000)*.008f) : 0;
    juce::Graphics::ScopedSaveState saved(p.g);
    p.g.addTransform(juce::AffineTransform::scale(scale).translated(left, top));
        if (roomRings && value(24) > .01f) for (unsigned n = 1; n <= 3; ++n) {
            juce::Path ring;
            const float radius = 25 + n * (5 + value(25) * 6);
            ring.addCentredArc(34, 62, radius, radius * .7f, 0, .7f, 2.4f, true);
            p.g.setColour(purple.withAlpha(value(24) * (active ? .75f : .25f) / n));
            p.g.strokePath(ring, juce::PathStrokeType(1));
        }
        juce::Path body;
        body.startNewSubPath(34, 30);
        body.cubicTo(16, 20, 3, 35, 14, 47);
        body.cubicTo(24, 48, 24, 56, 15, 61);
        body.cubicTo(-1, 76, 13, 94, 34, 94);
        body.cubicTo(55, 94, 69, 76, 53, 61);
        body.cubicTo(44, 56, 44, 48, 54, 47);
        body.cubicTo(65, 35, 52, 20, 34, 30);
        body.closeSubPath();
        if (family == 3) {
            // Sloping double-bass shoulders, rather than a scaled violin silhouette.
            body.clear();body.startNewSubPath(34, 25);
            body.cubicTo(25, 30, 7, 35, 13, 48);body.cubicTo(23, 51, 23, 59, 12, 64);
            body.cubicTo(-2, 82, 14, 95, 34, 95);body.cubicTo(54, 95, 70, 82, 56, 64);
            body.cubicTo(45, 59, 45, 51, 55, 48);body.cubicTo(61, 35, 43, 30, 34, 25);body.closeSubPath();
        }
        p.g.setGradientFill(juce::ColourGradient(juce::Colour(0xff5a210e).darker((1-level)*1.7f), 9, 59,
                                               juce::Colour(0xffefad59).darker((1-level)*1.7f), 53, 68, false));
        p.g.fillPath(body);
        p.g.setColour(juce::Colour(0xff331108));p.g.strokePath(body, juce::PathStrokeType(2.5f));
        p.g.setColour(gold.withAlpha(.65f));p.g.strokePath(body, juce::PathStrokeType(.6f));
        p.fill(29, 4, 10, 43, juce::Colour(0xff241716), 2);
        p.fill(31, 8, 2, 35, juce::Colour(0xff6b4734), 1);
        p.circle(34, 3, 5, juce::Colour(0xff713514));p.circle(34, 3, 2.5f, gold.withAlpha(.6f));
        for (unsigned n = 0; n < 4; ++n) p.fill(n % 2 ? 38 : 24, 9 + n * 5, 6, 2, gold, 1);
        for (unsigned side = 0; side < 2; ++side) {
            juce::Path hole;const float hx = side ? 47 : 21, sign = side ? -1.f : 1.f;
            hole.startNewSubPath(hx + sign * 2, 51);
            hole.cubicTo(hx - sign * 5, 46, hx + sign * 5, 65, hx - sign * 2, 70);
            p.g.setColour(juce::Colour(0xff32180f));p.g.strokePath(hole, juce::PathStrokeType(1.8f));
        }
        p.fill(25, 73, 18, 3, gold, 1);
        juce::Path tail;tail.addTriangle(29, 81, 39, 81, 34, 94);
        p.g.setColour(juce::Colour(0xff21151b));p.g.fillPath(tail);
        for (unsigned n = 0; n < 4; ++n) {
            const float sx = 31 + n * 2;
            const float vibration = active ? std::min(2.f, peak * 6) * std::sin(float(now % 10000) * .04f + n) : 0;
            juce::Path string;string.startNewSubPath(sx, 8);string.quadraticTo(sx + vibration, 58, sx, 85);
            p.g.setColour(white.withAlpha(.65f));p.g.strokePath(string, juce::PathStrokeType(.55f));
        }
        if (value(0) < .5f) {
            const float shift = motion * 7;
            p.g.setColour(juce::Colour(0xff5c3227));p.g.drawLine(6 + shift, 78, 69 + shift, 44, 2.8f);
            p.g.setColour(gold);p.g.drawLine(6 + shift, 80, 69 + shift, 46, .8f);
            p.fill(4 + shift, 77, 7, 4, juce::Colour(0xff242027), 1);
        } else {
            p.text("PIZZ", 2, 49, 25, 11, plain(cyan, 8, Weight::Bold));
        }
}

inline void draw_strings_tabs(ui::Paint& p,float x,float top,float width,unsigned row) {
    using namespace ui;
    constexpr const char* labels[]={"PLAYERS","SOUND","LEGATO","FX"};
    const unsigned group=row<=1?0:row==9?2:(row==5||row==8)?3:1;
    for(unsigned n=0;n<4;++n) {
        const float xx=x+5+n*(width-8)/4;
        p.fill(xx,top+129,(width-16)/4,10,juce::Colour(n==group?0xffc18cff:0xff21192d),2);
        p.text(labels[n],xx,top+129,(width-16)/4,10,plain(juce::Colour(n==group?0xff100b18:0xffc8b8da),8,Weight::Bold),juce::Justification::centred);
    }
}

// All meters/waveforms use measured output. The bow/string motion is an illustrative,
// level-gated animation, not a claim to measure physical bow position or individual strings.
inline void draw_strings_view(ui::Paint& p, const Snapshot& s, float top, bool bypass = false) {
    if(draw_melody_view(p,s,top))return;
    using namespace ui;
    const auto& panel = s.ui.plugin_panel;
    const auto purple = juce::Colour(0xffc18cff), cyan = juce::Colour(0xff61e1d2);
    const auto white = juce::Colour(0xfffaf5ff), muted = juce::Colour(0xffb3a3c2);
    const auto gold = juce::Colour(0xffffc780), dark = juce::Colour(0xff09070e);
    auto value = [&](unsigned n) {
        return n < panel.visual_count ? std::clamp(panel.visual_controls[n].value, 0.f, 1.f) : 0.f;
    };
    float peak = 0;
    if (!bypass) for (auto packed : panel.scope)
        peak = std::max({peak, std::abs(int16_t(packed) / 32768.f), std::abs(int16_t(packed >> 16) / 32768.f)});
    const bool active = peak > .001f;
    constexpr float x = zone::pad, w = zone::width - 2 * x;
    p.fill(x, top + 1, w, 140, dark, 5);
    p.ring(x, top + 1, w, 140, purple.withAlpha(.28f), 5);
    p.text("TAPE WORKSHOP / STRINGS", x + 8, top + 4, 242, 14, plain(white, 11, Weight::Bold));
    p.text(juce::String(panel.row + 1) + "/" + juce::String(panel.rows), x + w - 42, top + 4, 34, 14,
           plain(purple, 10, Weight::Bold), juce::Justification::centredRight);
    const juce::String preset = panel.preset_name[0] ? juce::String::fromUTF8(panel.preset_name) : "User sound";
    p.text(preset, x + 8, top + 19, w - 70, 13, p.fit(preset, plain(gold, 10, Weight::Bold), w - 70, type::floor));
    p.text(bypass ? "BYPASS" : active ? "LIVE" : "READY", x + w - 66, top + 20, 58, 11,
           plain(active ? cyan : muted, 8, Weight::Bold), juce::Justification::centredRight);

    // The Players page shows each family at its own scale on an original dark stage.
    if (panel.row == 0) {
        const float floor = top + 109;
        p.g.setGradientFill(juce::ColourGradient(juce::Colour(0xff382139),x+50,top+37,dark,x+w, floor,false));
        p.g.fillRoundedRectangle(x+4,top+34,w-8,92,3);
        for (unsigned n=0;n<4;++n) {
            const float cx=x+40+n*(w-8)/4;
            const float level=value(1+n), scale=.52f+n*.09f;
            p.g.setColour(purple.withAlpha(.06f+level*.12f));
            p.g.fillEllipse(cx-25,floor-6,50,10);
            draw_string_instrument(p,panel,cx-34*scale,floor-94*scale,scale,level,peak,s.now,false,n);
            if (n>=2) p.fill(cx-.5f,floor-2,1,5,gold.withAlpha(.6f));
            constexpr const char* labels[]={"VIOLIN","VIOLA","CELLO","BASS"};
            p.text(labels[n],cx-30,top+110,60,10,plain(level>.01f?gold:muted,8,Weight::Bold),juce::Justification::centred);
            p.fill(cx-18,top+122,36,2,purple.withAlpha(.15f),1);
            p.fill(cx-18,top+122,36*level,2,level>.01f?cyan:muted,1);
        }
        draw_strings_tabs(p,x,top,w,panel.row);
        return;
    }
    draw_string_instrument(p,panel,x+11,top+35,1.f,1.f,peak,s.now);
    const float gx = x + 99, gy = top + 36, gw = w - 107;
    p.fill(gx, gy, gw, 46, juce::Colour(0xff14101e), 3);
    const unsigned row = panel.row;
    const bool envelope = row == 3;
    const bool filter = row == 4;
    const bool movement = row == 6 || row == 7;
    const bool space = row == 5 || row == 8;
    const char* title = row == 9 ? "SOLO LEGATO / PORTAMENTO" : row == 2 ? "DETUNE / WIDTH / ATTACK" : row == 5 ? "EXPRESSION / ROOM / TONE" : envelope ? "AMPLITUDE ENVELOPE" : filter ? "FILTER RESPONSE" :
                        movement ? "MODULATION / MOTION" : space ? "ROOM / REFLECTIONS" : "STRING SECTION MIX";
    p.text(title, gx + 5, gy + 2, gw - 10, 10, plain(purple, 8, Weight::Bold));
    if (row == 9) {
        const float start=gx+7, end=gx+gw-7, low=gy+40, high=gy+28;
        p.fill(start,low,end-start,.6f,purple.withAlpha(.2f));
        const bool enabled=value(27)>.5f;
        const float glide=enabled?value(28)*(gw-32):0;
        juce::Path line;line.startNewSubPath(start,low);line.lineTo(start+12,low);
        line.lineTo(start+12+glide,high);
        line.lineTo(end,high);p.g.setColour(enabled?cyan:muted);p.g.strokePath(line,juce::PathStrokeType(1.6f));
        p.text(enabled?juce::String(int(std::lround(value(28)*1000)))+" ms / "+juce::String(int(std::lround(value(29)*24)))+" semi":"OFF / POLYPHONIC CHORDS",gx+6,gy+13,gw-12,10,plain(gold,8,Weight::Bold));
    } else if (row == 2) {
        const float centre = gx + gw * .5f;
        p.fill(gx + 9, gy + 29, gw - 18, .7f, purple.withAlpha(.35f));
        for (unsigned n = 0; n < 6; ++n) {
            const float side = (float(n) - 2.5f) / 2.5f;
            const float px = centre + side * (8 + value(7) * (gw * .4f - 8));
            const float py = gy + 29 + std::sin(n * 2.f) * value(6) * 8;
            p.circle(px, py, 2.5f, (n % 2 ? cyan : gold).withAlpha(n > 1 ? .3f + .7f * value(5) : 1.f));
        }
        p.text("L", gx + 4, gy + 30, 11, 12, plain(muted, 8, Weight::Bold));
        p.text("R", gx + gw - 13, gy + 30, 11, 12, plain(muted, 8, Weight::Bold));
    } else if (row == 5) {
        for (unsigned n = 0; n < 6; ++n) {
            const float radius = 8 + n * (3 + value(17) * 5);
            juce::Path arc;arc.addCentredArc(gx + gw * .32f, gy + 39, radius, radius * .65f, 0, -1.55f, 1.55f, true);
            p.g.setColour(purple.withAlpha((.15f + value(17) * .7f) / (1 + n * .3f)));
            p.g.strokePath(arc, juce::PathStrokeType(1));
        }
        p.text("EXPR", gx + gw - 53, gy + 15, 48, 10, plain(muted, 8, Weight::Bold));
        p.fill(gx + gw - 53, gy + 31, 46, 3, cyan.withAlpha(.2f), 1);
        p.fill(gx + gw - 53, gy + 31, 46 * value(16), 3, cyan, 1);
    } else if (envelope) {
        juce::Path curve;float start = gx + 5, end = gx + gw - 5, span = end - start, floor = gy + 40;
        float attack = start + 7 + value(8) * span * .3f, decay = attack + 8 + value(9) * span * .18f;
        float release = end - 8 - value(11) * span * .18f, sustain = floor - value(10) * 24;
        curve.startNewSubPath(start, floor);curve.lineTo(attack, gy + 15);curve.lineTo(decay, sustain);
        curve.lineTo(release, sustain);curve.lineTo(end, floor);
        p.g.setColour(cyan);p.g.strokePath(curve, juce::PathStrokeType(1.5f));
    } else if (filter || movement) {
        juce::Path curve;
        for (unsigned n = 0; n < 80; ++n) {
            float t = n / 79.f, height;
            if (filter) { float knee = value(12);height = 1 / (1 + std::exp((t - knee) * 20));height += value(13) * .35f * std::exp(-std::pow((t - knee) * 18, 2)); }
            else {float rate = row == 4 ? 3 + 5 * value(15) : .05f + 10 * value(20) * value(20);
                float depth = row == 4 ? value(14) : value(21);float phase = t * (1 + rate * .35f);
                float osc = value(22) < .5f ? std::sin(phase * 6.283185f) : 1 - 4 * std::abs((phase - std::floor(phase)) - .5f);
                height = .5f + osc * depth * .42f;}
            const float px = gx + 5 + t * (gw - 10), py = gy + 40 - std::clamp(height, 0.f, 1.2f) * 24;
            if (!n) curve.startNewSubPath(px, py); else curve.lineTo(px, py);
        }
        p.g.setColour(cyan);p.g.strokePath(curve, juce::PathStrokeType(1.5f));
    } else if (space) {
        const unsigned count = 3 + unsigned(value(26) * 5);
        const float spread = 7 + value(25) * 10;
        for (unsigned n = 0; n < count; ++n) {
            const float xx = gx + 8 + n * spread;
            if (xx > gx + gw - 8) break;
            const float amount = n ? value(24) * std::pow(.15f + value(26) * .7f, float(n - 1)) : 1;
            p.fill(xx, gy + 40 - amount * 22, 3, amount * 22, n ? purple : cyan, 1);
        }
        p.text(juce::String(int(std::lround(40 + value(25) * 760))) + " ms", gx + gw - 53, gy + 14, 48, 13,
               plain(gold, 9, Weight::Bold), juce::Justification::centredRight);
    } else {
        constexpr const char* families[] = {"VLN", "VLA", "CEL", "BASS"};
        for (unsigned n = 0; n < 4; ++n) {
            const float xx = gx + 7 + n * ((gw - 12) / 4);
            p.text(families[n], xx, gy + 15, 38, 10, plain(muted, 8, Weight::Bold));
            p.fill(xx, gy + 33, 36, 3, purple.withAlpha(.15f), 1);
            p.fill(xx, gy + 33, 36 * value(1 + n), 3, n % 2 ? cyan : gold, 1);
        }
    }
    draw_workshop_waveform(p, panel, gx, top + 86, gw, 39, bypass, purple);
    draw_strings_tabs(p,x,top,w,panel.row);
}
} // namespace melodee
