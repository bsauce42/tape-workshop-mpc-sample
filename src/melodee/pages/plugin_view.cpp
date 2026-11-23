// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
// A plug-in over the cards (docs/MELODEE.md, Plug-ins), for Edit on a PLUGIN
// track: its name, then while it opens the plug glyph, if it could not the
// broken plug and why in two words, if its 32-bit bridge died STOPPED; once
// open, its parameters as a list, each name on one full line with its
// number: the three on K1-K3 raised and ticked in their colours (their values
// on the cards), the row before (or after) with its values; the wheel or
// Minus / Plus move the rows. The share of each block it takes is the
// header's (pages/edit_page.cpp).
#include "melodee/pages/common.h"
#include "melodee/pages/sampler_common.h"
#include "melodee/plugin/plugin_pages.h"
#include "melodee/plugin/plugin_skin.h"
#include "melodee/shell/screen_view.h"
#include "melodee/shell/stage_map.h"
#include "melodee/ui/knobs.h"
#include "melodee/ui/widgets.h"
#include "melodee/pages/plugin_controls.h"
#include "melodee/pages/plugin_waveform.h"
#include "melodee/shell/stage_map.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include "melodee/pages/layer_house_shapes.h"

namespace melodee {
using namespace ui;

juce::String plugin_name(const Snapshot& s, unsigned entry) {
    if (entry >= max_plugins) return "Plug-in";
    const auto& panel = s.ui.plugin_panel;
    if (panel.entry == entry && panel.name[0]) return juce::String::fromUTF8(panel.name);
    const auto& ref = s.project.plugins[entry];
    if (!ref.used()) return "Plug-in";
    if (ref.name[0]) return juce::String::fromUTF8(ref.name); // as the picker named it
    return juce::String(std::filesystem::path(ref.path).stem().string());
}

namespace {
void draw_output_scope(Paint& p, const PluginPanel& panel, float x, float y, float w, float height, juce::Colour cyan) {
    const float mid = y + height / 2;
    p.fill(x, y, w, height, juce::Colour(0xff152529), 4);
    p.fill(x + 5, mid, w - 10, 1, colour::line);
    for (unsigned i = 0; i < panel.scope.size(); ++i) {
        const uint32_t pair = panel.scope[i];
        const float high = float(pair >> 16) / 32767;
        const float low = float(pair & 0xffff) / 32767 - ((pair & 0x8000) ? 65536.f / 32767 : 0.f);
        const float px = x + 5 + (w - 10) * i / float(panel.scope.size());
        const float above = std::sqrt(std::max(0.f, high)) * (height * .44f), below = std::sqrt(std::max(0.f, -low)) * (height * .44f);
        if (above + below > .2f) p.fill(px, mid - above, 2.6f, above + below, cyan, 1);
    }
}

void keyboard_library_view(Paint& p, const Snapshot& s, float top, bool astral) {
    const auto& panel = s.ui.plugin_panel;
    const bool preset_page = s.ui.page == PageId::Stage &&
        stage_page_now(s.ui, s.track(), false).kind == StageKind::PluginSound;
    constexpr float x = zone::pad, w = zone::width - 2 * x, browser = 104;
    const auto gold = juce::Colour(astral ? 0xff83d7ff : 0xffedbc83);
    const auto ivory = juce::Colour(astral ? 0xffecf5ff : 0xfff5eadb);
    const auto muted = juce::Colour(astral ? 0xffabbce4 : 0xffc3afa3);
    const auto shell = juce::Colour(astral ? 0xff323e76 : 0xff332929);
    const auto dark = juce::Colour(astral ? 0xff181d34 : 0xff201b1c);
    const auto floor = juce::Colour(astral ? 0xff101526 : 0xff151619);
    p.g.setGradientFill(juce::ColourGradient(shell.brighter(.18f), x, top, dark, x, top + 140, false));
    p.g.fillRoundedRectangle(x, top + 2, w, 138, 5);
    p.ring(x, top + 2, w, 138, shell.brighter(.38f), 5);

    const float bx = x + 5, by = top + 7;
    p.fill(bx, by, browser, 128, floor, 4);
    p.text("PRESETS", bx + 7, by + 4, browser - 24, 15, caps_text(gold));
    icon(p, Icon::ChevronRight, bx + browser - 16, by + 7, 9, gold, 1.4f);
    const auto& list = s.presets.list;
    int selected = -1;
    if (s.presets.entry == panel.entry) {
        for (size_t n = 0; n < list.size(); ++n)
            if (list[n].index == s.presets.current) { selected = int(n); break; }
    }
    const int count = int(list.size());
    if (s.ui.preset_due && s.ui.presets_entry == panel.entry && count > 0)
        selected = std::min<int>(s.ui.presets_cursor, count - 1);
    const int first = std::clamp(selected - 2, 0, std::max(0, count - 6));
    for (int i = 0; i < 6 && first + i < count; ++i) {
        const auto& preset = list[size_t(first + i)];
        const bool active = selected == first + i;
        const float y = by + 23 + i * 16;
        if (active) {
            p.fill(bx + 4, y, browser - 8, 15, shell.brighter(.28f), 2);
            p.fill(bx + 4, y + 2, 2, 11, gold, 1);
        }
        auto name = juce::String::fromUTF8(preset.name.c_str());
        const auto label = plain(active ? gold : muted, type::small);
        if (astral && p.width(name, label) > browser - 13) {
            while (name.length() > 1 && p.width(name + "...", label) > browser - 13)
                name = name.dropLastCharacters(1);
            name += "...";
        }
        p.text(name, bx + 8, y, browser - 13, 15,
               p.fit(name, label, browser - 13, type::floor));
    }

    const float rx = bx + browser + 7, rw = x + w - 6 - rx;
    p.text(astral ? "Astral Loom" : "Room Grand", rx + 3, top + 7, rw - 6, 22, plain(ivory, 18, Weight::SemiBold));
    const auto name = s.presets.loading ? juce::String("Loading preset...") :
                      selected >= 0 ? juce::String::fromUTF8(list[size_t(selected)].name.c_str())
                                    : juce::String("Browse presets");
    const auto preset_style = plain(gold, type::small, Weight::Medium);
    p.text(name, rx + 3, top + 29, rw - 6, 14, p.fit(name, preset_style, rw - 6, type::floor));
    const float gx = rx + 1, gy = top + 48, gw = rw - 2, gh = astral ? 63.f : 48.f;
    p.fill(gx, gy, gw, gh, floor, 3);
    p.text("LIVE OUTPUT", gx + 5, gy + 2, gw - 10, 12, caps_text(muted));
    if (astral && selected >= 0)
        p.text(juce::String::fromUTF8(list[size_t(selected)].folder.c_str()).toUpperCase(),
               gx + gw - 78, gy + 2, 72, 12, caps_text(gold), juce::Justification::centredRight);
    const float mid = gy + (astral ? 38.f : 30.f);
    for (int i = 1; i < 8; ++i) p.fill(gx + gw*i/8, gy + 15, 1, gh - 19, shell);
    p.fill(gx + 4, mid, gw - 8, 1, shell);
    juce::Path trace;
    for (unsigned i = 0; i < panel.scope.size(); ++i) {
        const auto pair = panel.scope[i];
        const float lo = int16_t(pair & 0xffffu) / 32768.f;
        const float hi = int16_t(pair >> 16) / 32768.f;
        const auto y_of = [mid, astral](float value) { return mid - (astral ? 19.f : 14.f) * std::copysign(std::sqrt(std::abs(value)), value); };
        const float px = gx + 5 + (gw - 10) * i / (panel.scope.size() - 1);
        if (hi > lo) p.fill(px, y_of(hi), 1, y_of(lo) - y_of(hi), gold.withAlpha(.42f));
        const float py = y_of((lo + hi) * .5f);
        if (i == 0) trace.startNewSubPath(px, py); else trace.lineTo(px, py);
    }
    p.g.setColour(gold);
    p.g.strokePath(trace, juce::PathStrokeType(1.25f));

    if (astral) {
        static constexpr const char* tabs[] = {"MAIN", "A", "B", "C", "D", "ENV", "MOD", "FX", "ARP"};
        static constexpr int group[] = {0,1,1,2,2,3,3,4,4,5,6,7,7,8,8,0,7};
        const int active = group[std::min<size_t>(panel.row, std::size(group)-1)];
        if (preset_page) {
            p.text("TURN TO BROWSE", rx, top + 118, rw, 17, caps_text(gold), juce::Justification::centred);
        } else {
            float x_tab = rx;
            for (int i = 0; i < 9; ++i) {
                const float span = i == 0 ? 32 : i < 5 ? 12 : i == 7 ? 17 : 24;
                p.fill(x_tab, top + 117, span - 1, 17, i == active ? gold.withAlpha(.23f) : floor, 2);
                p.text(tabs[i], x_tab, top + 117, span - 1, 17, caps_text(i == active ? gold : muted),
                       juce::Justification::centred);
                x_tab += span;
            }
        }
        return;
    }

    // Two octaves of piano keys. Held pads highlight their pitch classes.
    unsigned held = 0;
    for (const int key : s.held_keys) if (key >= 0) held |= 1u << (key % 12);
    static constexpr int white[] = {0, 2, 4, 5, 7, 9, 11};
    static constexpr int black[] = {1, 3, -1, 6, 8, 10, -1};
    const float kw = gw / 14, ky = top + 99;
    for (int i = 0; i < 14; ++i)
        p.fill(gx + kw*i, ky, kw - 1, 19, (held & (1u << white[i % 7])) ? gold : ivory, 1);
    for (int i = 0; i < 14; ++i) {
        const int note = black[i % 7];
        if (note >= 0) p.fill(gx + kw*(i + .69f), ky, kw*.54f, 11,
                             (held & (1u << note)) ? gold : floor, 1);
    }
    static constexpr const char* pages[] = {"VOICE", "TOUCH", "SPACE", "MODEL", "AMP"};
    const int row = std::min<int>(panel.row, 4);
    p.text(preset_page ? juce::String("TURN TO BROWSE") : juce::String(pages[row]) + "   " + juce::String(row + 1) + "/5",
           rx + 2, top + 121, rw - 4, 14, caps_text(gold), juce::Justification::centred);
}

void sub_foundry_view(Paint& p, const Snapshot& s, float top) {
    const auto& panel = s.ui.plugin_panel;
    const bool presets = s.ui.page == PageId::Stage &&
        stage_page_now(s.ui, s.track(), false).kind == StageKind::PluginSound;
    constexpr float x = zone::pad, w = zone::width - 2*x;
    const auto gold = juce::Colour(0xfff5ca47);
    const auto dark = juce::Colour(0xff161a1d);
    const auto card = juce::Colour(0xff23282c);
    const auto dim = juce::Colour(0xffaeb8bb);
    static constexpr const char* names[] = {
        "SYNTH","SAMPLE","SAMPLE SHAPE","SAMPLE FILTER","SUB","MIXER","AMP ENV",
        "PITCH / RELEASE","GLIDE","FILTER","FILTER MOD","LFO","LFO ROUTES","DRIVE",
        "TAPE","FOLD / CRUSH","COMPRESSOR","COMP TIMING","SPACE","TONE","OUTPUT"
    };
    const unsigned page = std::min<unsigned>(panel.row,20);
    static constexpr uint32_t inks[] = {
        0xff53e2a3,0xffffa54b,0xffffa54b,0xffffa54b,0xffc694ff,0xff53e2a3,
        0xff86a8ff,0xff86a8ff,0xff86a8ff,0xff64e2d9,0xff64e2d9,0xffff84bb,0xffff84bb
    };
    const auto accent = page < std::size(inks) ? juce::Colour(inks[page]) : gold;
    const auto centre = juce::Justification::centred;
    p.fill(x,top+2,w,138,dark,5);
    p.ring(x,top+2,w,138,card.brighter(.18f),5);
    p.fill(x+5,top+7,w-10,27,card,4);
    p.text("Sub Foundry",x+11,top+8,126,23,plain(gold,17,Weight::Bold));
    const auto& list=s.presets.list;
    int selected=-1;
    if (s.presets.entry==panel.entry) {
        if (s.ui.preset_due && s.ui.presets_entry==panel.entry && s.ui.presets_cursor<list.size())
            selected=s.ui.presets_cursor;
        else for (size_t n=0;n<list.size();++n)
            if (list[n].index==s.presets.current) { selected=int(n); break; }
    }
    const auto preset=selected>=0 ? juce::String::fromUTF8(list[size_t(selected)].name.c_str())
                                  : juce::String("Browse presets");
    p.text(preset,x+142,top+9,w-154,22,plain(colour::text,type::body,Weight::Medium),
           juce::Justification::centredRight);

    const float gx=x+6, gy=top+40, gh=71, gw=presets ? w-12 : 167;
    p.fill(gx,gy,gw,gh,card,3);
    const bool spectrum = !presets && page == 19;
    p.text(spectrum ? "OUTPUT SPECTRUM" : "LIVE OUTPUT",gx+6,gy+2,gw-12,14,caps_text(gold));
    if (presets && selected>=0)
        p.text(juce::String::fromUTF8(list[size_t(selected)].folder.c_str()).toUpperCase(),
               gx+110,gy+2,gw-116,14,caps_text(dim),juce::Justification::centredRight);
    const float left=gx+6, width=gw-12, mid=gy+42;
    for (unsigned i=0;i<=8;++i) p.fill(left+i*width/8,gy+20,1,44,dark);
    p.fill(left,mid,width,1,dark);
    juce::Path trace;
    if (spectrum) {
        for (unsigned i=0;i<panel.eq_spectrum.size();++i) {
            const float px=left+width*i/(panel.eq_spectrum.size()-1);
            const float yy=gy+62-42*panel.eq_spectrum[i];
            if (i==0) trace.startNewSubPath(px,yy); else trace.lineTo(px,yy);
        }
    } else for (unsigned i=0;i<panel.scope.size();++i) {
        const auto pair=panel.scope[i];
        const float low=int16_t(pair&0xffffu)/32768.f, high=int16_t(pair>>16)/32768.f;
        const float px=left+width*i/(panel.scope.size()-1);
        const auto y_of=[mid](float v){return mid-21*std::copysign(std::sqrt(std::abs(v)),v);};
        if (high>low) p.fill(px,y_of(high),1,y_of(low)-y_of(high),accent.withAlpha(.4f));
        const float yy=y_of((low+high)*.5f);
        if (i==0) trace.startNewSubPath(px,yy); else trace.lineTo(px,yy);
    }
    p.g.setColour(accent);
    p.g.strokePath(trace,juce::PathStrokeType(1.4f));

    if (presets) {
        p.text("TURN TO BROWSE PRESETS",x+6,top+117,w-12,17,caps_text(gold),centre);
        return;
    }
    const float px=gx+gw+5, pw=x+w-6-px;
    p.fill(px,gy,pw,gh,card,3);
    p.text(names[page],px+4,gy+2,pw-8,14,caps_text(accent),centre);
    const auto value=[&](unsigned slot) {
        return std::clamp(panel.params[slot].value/float(plugin_param_max),0.f,1.f);
    };
    if (page == 14) {
        const bool running = std::any_of(panel.scope.begin(), panel.scope.end(), [](uint32_t pair) { return pair != 0; });
        const float angle = running ? (s.now % 1000)*.0062831853f : 0;
        for (int j=0;j<2;++j) {
            const float cx=px+pw*(j==0?.30f:.70f), cy=gy+37;
            p.circle(cx,cy,13,dark);
            p.circle(cx,cy,10,accent.withAlpha(.24f));
            for (int spoke=0;spoke<6;++spoke) {
                const float theta=angle+spoke*1.04719755f;
                p.circle(cx+std::cos(theta)*7,cy+std::sin(theta)*7,2,accent);
            }
            p.circle(cx,cy,2.4f,accent);
        }
        p.fill(px+10,gy+58,pw-20,2,dark,1);
        if (value(0)>0) p.fill(px+10,gy+58,(pw-20)*value(0),2,accent,1);
    } else if (page == 0 || page == 4 || page == 6 || page == 7 || page == 8 || page == 9 || page == 11 || page == 13 || page == 15) {
        juce::Path response;
        for (int i=0;i<=48;++i) {
            const float u=i/48.f;
            float v=0;
            switch (page) {
            case 0: {
                const float phase=2*u-std::floor(2*u);
                const int shape=int(std::lround(4*value(0)));
                const float wave=shape==0?std::sin(6.2831853f*phase):shape==1?1-4*std::abs(phase-.5f):
                                 shape==3?(phase<.5f?1.f:-1.f):shape==4?.7f*(2*phase-1)+.3f*std::sin(12.56637f*phase):2*phase-1;
                v=.5f+.42f*wave;
                break;
            }
            case 4: {
                const float phase=6.2831853f*(2*u+.5f*value(2));
                v=.5f+.38f*std::sin(phase)+.12f*value(1)*std::sin(2*phase);
                break;
            }
            case 6: {
                const float peak=.04f+.39f*value(0), edge=peak+.11f+.36f*value(1);
                v=u<peak ? u/peak : u<edge ? 1-(u-peak)*(1-value(2))/(edge-peak) : value(2);
                break;
            }
            case 7: v=value(1)*std::exp(-u/(.05f+.6f*value(2))); break;
            case 8: v=1-std::exp(-u/(.045f+.32f*value(0))); break;
            case 9: {
                const float corner=.13f+.75f*value(1);
                const float low=1/(1+std::pow(u/corner,5.f));
                const int kind=int(std::lround(value(0)*3));
                v=kind==0?.7f:kind==1?low:kind==2?4*low*(1-low):1-low;
                v=std::min(1.f,.88f*v+.24f*value(2)*std::exp(-60*(u-corner)*(u-corner)));
                break;
            }
            case 11: {
                const float t=u*(1+2*value(1)), phase=t-std::floor(t);
                const int shape=int(std::lround(4*value(0)));
                const float wave=shape==0?std::sin(6.2831853f*phase):shape==1?1-4*std::abs(phase-.5f):
                                 shape==2?2*phase-1:shape==3?(phase<.5f?1.f:-1.f):std::sin(17*std::floor(t*7));
                v=.5f+.45f*wave*std::min(1.f,u/(.04f+.8f*value(2)));
                break;
            }
            case 13: v=.5f+.46f*std::tanh((u*2-1)*(1+8*value(1)*value(2))); break;
            default: v=.5f+.42f*std::sin((u*2-1)*(1+9*value(0))); break;
            }
            const float xx=px+7+(pw-14)*u, yy=gy+62-40*std::clamp(v,0.f,1.f);
            if (i==0) response.startNewSubPath(xx,yy); else response.lineTo(xx,yy);
        }
        p.g.setColour(accent);
        p.g.strokePath(response,juce::PathStrokeType(1.5f));
    } else {
        for (unsigned i=0;i<3;++i) {
            const auto name=juce::String::fromUTF8(panel.params[i].name);
            const float yy=gy+19+16*i;
            p.text(name,px+6,yy,77,13,p.fit(name,plain(dim,type::small),77,type::floor));
            static constexpr uint32_t mixer[] = {0xff53e2a3,0xffffa54b,0xffc694ff};
            const auto ink = page == 5 ? juce::Colour(mixer[i]) : accent;
            p.fill(px+86,yy+6,pw-93,2,dark,1);
            if (value(i)>0) p.fill(px+86,yy+6,(pw-93)*value(i),2,ink,1);
        }
    }
    static constexpr const char* groups[]={"SOUND","MOD","FILTER","FX"};
    const unsigned at=page<6?0:page<=8||page==11||page==12?1:page<=10?2:3;
    for (unsigned i=0;i<4;++i) {
        const float tw=(w-12)/4, xx=x+6+i*tw;
        p.fill(xx,top+117,tw-3,17,i==at?accent.withAlpha(.19f):card,3);
        p.text(groups[i],xx,top+117,tw-3,17,caps_text(i==at?accent:dim),centre);
    }
}

void instrument_workshop_view(Paint& p, const Snapshot& s, float top) {
    draw_plugin_workshop_face(p, s, top);
}

void room_grand_view(Paint& p, const Snapshot& s, float top) {
    const auto& panel = s.ui.plugin_panel;
    constexpr float x = zone::pad, w = zone::width - 2 * x;
    const auto brass = juce::Colour(0xffe9c487);
    const auto teal = juce::Colour(0xff80d9cb);
    const auto blue = juce::Colour(0xffa4bcff);
    const auto ink = juce::Colour(0xff151b24);
    p.fill(x, top + 3, w, 134, ink, 5);
    p.text("Room Grand", x + 10, top + 6, w - 20, 24,
           plain(colour::text, type::title, Weight::SemiBold));
    const unsigned page = std::min<unsigned>(panel.row, 2);
    static constexpr const char* pages[] = {"PIANO", "PLAYING", "SPACE"};
    static constexpr const char* names[][3] = {
        {"BRIGHTNESS", "DECAY", "ROOM"},
        {"TOUCH", "HAMMER", "WIDTH"},
        {"SIZE", "DAMPING", "LEVEL"},
    };
    p.text(panel.rows == 1 ? juce::String("OUTPUT") : juce::String(pages[std::min<unsigned>(panel.row, 2)]) + "  " + juce::String(panel.row + 1) + "/3", x + w - 107, top + 11, 96, 15, caps_text(teal),
           juce::Justification::centredRight);
    draw_output_scope(p, panel, x + 9, top + 34, w - 18, 40, teal);

    const juce::Colour accents[] = {brass, teal, blue};
    constexpr float gap = 4, tile = (w - 18 - 2 * gap) / 3;
    for (unsigned k = 0; k < 3; ++k) {
        if (!panel.params[k].used) continue;
        const float left = x + 9 + k * (tile + gap), y = top + 80;
        const float value = std::clamp(panel.params[k].value / float(plugin_param_max), 0.f, 1.f);
        const auto accent = accents[k];
        p.fill(left, y, tile, 49, juce::Colour(0xff222b37), 3);
        p.text(names[page][k], left + 5, y + 3, tile - 10, 13, caps_text(accent));
        juce::Path curve;
        for (int i = 0; i <= 32; ++i) {
            const float t = i / 32.f;
            // These sketches track the controls while the output scope shows the audio.
            const float f = page != 0 ? t * value : k == 0 ? 1 / (1 + std::pow(t / (.12f + .85f*value), 4.f))
                          : k == 1 ? std::exp(-t / (.07f + .65f*value))
                                   : value * std::exp(-3*t) * std::abs(std::cos(34*t));
            const float px = left + 5 + t*(tile - 10), py = y + 38 - f*17;
            if (i == 0) curve.startNewSubPath(px, py); else curve.lineTo(px, py);
        }
        p.g.setColour(accent);
        p.g.strokePath(curve, juce::PathStrokeType(1.4f));
        p.fill(left + 5, y + 43, tile - 10, 2, colour::line, 1);
        if (value > 0) p.fill(left + 5, y + 43, (tile-10)*value, 2, accent, 1);
    }
}

enum class LiveInstrument { Sloop, Dex, Stillwater, Choir };

void live_instrument_view(Paint& p, const Snapshot& s, float top, LiveInstrument instrument) {
    const auto& panel = s.ui.plugin_panel;
    const bool dex = instrument == LiveInstrument::Dex;
    const bool fm = instrument == LiveInstrument::Sloop || dex;
    const bool choir = instrument == LiveInstrument::Choir;
    constexpr float x = zone::pad, w = zone::width - 2 * x;
    const auto accent = juce::Colour(dex ? 0xffc6adff : fm ? 0xffa9e77f : choir ? 0xfff5b68a : 0xff76e4e2);
    const auto base = juce::Colour(dex ? 0xff28223f : fm ? 0xff1b2b22 : choir ? 0xff36252d : 0xff192b3a);
    const auto canvas = base.darker(.85f);
    const auto muted = juce::Colour(0xffb7c2ca);
    const unsigned row = panel.row;
    static constexpr const char* still_pages[] = {"VOICE", "ENVELOPE", "DRIFT", "BELLS", "ECHO", "REVERB", "LAYERS"};
    static constexpr const char* choir_pages[] = {"BLEND", "SECTIONS", "LOW CHOIR", "HIGH CHOIR",
                                                   "MASTER", "PHRASES", "ENSEMBLE", "VOICE"};
    static constexpr const char* fm_pages[] = {"VOICE", "ALGORITHM", "EXPRESSION", "LFO"};
    juce::String page;
    if (fm) {
        if (row < 4) page = fm_pages[row];
        else if (row >= 16)
            page = "OP " + juce::String(1 + (row - 16) / 2) + (row % 2 ? " RESPONSE" : " SHAPE");
        else page = "OP " + juce::String(1 + (row - 4) / 2) + (row % 2 ? " ENVELOPE" : " TUNING");
    } else {
        page = choir ? choir_pages[std::min(row, 7u)] : still_pages[std::min(row, 6u)];
    }
    page += "  " + juce::String(row + 1) + "/" + juce::String(panel.rows);
    p.fill(x, top + 2, w, 138, base, 5);
    p.ring(x, top + 2, w, 138, accent.withAlpha(.37f), 5);
    p.text(dex ? "Dex FM" : fm ? "Sloop FM6" : choir ? "Velvet Choir" : "Stillwater",
           x + 8, top + 5, 148, 21, plain(colour::text, type::title, Weight::SemiBold));
    p.text(page, x + 8, top + 24, 148, 13, caps_text(accent));

    juce::String preset = "Turn Data for presets";
    if (s.ui.preset_due && s.ui.presets_entry == panel.entry && s.ui.presets_cursor < s.presets.list.size()) {
        preset = juce::String::fromUTF8(s.presets.list[s.ui.presets_cursor].name.c_str());
    } else if (s.presets.entry == panel.entry) {
        for (const auto& item : s.presets.list)
            if (item.index == s.presets.current) { preset = juce::String::fromUTF8(item.name.c_str()); break; }
    }
    const float preset_x = x + 161, preset_w = w - 169;
    p.fill(preset_x, top + 6, preset_w, 29, canvas, 3);
    p.text("PRESET", preset_x + 6, top + 7, preset_w - 12, 12, caps_text(accent));
    const auto preset_style = plain(colour::text, type::small, Weight::Medium);
    p.text(preset, preset_x + 6, top + 19, preset_w - 12, 13,
           p.fit(preset, preset_style, preset_w - 12, type::floor));

    const float y = top + 41, gx = x + 7, gw = 150, gh = 53;
    p.fill(gx, y, gw, gh, canvas, 4);
    p.text("LIVE OUTPUT", gx + 5, y + 3, gw - 10, 12, caps_text(muted));
    const float mid = y + 31, graph_w = gw - 25;
    for (int i = 1; i < 5; ++i)
        p.fill(gx + 5 + graph_w * i / 5, y + 16, 1, 30, accent.withAlpha(.12f));
    p.fill(gx + 5, mid, graph_w, 1, accent.withAlpha(.21f));
    float peak = 0;
    for (unsigned i = 0; i < panel.scope.size(); ++i) {
        const uint32_t pair = panel.scope[i];
        const float low = int16_t(pair & 0xffffu) / 32768.f;
        const float high = int16_t(pair >> 16) / 32768.f;
        const float above = 13 * std::sqrt(std::max(0.f, high));
        const float below = 13 * std::sqrt(std::max(0.f, -low));
        if (above + below > .25f)
            p.fill(gx + 5 + graph_w * i / float(panel.scope.size()), mid - above, 1.7f, above + below, accent);
        if (i + 12 >= panel.scope.size()) peak = std::max(peak, std::max(std::abs(low), std::abs(high)));
    }
    for (int i = 0; i < 8; ++i) {
        const auto ink = i >= 7 ? juce::Colour(0xfff9a57f) : accent;
        p.fill(gx + gw - 12, y + 45 - 4 * i, 6, 2.5f,
               std::sqrt(peak) * 8 > i ? ink : ink.withAlpha(.15f), 1);
    }

    const float bx = gx + gw + 5, bw = w - gw - 19;
    p.fill(bx, y, bw, gh, canvas, 4);
    const auto value = [&](unsigned k) {
        return std::clamp(panel.params[k].value / float(plugin_param_max), 0.f, 1.f);
    };
    const bool envelope = fm ? row >= 5 && row < 16 && row % 2 : !choir && row == 1;
    const bool modulation = fm ? row == 3 : !choir && row == 2;
    const bool blend = choir && row == 0;
    if (envelope || modulation) {
        p.text(envelope ? "ENVELOPE SHAPE" : "MODULATION SHAPE", bx + 5, y + 3, bw - 10, 12, caps_text(muted));
        const float left = bx + 8, width = bw - 16, floor = y + 46;
        p.fill(left, floor, width, 1, accent.withAlpha(.2f));
        juce::Path graph;
        if (envelope) {
            const float attack = .08f + .25f * value(0);
            const float sustain_x = fm ? attack + .10f + .22f * value(1) : attack + .24f;
            const float release = fm ? value(2) : value(1);
            graph.startNewSubPath(left, floor);
            graph.quadraticTo(left + width * attack * .43f, floor - 20, left + width * attack, floor - 27);
            graph.quadraticTo(left + width * sustain_x, floor - 18, left + width * sustain_x, floor - 15);
            graph.lineTo(left + width * .74f, floor - 15);
            graph.quadraticTo(left + width * (.76f + .20f * release), floor - 12,
                              left + width * (.79f + .20f * release), floor);
        } else {
            for (int n = 0; n <= 45; ++n) {
                const float t = n / 45.f;
                const float depth = std::max(value(0), fm ? value(2) : 0.f);
                const float py = y + 32 - 13 * depth * std::sin(6.2831853f * t * (1 + 4 * value(1)));
                if (n == 0) graph.startNewSubPath(left, py); else graph.lineTo(left + width * t, py);
            }
        }
        p.g.setColour(accent);
        p.g.strokePath(graph, juce::PathStrokeType(1.5f));
    } else if (blend) {
        const float cross = value(0);
        p.text("LOW / HIGH CHOIR", bx + 5, y + 3, bw - 10, 12, caps_text(muted));
        p.fill(bx + 7, y + 23, bw - 14, 9, base.brighter(.45f), 2);
        const float marker = bx + 7 + (bw - 14) * cross;
        p.fill(marker - 2, y + 21, 4, 13, accent, 1);
        p.text("LOW", bx + 7, y + 35, 38, 13, caps_text(muted));
        p.text("HIGH", bx + bw - 45, y + 35, 38, 13, caps_text(muted), juce::Justification::centredRight);
    } else if (dex && row >= 16 && row % 2 == 0) {
        p.text("ENVELOPE LEVEL OFFSETS", bx + 5, y + 3, bw - 10, 12, caps_text(muted));
        const float center = bx + 29 + (bw - 36) / 2;
        p.fill(center, y + 16, 1, 32, accent.withAlpha(.45f));
        for (unsigned k = 0; k < 3; ++k) {
            const float yy = y + 16 + 11 * k;
            const float width = (value(k) - .5f) * (bw - 36);
            p.text("K" + juce::String(k + 1), bx + 6, yy, 19, 11, caps_text(muted));
            if (std::abs(width) > .2f)
                p.fill(center + std::min(width, 0.f), yy + 4, std::abs(width), 4,
                       knob_shade(s.ui.track, k), 2);
        }
    } else {
        p.text("CONTROL LEVELS", bx + 5, y + 3, bw - 10, 12, caps_text(muted));
        for (unsigned k = 0; k < 3; ++k) {
            const float yy = y + 16 + 11 * k;
            p.text("K" + juce::String(k + 1), bx + 6, yy, 19, 11, caps_text(muted));
            p.fill(bx + 29, yy + 4, bw - 36, 4, base.brighter(.34f), 2);
            if (value(k) > 0)
                p.fill(bx + 29, yy + 4, (bw - 36) * value(k), 4, knob_shade(s.ui.track, k), 2);
        }
    }

    constexpr float tile = (w - 22) / 3;
    for (unsigned k = 0; k < 3; ++k) {
        const auto& param = panel.params[k];
        if (!param.used) continue;
        const float tx = x + 7 + k * (tile + 4), ty = top + 99;
        const auto ink = knob_shade(s.ui.track, k);
        p.fill(tx, ty, tile, 34, canvas, 3);
        const auto label = "K" + juce::String(k + 1) + " " + juce::String::fromUTF8(param.name);
        const auto label_style = plain(ink, type::small, Weight::SemiBold);
        p.text(label, tx + 5, ty + 2, tile - 10, 13, p.fit(label, label_style, tile - 10, type::floor));
        const auto text = juce::String::fromUTF8(param.text);
        const auto style = plain(colour::text, type::body, Weight::SemiBold);
        p.text(text, tx + 5, ty + 14, tile - 10, 15, p.fit(text, style, tile - 10, type::floor));
        if (value(k) > 0) p.fill(tx + 5, ty + 30, (tile - 10) * value(k), 2, ink, 1);
    }
}

void instrument_scope_view(Paint& p, const Snapshot& s, float top, bool stillwater) {
    const auto& panel = s.ui.plugin_panel;
    constexpr float x = zone::pad, w = zone::width - 2 * x;
    const auto gold = juce::Colour(stillwater ? 0xff79e6dc : 0xffffc681);
    const auto aqua = juce::Colour(0xff75e5d4);
    const auto dark = juce::Colour(0xff121922);
    const auto plate = juce::Colour(0xff232f3b);
    const auto muted = juce::Colour(0xffb2c4cf);
    const bool output = !stillwater && panel.row == 1;

    p.g.setGradientFill(juce::ColourGradient(plate, x, top, dark, x, top + 140, false));
    p.g.fillRoundedRectangle(x, top + 2, w, 138, 5);
    p.ring(x, top + 2, w, 138, plate.brighter(.25f), 5);
    p.text(stillwater ? "Stillwater" : "Afterglow Piano", x + 9, top + 8, 145, 23,
           plain(colour::text, 17, Weight::SemiBold));

    const float preset_x = x + 158, preset_w = w - 166;
    p.fill(preset_x, top + 7, preset_w, 29, dark, 4);
    p.text("PRESET", preset_x + 7, top + 8, preset_w - 27, 12, caps_text(gold));
    const bool pending = s.ui.preset_due && s.ui.presets_entry == panel.entry &&
                         s.ui.presets_cursor < s.presets.list.size();
    juce::String selected = s.presets.loading ? "Loading..." : "Browse presets";
    if (s.presets.entry == panel.entry && !s.presets.loading)
        for (const auto& preset : s.presets.list)
            if (preset.index == s.presets.current) { selected = juce::String::fromUTF8(preset.name.c_str()); break; }
    if (pending) selected = juce::String::fromUTF8(s.presets.list[s.ui.presets_cursor].name.c_str());
    const auto name_style = plain(colour::text, type::small, Weight::Medium);
    p.text(selected, preset_x + 7, top + 20, preset_w - 26, 13,
           p.fit(selected, name_style, preset_w - 26, type::floor));
    icon(p, Icon::ChevronRight, preset_x + preset_w - 15, top + 17, 9, gold, 1.4f);

    const float gx = x + 8, gy = top + 43, gh = 68;
    const float gw = output ? 182.f : w - 16;
    const auto grid = juce::Colour(0xff253640);
    const auto canvas = juce::Colour(0xff0d1820);
    p.fill(gx, gy, gw, gh, canvas, 4);
    p.text("LIVE OUTPUT", gx + 7, gy + 3, gw - 14, 13, caps_text(aqua));
    const float px = gx + 7, py = gy + 19, pw = gw - 14, ph = gh - 25;
    const float mid = py + ph / 2;
    for (int n = 0; n <= 8; ++n) p.fill(px + n * pw / 8, py, 1, ph, grid);
    for (int n = 0; n <= 4; ++n) p.fill(px, py + n * ph / 4, pw, 1, grid);
    juce::Path trace;
    for (unsigned i = 0; i < panel.scope.size(); ++i) {
        const uint32_t pair = panel.scope[i];
        const float lo = int16_t(pair & 0xffffu) / 32768.f;
        const float hi = int16_t(pair >> 16) / 32768.f;
        const float sx = px + pw * i / (panel.scope.size() - 1);
        const auto sample_y = [&](float sample) {
            return mid - ph * .47f * std::copysign(std::sqrt(std::abs(sample)), sample);
        };
        const float crest = sample_y(hi), trough = sample_y(lo);
        if (trough - crest > .2f) p.fill(sx, crest, 1.5f, trough - crest, aqua.withAlpha(.40f));
        const float y = sample_y((lo + hi) * .5f);
        if (i == 0) trace.startNewSubPath(sx, y); else trace.lineTo(sx, y);
    }
    p.g.setColour(aqua);
    p.g.strokePath(trace, juce::PathStrokeType(1.35f));

    if (output) {
        const float dx = gx + gw + 5, dw = x + w - 8 - dx;
        p.fill(dx, gy, dw, gh, canvas, 4);
        p.text("DECAY", dx + 7, gy + 3, dw - 14, 13, caps_text(gold));
        const float amount = std::clamp(panel.params[0].value / float(plugin_param_max), 0.f, 1.f);
        juce::Path decay;
        for (int n = 0; n <= 32; ++n) {
            const float t = n / 32.f;
            const float y = py + ph * (1 - .94f * std::exp(-t * 3 / (.21f + .79f * amount)));
            const float sx = dx + 7 + (dw - 14) * t;
            if (n == 0) decay.startNewSubPath(sx, y); else decay.lineTo(sx, y);
        }
        p.g.setColour(gold);
        p.g.strokePath(decay, juce::PathStrokeType(1.5f));
    }

    if (stillwater) {
        p.text("TURN TO BROWSE PRESETS", x + 8, top + 118, w - 16, 15, caps_text(gold),
               juce::Justification::centred);
        return;
    }

    const char* tabs[] = {"SOUND", "OUTPUT"};
    for (unsigned i = 0; i < 2; ++i) {
        const bool active = i == panel.row;
        const float tx = x + 8 + i * (w - 16) / 2;
        const float tw = (w - 22) / 2;
        p.fill(tx, top + 117, tw, 16, active ? plate.brighter(.16f) : dark, 3);
        if (active) p.fill(tx + 4, top + 132, tw - 8, 1, gold);
        p.text(tabs[i], tx, top + 117, tw, 16, caps_text(active ? gold : muted),
               juce::Justification::centred);
    }
}

void ambient_view(Paint& p, const Snapshot& s, float top, bool piano) {
    const auto& panel = s.ui.plugin_panel;
    constexpr float x = zone::pad, w = zone::width - 2 * x;
    const auto cyan = juce::Colour(0xff7de0d4);
    static const char* const pages[] = {"VOICE", "ENVELOPE", "DRIFT", "BELLS", "ECHO", "REVERB", "LAYERS"};
    static const char* const piano_pages[] = {"PIANO", "CHORDS", "SHAPE", "SCALE", "ARPEGGIO", "VOICING", "SONG KEY", "FEEL", "NOTE INPUT"};
    const unsigned row = std::min<unsigned>(panel.row, piano ? 8 : 6);
    const char* page = piano ? piano_pages[row] : pages[row];
    p.text(piano ? "Afterglow Piano" : "Stillwater", x, top + 2, 175, 23, plain(colour::text, type::title, Weight::SemiBold));
    p.text(juce::String(page) + "  " + juce::String(row + 1) + (piano ? "/" + juce::String(panel.rows) : juce::String("/7")),
           x + 175, top + 4, w - 175, 20, caps_text(cyan), juce::Justification::centredRight);

    const float y = top + 29;
    if (piano) {
        p.fill(x, y, w, 105, juce::Colour(0xff202431), 5);
        draw_output_scope(p, panel, x + 5, y + 4, w - 10, 42, juce::Colour(0xffe5c18d));
        if (row == 8) {
            const bool native = panel.params[0].value >= plugin_param_max / 2;
            p.text(native ? "MELODEE CHORDS" : "AFTERGLOW PROGRESSIONS", x + 12, y + 51,
                   w - 24, 21, caps_text(cyan));
            p.text(native ? "Pads follow the Keys layout" : "Set Progression on page 2", x + 12, y + 76,
                   w - 24, 17, plain(colour::text, type::body));
            return;
        }
    } else {
        draw_output_scope(p, panel, x, y, w, 48, cyan);
    }
    const float chart_y = top + 84, tile = (w - 8) / 3;
    for (unsigned k = 0; k < 3; ++k) {
        const float left = x + k * (tile + 4);
        const auto ink = knob_shade(s.ui.track, k);
        const auto& parameter = panel.params[k];
        if (!parameter.used) continue;
        const float value = std::clamp(parameter.value / float(plugin_param_max), 0.f, 1.f);
        p.fill(left, chart_y, tile, 53, colour::panel, 3);
        if (piano) {
            const auto label = juce::String::fromUTF8(parameter.name);
            p.text(label, left + 7, chart_y + 2, tile - 14, 14,
                   p.fit(label, plain(ink, type::small, Weight::SemiBold), tile - 14, type::floor));
        }
        juce::Path curve;
        for (int n = 0; n <= 40; ++n) {
            const float u = n / 40.f;
            float v = value;
            if (piano && row == 2 && k == 0) {
                v = std::exp(-u * 3 / (.10f + .90f * value));
            } else if (piano && row == 2 && k == 1) {
                v = .25f + .18f * std::floor(std::min(3.f, u * (4.5f - 3.5f * value)));
            } else if (piano && row == 0 && k == 2) {
                v = value * std::exp(-u * (1 + 7 * (1 - value)));
            } else if (!piano && row == 1 && k < 2) {
                const float tail = std::exp(-u * 3 / (.10f + .90f * value));
                v = k == 0 ? 1 - tail : tail;
            } else if (!piano && row == 2) {
                v = .5f + .42f * (k == 1 ? 1.f : value) * std::sin(6.2831853f * u * (k == 1 ? 1 + 4 * value : 2));
            } else if (!piano && row >= 3 && row <= 5) {
                v = value * std::exp(-u * (1 + 7 * (1 - value)));
            } else if (!piano && row == 0 && k == 1) {
                v = .5f + .43f * std::sin(6.2831853f * u + value * std::sin(12.566371f * u));
            }
            const float px = left + 7 + (tile - 14) * u;
            const float py = chart_y + (piano ? 18 + (1 - v) * 18 : 6 + (1 - v) * 29);
            if (n == 0) curve.startNewSubPath(px, py); else curve.lineTo(px, py);
        }
        p.g.setColour(ink);
        p.g.strokePath(curve, juce::PathStrokeType(1.4f));
        p.fill(left + 7, chart_y + 43, tile - 14, 2, colour::line, 1);
        if (value > 0) p.fill(left + 7, chart_y + 43, (tile - 14) * value, 2, ink, 1);
    }
}

void layer_house_view(Paint& p, const Snapshot& s, float top) {
    const auto& panel = s.ui.plugin_panel;
    const auto accent = juce::Colour(0xff9aea58);
    const auto muted = juce::Colour(0xffb0babb);
    const bool expanded = panel.rows >= 12;
    const float x = zone::pad + (expanded ? 67 : 0), w = zone::width - zone::pad - x;
    static const char* const tabs[] = {"LAYERS", "A ENV", "B ENV", "ATTACK", "MOTION", "FX", "MORPH", "WARP", "COLOR", "CHORUS", "DELAY", "FILTER", "LAYER C", "C ENVELOPE", "LAYER D", "D ENVELOPE"};
    const unsigned row = std::min<unsigned>(panel.row, 15);
    if (expanded) {
        static constexpr const char* groups[] = {"LAYERS", "ENV", "MOTION", "FX", "TABLE", "COLOR", "FILTER"};
        static constexpr unsigned group_for[] = {0,1,1,1,2,3,4,4,5,3,3,6,0,1,0,1};
        p.fill(zone::pad, top + 4, 62, 133, juce::Colour(0xff171e21), 4);
        for (unsigned i = 0; i < 7; ++i) {
            const float y = top + 8 + i * 18;
            const bool active = i == group_for[row];
            if (active) p.fill(zone::pad + 2, y, 58, 16, accent.withAlpha(.19f), 2);
            p.text(groups[i], zone::pad + 6, y, 54, 16, caps_text(active ? accent : muted));
        }
    }
    p.text("Layer House", x, top + 1, 150, 21, plain(colour::text, type::title, Weight::SemiBold));
    juce::String preset = "TURN DATA FOR PRESETS";
    if (s.ui.preset_due && s.ui.presets_entry == panel.entry && s.ui.presets_cursor < s.presets.list.size()) {
        preset = juce::String::fromUTF8(s.presets.list[s.ui.presets_cursor].name.c_str());
    } else if (s.presets.entry == panel.entry) {
        for (const auto& item : s.presets.list)
            if (item.index == s.presets.current) { preset = juce::String::fromUTF8(item.name.c_str()); break; }
    }
    const auto preset_style = plain(muted, type::small, Weight::Medium);
    p.text(preset, x, top + 21, 145, 12, p.fit(preset, preset_style, 145, type::floor));
    const auto page = juce::String(tabs[row]) + "  " + juce::String(row + 1) + "/" + juce::String(panel.rows);
    p.text(page, x + 150, top + 27, w - 150, 16, caps_text(accent), juce::Justification::centredRight);
    for (unsigned i = 0; i < panel.rows; ++i)
        p.fill(x + i * (w / panel.rows), top + 34, w / panel.rows - 4, 3, i == row ? accent : colour::line, 1);
    const auto cards = cards_of(s);
    for (unsigned k = 0; k < 3; ++k) {
        if (cards[k].state != KnobCard::State::Pickup) continue;
        const auto hint = juce::String("MATCH K") + juce::String(k + 1);
        p.text(hint, x + 170, top + 2, w - 170, 20, caps_text(muted), juce::Justification::centredRight);
        break;
    }
    const float y = top + 51;
    p.fill(x, y, w, 88, juce::Colour(0xff171e21), 4);
    const auto value = [&](unsigned knob) { return juce::String::fromUTF8(panel.params[knob].text); };
    const auto fraction = [&](unsigned knob) {
        return std::clamp(panel.params[knob].value / float(plugin_param_max), 0.f, 1.f);
    };
    if (row == 0) {
        const float blend = fraction(2);
        for (unsigned k = 0; k < 2; ++k) {
            const float left = x + k * (w / 2) + 3;
            const float width = w / 2 - 6;
            p.fill(left, y + 3, width, 81, juce::Colour(0xff252d30), 3);
            p.text(k ? "LAYER B" : "LAYER A", left + 6, y + 5, width * .57f - 6, 13, caps_text(accent));
            const int amount = int(std::lround((k ? blend : 1 - blend) * 100));
            p.text(juce::String(amount) + "%", left + width * .57f, y + 5, width * .43f - 6, 13,
                   caps_text(muted), juce::Justification::centredRight);
            const auto style = plain(colour::text, type::body, Weight::SemiBold);
            p.text(value(k), left + 6, y + 19, width - 12, 17, p.fit(value(k), style, width - 12, type::floor));
            p.fill(left + 6, y + 39, width - 12, 39, juce::Colour(0xff101819), 2);
            for (int line = 1; line < 8; ++line)
                p.fill(left + 6 + line * (width - 12) / 8, y + 40, 1, 36, juce::Colour(0xff24302a));
            p.fill(left + 6, y + 58, width - 12, 1, juce::Colour(0xff344335));
            // Match the layer gains so moving Blend changes both traces.
            const float gain = (k ? blend : 1 - blend) * .19f;
            const int count = panel.rows >= 9 ? 16 : 8;
            const int shape = std::clamp(int(std::lround(fraction(k) * (count - 1))), 0, count - 1);
            static constexpr unsigned partners[] = {1,5,4,11,8,12,9,10,15,14,8,12,5,11,9,8};
            const auto& waveform = layer_house_shapes[shape];
            const auto& alternate = layer_house_shapes[partners[shape]];
            const float morph = panel.layer_morph[k], pivot = .5f - .33984375f * panel.layer_warp[k];
            juce::Path wave;
            for (unsigned n = 0; n < 64; ++n) {
                const float phase = n / 8.f;
                const float position = phase - std::floor(phase);
                const float read = std::floor(phase) + (position < pivot ? .5f * position / pivot
                    : .5f + .5f * (position - pivot) / (1 - pivot));
                const float point = read * 8;
                const unsigned i = unsigned(point) % 64, next = (i + 1) % 64;
                const float f = point - std::floor(point);
                const float base = waveform[i] + f * (waveform[next] - waveform[i]);
                const float alt = alternate[i] + f * (alternate[next] - alternate[i]);
                const float px = left + 7 + n * (width - 14) / 63;
                const float py = y + 58 - (base + morph * (alt - base)) * gain;
                if (n == 0) wave.startNewSubPath(px, py); else wave.lineTo(px, py);
            }
            p.g.setColour(accent.withAlpha(.20f));
            p.g.strokePath(wave, juce::PathStrokeType(4));
            p.g.setColour(accent);
            p.g.strokePath(wave, juce::PathStrokeType(1.3f));
        }
        return;
    }
    if (row == 12 || row == 14) {
        const unsigned shape = std::clamp(int(std::lround(15 * fraction(0))), 0, 15);
        const auto& waveform = layer_house_shapes[shape];
        p.text(value(0), x + 8, y + 5, w - 16, 19, plain(accent, type::body, Weight::SemiBold));
        juce::Path wave;
        for (unsigned i = 0; i <= 64; ++i) {
            const float px = x + 8 + i * (w - 16) / 64;
            const float py = y + 54 - waveform[i % 64] * .32f * fraction(1);
            if (i == 0) wave.startNewSubPath(px, py); else wave.lineTo(px, py);
        }
        p.g.setColour(accent);
        p.g.strokePath(wave, juce::PathStrokeType(1.5f));
        return;
    }
    if (expanded && (row == 6 || row == 7)) {
        static constexpr unsigned partners[] = {1,5,4,11,8,12,9,10,15,14,8,12,5,11,9,8};
        for (unsigned layer = 0; layer < 2; ++layer) {
            const float left = x + 6 + layer * w / 2, width = w / 2 - 14;
            p.text(layer ? "OSC B" : "OSC A", left, y + 3, width, 14, caps_text(accent));
            const unsigned shape = panel.layer_shape[layer];
            const auto& base = layer_house_shapes[shape];
            const auto& alt = layer_house_shapes[partners[shape]];
            for (int slice = 4; slice >= 0; --slice) {
                const float morph = slice == 0 ? panel.layer_morph[layer]
                    : std::clamp(panel.layer_morph[layer] + slice * .17f, 0.f, 1.f);
                const float pivot = .5f - .33984375f * panel.layer_warp[layer];
                juce::Path wave;
                for (int n = 0; n <= 64; ++n) {
                    const float t = n / 64.f;
                    const float skew = t < pivot ? .5f*t/pivot : .5f + .5f*(t-pivot)/(1-pivot);
                    const float point = skew * 8;
                    const unsigned i = unsigned(point);
                    const float f = point - i;
                    const float a = base[i] + f * (base[i+1] - base[i]);
                    const float b = alt[i] + f * (alt[i+1] - alt[i]);
                    const float px = left + slice * 2 + t * (width - 8);
                    const float py = y + 67 - slice * 8 - (a + morph*(b-a)) * .16f;
                    if (n == 0) wave.startNewSubPath(px, py); else wave.lineTo(px, py);
                }
                p.g.setColour(accent.withAlpha(slice == 0 ? 1.f : .21f + .08f * (4-slice)));
                p.g.strokePath(wave, juce::PathStrokeType(slice == 0 ? 1.5f : 1.f));
            }
        }
        return;
    }
    if (row == 1 || row == 2 || row == 13 || row == 15) {
        // The engine uses one-pole attack and release. Show their relative
        // lengths with a held section between them.
        const float tone = fraction(0), a = fraction(1), r = fraction(2);
        const float left = x + 15, base = y + 58, peak = y + 13;
        // Widen the short settings for visibility on the 320 px display.
        const float rise = 12 + .32f * w * a, fall = 12 + .32f * w * r;
        const float end = x + w - 15;
        juce::Path curve;
        curve.startNewSubPath(left, base);
        curve.cubicTo(left + rise * .24f, peak, left + rise * .62f, peak, left + rise, peak);
        curve.lineTo(end - fall, peak);
        curve.cubicTo(end - fall * .72f, base, end - fall * .35f, base, end, base);
        auto shade = curve;
        shade.closeSubPath();
        p.g.setColour(accent.withAlpha(.14f));
        p.g.fillPath(shade);
        p.g.setColour(accent);
        p.g.strokePath(curve, juce::PathStrokeType(2));
        p.circle(left + rise, peak, 3, accent);
        p.circle(end - fall, peak, 3, accent);
        // K1 shapes the low-pass filter, shown as its own response trace.
        const float filter_x = x + w * .5f - 29, filter_y = y + 38;
        p.fill(filter_x, filter_y, 58, 22, juce::Colour(0xff101819), 2);
        juce::Path response;
        const float coefficient = .027f + .76f * tone;
        for (int i = 0; i <= 48; ++i) {
            const float hz = 70.f * std::pow(280.f, i / 48.f);
            const float z = 1.f - coefficient;
            const float gain = coefficient / std::sqrt(1.f + z * z - 2.f * z * std::cos(6.2831853f * hz / 44100.f));
            const float py = filter_y + 4 + 14 * (1 - gain);
            if (i == 0) response.startNewSubPath(filter_x + 5, py);
            else response.lineTo(filter_x + 5 + i, py);
        }
        p.g.setColour(knob_shade(s.ui.track, 0));
        p.g.strokePath(response, juce::PathStrokeType(1.25f));
        const int attack_ms = int(std::lround(1000.f * (42 + 3900 * a * a) / 44100));
        const int release_ms = int(std::lround(1000.f * (145 + 8200 * r * r) / 44100));
        p.text("ATTACK " + juce::String(attack_ms) + " ms", x + 12, y + 65, w/2 - 12, 17, caps_text(muted));
        p.text("RELEASE " + juce::String(release_ms) + " ms", x + w/2, y + 65, w/2 - 12, 17, caps_text(muted), juce::Justification::centredRight);
        return;
    }
    for (unsigned k = 0; k < 3; ++k) {
        const float left = x + k * (w / 3);
        const float width = w / 3;
        const auto label = juce::String::fromUTF8(panel.params[k].name);
        const auto ink = knob_shade(s.ui.track, k);
        p.text(label, left + 4, y + 8, width - 8, 16, p.fit(label, plain(ink, type::small), width - 8, type::floor), juce::Justification::centred);
        const auto style = plain(colour::text, type::list, Weight::SemiBold);
        p.text(value(k), left + 4, y + 32, width - 8, 22, p.fit(value(k), style, width - 8, type::floor), juce::Justification::centred);
        p.fill(left + 12, y + 65, width - 24, 4, colour::line, 2);
        p.fill(left + 12, y + 65, (width - 24) * fraction(k), 4, ink, 2);
    }
}
} // namespace

namespace {

void instrument_control_plot(Paint& p, const KnobCard& card, bool plugin_param, unsigned knob,
                       float x, float y, float w) {
    const auto ink = card.shade;
    p.fill(x, y, w, 56, colour::bg, 3);
    if (card.state == KnobCard::State::Idle) return;
    if (card.state == KnobCard::State::Turning) p.ring(x, y, w, 56, ink, 3);
    const auto name = card.label_mixed.isNotEmpty() ? card.label_mixed : card.label;
    const auto label = "K" + juce::String(knob + 1) + "  " + name;
    const auto label_style = plain(ink, type::small, Weight::Medium);
    p.text(label, x + 5, y, w - 10, 15, p.fit(label, label_style, w - 10, type::floor));

    const float fraction = std::clamp(card.fill, 0.f, 1.f);
    const float left = x + 6, span = w - 12;
    const float top = y + 17, bottom = y + 39, height = bottom - top;
    p.fill(left, bottom, span, 1, ink.withAlpha(.22f));
    juce::Path plot;
    auto point = [&](float t, float level, bool first = false) {
        const float px = left + t * span, py = bottom - level * height;
        if (first) plot.startNewSubPath(px, py);
        else plot.lineTo(px, py);
    };

    // The curves illustrate the knob's role. The live panes show measured audio.
    if (plugin_param && (name.equalsIgnoreCase("Attack") || name.equalsIgnoreCase("Decay") || name.equalsIgnoreCase("Release"))) {
        const bool attack = name.equalsIgnoreCase("Attack");
        const float end = .12f + .85f * fraction;
        for (int i = 0; i <= 24; ++i) {
            const float t = i / 24.f;
            const float progress = std::min(1.f, t / end);
            const float level = attack ? progress : std::pow(1.f - progress, 2.f);
            point(t, .08f + .84f * level, i == 0);
        }
    } else if (plugin_param && (name.containsIgnoreCase("Cutoff") || name.equalsIgnoreCase("Freq"))) {
        const float corner = .07f + .86f * fraction;
        for (int i = 0; i <= 24; ++i) {
            const float t = i / 24.f;
            const float bend = std::max(0.f, t - corner);
            point(t, .1f + .8f / (1.f + 150.f * bend * bend), i == 0);
        }
        p.fill(left + span * corner, top, 1, height, ink.withAlpha(.32f));
    } else if (plugin_param && name.containsIgnoreCase("Reso")) {
        for (int i = 0; i <= 24; ++i) {
            const float t = i / 24.f;
            const float distance = (t - .55f) * 8.f;
            point(t, .19f + .74f * fraction / (1.f + distance * distance), i == 0);
        }
    } else if (plugin_param && (name.equalsIgnoreCase("Rate") || name.equalsIgnoreCase("Spd"))) {
        for (int i = 0; i <= 32; ++i) {
            const float t = i / 32.f;
            point(t, .5f + .36f * std::sin(t * (1.f + 4.f * fraction) * juce::MathConstants<float>::twoPi), i == 0);
        }
    } else if (plugin_param && name.equalsIgnoreCase("Sustain")) {
        point(0, fraction, true);
        point(1, fraction);
    } else {
        // Model, preset, mix, and other controls use a position meter.
        p.fill(left, top + 9, span, 4, ink.withAlpha(.22f), 2);
        p.fill(left, top + 9, span * fraction, 4, ink, 2);
        p.circle(left + span * fraction, top + 11, 2.5f, ink);
    }
    if (!plot.isEmpty()) {
        p.g.setColour(ink);
        p.g.strokePath(plot, juce::PathStrokeType(1.3f));
    }
    const auto value = card.value + card.unit;
    const auto value_style = plain(colour::text, type::small);
    p.text(value, left, y + 41, span, 14, p.fit(value, value_style, span, type::floor));
}

void live_plugin_view(Paint& p, const Snapshot& s, float top) {
    const auto& panel = s.ui.plugin_panel;
    static constexpr uint32_t colours[] {
        0xfff8a56d, 0xff97e578, 0xffbcadff, 0xffed90bd, 0xff75e6ed, 0xff83b5ff, 0xffffc85f
    };
    const auto ink = juce::Colour(colours[std::clamp<unsigned>(panel.instrument_visual, 1, std::size(colours)) - 1]);
    constexpr float x = zone::pad, w = zone::width - 2 * x, gap = 4;
    constexpr float gx = x + 5, gw = w - 10;
    p.fill(x, top + 2, w, 140, colour::panel, 4);
    p.ring(x, top + 2, w, 140, ink.withAlpha(.5f), 4);
    const auto name = juce::String::fromUTF8(panel.name);
    const auto title = plain(colour::text, type::title, Weight::SemiBold);
    p.text(name, gx + 2, top + 3, 210, 24, p.fit(name, title, 210, type::floor));
    const auto bindings = page_bindings(s.project, s.ui, s.pattern);
    const bool editing = std::any_of(bindings.begin(), bindings.end(), [](const Binding& b) {
        return b.kind == Binding::Kind::PluginParam;
    });
    if (editing && panel.rows)
        p.text(juce::String(panel.row + 1) + "/" + juce::String(panel.rows),
               gx + gw - 65, top + 5, 62, 18, caps_text(ink), juce::Justification::centredRight);

    if (panel.preset_name[0]) {
        const auto preset = juce::String::fromUTF8(panel.preset_name);
        const auto style = plain(colour::sec, type::small, Weight::Medium);
        p.text(preset, gx + 2, top + 25, gw - 4, 13, p.fit(preset, style, gw - 4, type::floor));
    }
    draw_live_analyzer(p, panel, gx, top + 40, gw, 42, ink);

    constexpr float card_w = (gw - 2 * gap) / 3;
    const auto cards = cards_of(s);
    for (unsigned k = 0; k < cards.size(); ++k)
        instrument_control_plot(p, cards[k], bindings[k].kind == Binding::Kind::PluginParam,
                          k, gx + k * (card_w + gap), top + 84, card_w);
}

} // namespace

void draw_live_analyzer(ui::Paint& p, const PluginPanel& panel, float x, float y, float w, float h,
                        juce::Colour accent, bool bypass) {
    using namespace ui;
    if (!keep_workshop_plugin_ui(panel)) {
        draw_workshop_waveform(p, panel, x, y, w, h, bypass, accent);
        return;
    }
    constexpr float gap = 4, padding = 5;
    const float scope_w = (w - gap) * .56f;
    const float spectrum_x = x + scope_w + gap, spectrum_w = w - scope_w - gap;
    const auto ink = bypass ? colour::mid : accent;
    const auto right_ink = bypass ? colour::mid : juce::Colour(0xffbfa4ff);
    const float top = y + 16, floor = y + h - 4, height = std::max(1.f, floor - top);
    const float mid = top + height * .5f;
    const float scope_span = scope_w - 2 * padding, spectrum_span = spectrum_w - 2 * padding;
    p.fill(x, y, scope_w, h, juce::Colour(0xff101722), 3);
    p.fill(spectrum_x, y, spectrum_w, h, juce::Colour(0xff101722), 3);
    p.ring(x, y, scope_w, h, ink.withAlpha(.35f), 3);
    p.ring(spectrum_x, y, spectrum_w, h, ink.withAlpha(.35f), 3);
    p.text(bypass ? "BYPASSED" : "OUTPUT WAVE", x + padding, y + 1, scope_span, 13, caps_text(ink));
    p.text("SPECTRUM", spectrum_x + padding, y + 1, spectrum_span, 13, caps_text(ink));
    for (unsigned line = 0; line <= 4; ++line) {
        const float position = line / 4.f;
        p.fill(x + padding + position * scope_span, top, 1, height, ink.withAlpha(.14f));
        p.fill(spectrum_x + padding + position * spectrum_span, top, 1, height, ink.withAlpha(.14f));
        p.fill(x + padding, top + position * height, scope_span, 1, ink.withAlpha(line == 2 ? .34f : .12f));
        p.fill(spectrum_x + padding, top + position * height, spectrum_span, 1, ink.withAlpha(.12f));
    }
    if (bypass) return;

    auto channel = [&](const auto& peaks, juce::Colour colour) {
        juce::Path trace;
        float peak = 0.f;
        for (unsigned i = 0; i < peaks.size(); ++i) {
            const auto pair = peaks[i];
            const float lo = int16_t(pair & 65535u) / 32768.f, hi = int16_t(pair >> 16) / 32768.f;
            peak = std::max({peak, std::abs(lo), std::abs(hi)});
            const float at = x + padding + i * scope_span / float(peaks.size() - 1);
            const auto ordinate = [&](float sample) { return mid - height * .48f * sample; };
            if (pair) p.fill(at, ordinate(hi), 1.2f, std::max(1.f, ordinate(lo) - ordinate(hi)), colour.withAlpha(.55f));
            const float signal = ordinate((lo + hi) * .5f);
            if (i) trace.lineTo(at, signal); else trace.startNewSubPath(at, signal);
        }
        p.g.setColour(colour.withAlpha(.85f));
        p.g.strokePath(trace, juce::PathStrokeType(1.f));
        return peak;
    };
    const auto db = [](float peak) {
        return peak > .001f ? juce::String(int(std::lround(20.f * std::log10(peak)))) : juce::String("--");
    };
    const auto right_peak = channel(panel.scope_right, right_ink);
    const auto left_peak = channel(panel.scope, ink);
    const auto levels = "L " + db(left_peak) + "  R " + db(right_peak);
    p.text(levels, x + 81, y + 1, scope_w - 86, 13, plain(colour::sec, type::small),
           juce::Justification::centredRight);

    juce::Path spectrum, area;
    area.startNewSubPath(spectrum_x + padding, floor);
    for (unsigned i = 0; i < panel.eq_spectrum.size(); ++i) {
        const float at = spectrum_x + padding + i * spectrum_span / float(panel.eq_spectrum.size() - 1);
        const float signal = floor - height * std::clamp(panel.eq_spectrum[i], 0.f, 1.f);
        if (i) spectrum.lineTo(at, signal); else spectrum.startNewSubPath(at, signal);
        area.lineTo(at, signal);
    }
    area.lineTo(spectrum_x + padding + spectrum_span, floor);
    area.closeSubPath();
    p.g.setGradientFill(juce::ColourGradient(ink.withAlpha(.68f), spectrum_x, top,
                                            ink.withAlpha(.05f), spectrum_x, floor, false));
    p.g.fillPath(area);
    p.g.setColour(ink);
    p.g.strokePath(spectrum, juce::PathStrokeType(1.35f));
}

void draw_plugin_view(Paint& p, const Snapshot& s, unsigned entry, float top) {
    const auto& panel = s.ui.plugin_panel;
    const float x = zone::pad, w = zone::width - 2 * zone::pad;
    bool ours = panel.entry == entry;
    if (!ours || panel.status == PluginStatus::None || panel.status == PluginStatus::Loading) { // the plug, waiting
        p.text(plugin_name(s, entry), x, top + 4, w, 22, p.fit(plugin_name(s, entry), plain(colour::text, type::title, Weight::SemiBold), w, type::floor));
        icon(p, Icon::Plug, x + w / 2 - 14, top + 40, 28, colour::dim, 1.4f);
        p.text("OPENING", x, top + 74, w, 14, caps_text(colour::dim), juce::Justification::centred);
        return;
    }
    if (panel.status == PluginStatus::Failed || panel.status == PluginStatus::Crashed) {
        // The broken plug, the cause in two words, and (dim, for whoever fixes it) what the loader named.
        p.text(plugin_name(s, entry), x, top + 4, w, 22, p.fit(plugin_name(s, entry), plain(colour::text, type::title, Weight::SemiBold), w, type::floor));
        bool crashed = panel.status == PluginStatus::Crashed;
        auto error = juce::String::fromUTF8(panel.error);
        bool library = error.contains("shared object") || error.contains(".so");
        juce::String cause = crashed ? "STOPPED" : library ? "MISSING LIBRARY" : "WON'T OPEN";
        icon(p, Icon::PlugBroken, x + 2, top + 40, 40, colour::rec, 1.3f);
        icon(p, Icon::Warn, x + 56, top + 44, 13, colour::rec, 1.6f);
        p.text(cause, x + 74, top + 42, w - 74, 16, caps_text(colour::rec, type::body));
        juce::String code = crashed ? juce::String() : library ? error.upToFirstOccurrenceOf(":", false, false).trim() : error;
        if (code.isNotEmpty()) p.text(code, x + 56, top + 62, w - 56, 14, p.fit(code, plain(colour::dim, type::small), w - 56, type::floor));
        return;
    }
    if (panel.instrument && panel.live_analyzer && juce::StringRef(panel.name) != juce::StringRef("Sub Foundry")) {
        if (panel.rows && !s.ui.shift && juce::StringRef(panel.name) == juce::StringRef("Frodhes"))
            draw_plugin_controls(p, panel, top, s.ui.track);
        else instrument_workshop_view(p, s, top);
        return;
    }
    if (panel.instrument_visual || (panel.live_analyzer && (s.ui.shift || !panel.rows))) {
        live_plugin_view(p, s, top);
        return;
    }
    if (!panel.rows) {
        icon(p, Icon::Sliders, x + w / 2 - 14, top + 40, 28, colour::dim, 1.4f);
        p.text("NONE", x, top + 74, w, 14, caps_text(colour::dim), juce::Justification::centred);
        return;
    }
    if (panel.rows == 2 && juce::StringRef(panel.name) == juce::StringRef("Afterglow Piano")) {
        instrument_scope_view(p, s, top, false);
        return;
    }
    if ((panel.rows == 3 || panel.rows == 6 || panel.rows == 7 || panel.rows == 8 || panel.rows == 9) && juce::StringRef(panel.name) == juce::StringRef("Afterglow Piano")) {
        ambient_view(p, s, top, true);
        return;
    }
    if (panel.rows == 7 && juce::StringRef(panel.name) == juce::StringRef("Stillwater")) {
        live_instrument_view(p, s, top, LiveInstrument::Stillwater);
        return;
    }
    if (panel.rows == 21 && juce::StringRef(panel.name) == juce::StringRef("Sub Foundry")) {
        sub_foundry_view(p, s, top);
        return;
    }
    if (panel.rows == 17 && juce::StringRef(panel.name) == juce::StringRef("Astral Loom")) {
        keyboard_library_view(p, s, top, true);
        return;
    }
    if (panel.rows == 5 && juce::StringRef(panel.name) == juce::StringRef("Room Grand")) {
        keyboard_library_view(p, s, top, false);
        return;
    }
    if ((panel.rows == 1 || panel.rows == 3) && juce::StringRef(panel.name) == juce::StringRef("Room Grand")) {
        room_grand_view(p, s, top);
        return;
    }
    if ((panel.rows >= 6 && panel.rows <= 8) && juce::StringRef(panel.name) == juce::StringRef("Velvet Choir")) {
        live_instrument_view(p, s, top, LiveInstrument::Choir);
        return;
    }
    if (panel.rows == 28 && juce::StringRef(panel.name) == juce::StringRef("Dex FM")) {
        live_instrument_view(p, s, top, LiveInstrument::Dex);
        return;
    }
    if (panel.rows == 16 && juce::StringRef(panel.name) == juce::StringRef("Sloop FM6")) {
        live_instrument_view(p, s, top, LiveInstrument::Sloop);
        return;
    }
    if ((panel.rows == 6 || panel.rows == 9 || panel.rows == 12 || panel.rows == 16) && juce::String::fromUTF8(panel.name) == "Layer House") {
        layer_house_view(p, s, top);
        return;
    }
    if (panel.live_analyzer) {
        live_plugin_view(p, s, top);
        return;
    }
    // Six rows: the three in hand raised on one panel, ticked in their knobs' colours.
    constexpr float row_h = 23;
    unsigned hand = panel.row * 3u;
    int first_hand = -1;
    for (unsigned k = 0; k < panel.window_rows * 3u; ++k) if (panel.window_first + k == hand) { first_hand = int(k); break; }
    if (first_hand >= 0) p.fill(x, top + 2 + first_hand * row_h, w, 3 * row_h, colour::panel, 3);
    for (unsigned k = 0; k < panel.window_rows * 3u; ++k) {
        const auto& param = panel.window[k];
        if (!param.used) continue;
        unsigned at = panel.window_first + k;
        bool held = at >= hand && at < hand + 3;
        float y = top + 2 + k * row_h;
        if (held) p.fill(x + 4, y + 5, 2.5f, row_h - 10, knob_shade(s.ui.track, at - hand), 1);
        p.text(juce::String(at + 1), x + 6, y, 22, row_h, plain(colour::dim, type::small, Weight::SemiBold), juce::Justification::centredRight);
        juce::String name = juce::String::fromUTF8(param.name), value = juce::String::fromUTF8(param.text);
        auto value_style = plain(colour::mid, type::body, Weight::SemiBold);
        float vw = held ? 0 : p.width(value, value_style) + 10;
        auto name_style = plain(held ? colour::text : colour::mid, type::body, held ? Weight::Medium : Weight::Regular);
        p.text(name, x + 34, y, w - 42 - vw, row_h, p.fit(name, name_style, w - 42 - vw, type::floor));
        if (!held) p.text(value, x + w - 8 - vw + 10, y, vw - 10, row_h, value_style, juce::Justification::centredRight);
    }
}

// --- The page under B1-B3 (docs/PLUGIN_PAGES_PLAN.md, picks 3 c and 4 a) ---

namespace {
constexpr float cols[3] {58, 160, 262}; // each column over its pot
constexpr float arc_r = 20, sweep = 0.75f * juce::MathConstants<float>::pi; // 270 degrees, the gap at the bottom

// The page line: the page's name (the plug-in's where its page has none), and where it is among the pages.
void page_line(Paint& p, const Snapshot& s, unsigned entry, float y) {
    const auto& panel = s.ui.plugin_panel;
    auto c = colour_of(s);
    juce::String name = panel.page_name[0] ? juce::String::fromUTF8(panel.page_name) : plugin_name(s, entry);
    p.text(name.toUpperCase(), zone::pad, y, 220, 14, p.fit(name.toUpperCase(), caps_text(c), 220, type::floor));
    if (panel.pages > 24) // too many for dots: the count
        p.text(juce::String(panel.page + 1) + " / " + juce::String(panel.pages), zone::width - zone::pad - 80, y, 80, 14,
               plain(colour::mid, type::small, Weight::SemiBold), juce::Justification::centredRight);
    else if (panel.pages > 1)
        page_dots(p, zone::width - zone::pad, y + 7, int(panel.pages), int(panel.page), c, true);
}

juce::Path arc_between(float cx, float cy, float from, float to) {
    juce::Path path;
    path.addCentredArc(cx, cy, arc_r, arc_r, 0, from, to, true);
    return path;
}

// One parameter over its pot: the arc (a choice of a few values: its values as dots), its value, its name. In hand,
// the knob's colour and what its card showed: faint with a white dot where the pot sits until the pot meets the value
// (takeover), the fader glyph, the Shift mark, the snowflake of a frozen track.
void arc_cell(Paint& p, float cx, float top, const PluginPanelParam& param, bool hand, const ui::KnobCard* card, juce::Colour shade) {
    if (!param.used) return;
    const float cy = top + 28;
    const bool pickup = hand && card && card->state == ui::KnobCard::State::Pickup;
    const bool turning = hand && card && card->state == ui::KnobCard::State::Turning;
    juce::Colour c = hand ? (pickup ? shade.withAlpha(0.45f) : shade) : colour::mid;
    auto stroke = [&](const juce::Path& path, juce::Colour colour, float w) {
        p.g.setColour(colour);
        p.g.strokePath(path, juce::PathStrokeType(w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };
    stroke(arc_between(cx, cy, -sweep, sweep), colour::line, 3);
    float v = std::clamp(param.value / float(plugin_param_max), 0.0f, 1.0f);
    auto point = [&](float at, float r) { float a = -sweep + 2 * sweep * at; return juce::Point<float>(cx + r * std::sin(a), cy - r * std::cos(a)); };
    if (param.steps >= 2 && param.steps <= 12) { // its values as dots, the one it is on in the colour
        unsigned now = unsigned(std::lround(v * float(param.steps - 1)));
        for (unsigned n = 0; n < param.steps; ++n) {
            auto at = point(float(n) / float(param.steps - 1), arc_r);
            p.circle(at.x, at.y, n == now ? 3.2f : 2.0f, n == now ? c : colour::mid);
        }
    } else if (v > 0.002f) {
        stroke(arc_between(cx, cy, -sweep, -sweep + 2 * sweep * v), c, turning ? 4.0f : 3.0f);
    }
    if (pickup && card->pot >= 0) {
        auto at = point(std::clamp(card->pot, 0.0f, 1.0f), arc_r);
        p.circle(at.x, at.y, 3.2f, colour::text);
        p.circle(at.x, at.y, 1.9f, colour::bg);
    }
    if (hand && card) {
        if (card->fader) icon(p, Icon::Fader, cx + 22, top + 2, 10, pickup ? colour::mid : shade, 1.4f);
        if (card->shift) shift_mark(p, cx - 32, top + 4, card->shift == 2 ? shade : colour::mid, card->shift == 2);
        if (card->frozen) icon(p, Icon::Snow, cx + 22, top + 14, 10, colour::ice, 1.4f);
    }
    juce::String value = juce::String::fromUTF8(param.text);
    auto value_style = plain(hand ? (pickup ? colour::dim : colour::text) : colour::mid, type::list, Weight::Medium);
    p.text(value, cx - 50, top + 50, 100, 18, p.fit(value, value_style, 100, type::floor), juce::Justification::centred);
    juce::String name = juce::String::fromUTF8(param.name);
    auto name_style = plain(hand ? c : colour::dim, type::small, Weight::SemiBold);
    if (pickup) name_style = plain(shade, type::small, Weight::SemiBold);
    auto lines = p.wrap(name, name_style, 100, 2);
    if (lines.size() <= 1) p.text(name, cx - 50, top + 69, 100, 13, name_style, juce::Justification::centred);
    else for (int n = 0; n < 2; ++n) p.text(lines[n], cx - 50, top + 64 + 12 * n, 100, 12, name_style, juce::Justification::centred);
}

// A row of three (its slots row * 3 .. + 2) at `top`; the row in hand raised, with its cards' states.
void arc_row(Paint& p, const Snapshot& s, unsigned row, float top, const std::array<ui::KnobCard, 3>& cards) {
    const auto& panel = s.ui.plugin_panel;
    bool hand = row == panel.row;
    if (hand) p.fill(zone::pad, top, zone::width - 2 * zone::pad, 90, colour::panel, 4);
    for (unsigned k = 0; k < 3; ++k) {
        unsigned at = row * 3 + k;
        if (at < panel.window_first || at >= panel.window_first + panel.window_rows * 3u) continue;
        const auto& param = panel.window[at - panel.window_first];
        arc_cell(p, cols[k], top, param, hand, hand ? &cards[k] : nullptr, knob_shade(s.ui.track, k));
    }
}

// The colour a picture's part takes: its knob's while its row is in hand, else grey.
juce::Colour part_colour(const Snapshot& s, int slot) {
    const auto& panel = s.ui.plugin_panel;
    if (slot < 0 || unsigned(slot) / 3 != panel.row) return colour::mid;
    return knob_shade(s.ui.track, unsigned(slot) % 3);
}

// Times from the value's text where it names them ("12 ms"), else from where it sits (0..1, squared, up to 4 s).
double seconds_of(const PluginPanelParam& param) {
    double out = 0;
    if (value_in(param.text, false, out)) return std::clamp(out, 0.0, 30.0);
    double v = param.value / double(plugin_param_max);
    return v * v * 4.0;
}

void envelope_picture(Paint& p, const Snapshot& s, float x, float y, float w, float h, juce::Colour own = {}) {
    const auto& panel = s.ui.plugin_panel;
    double a = seconds_of(panel.picture_params[0]), d = seconds_of(panel.picture_params[1]), r = seconds_of(panel.picture_params[3]);
    float sustain = std::clamp(panel.picture_params[2].value / float(plugin_param_max), 0.0f, 1.0f);
    double hold = std::max(0.2, 0.3 * (a + d + r)), length = std::max(a + d + hold + r, 0.01);
    auto x_of = [&](double t) { return x + float(t / length) * w; };
    float top = y + 2, base = y + h - 2;
    auto y_of = [&](float l) { return base - l * (base - top); };
    struct Part { double t0, t1; float l0, l1; int slot; };
    std::array<Part, 4> parts {Part {0, a, 0, 1, panel.picture_slots[0]}, Part {a, a + d, 1, sustain, panel.picture_slots[1]},
                               Part {a + d, a + d + hold, sustain, sustain, panel.picture_slots[2]},
                               Part {a + d + hold, length, sustain, 0, panel.picture_slots[3]}};
    juce::Path area;
    area.startNewSubPath(x_of(0), base);
    for (const auto& part : parts) area.lineTo(x_of(part.t1), y_of(part.l1));
    area.closeSubPath();
    p.g.setColour((own.isTransparent() ? colour_of(s) : own).withAlpha(0.1f));
    p.g.fillPath(area);
    for (const auto& part : parts) {
        auto c = own.isTransparent() ? part_colour(s, part.slot) : own;
        p.g.setColour(c);
        p.g.drawLine(x_of(part.t0), y_of(part.l0), x_of(part.t1), y_of(part.l1), 2);
        if (c != colour::mid) { p.circle(x_of(part.t1), y_of(part.l1), 3, c); p.circle(x_of(part.t1), y_of(part.l1), 1.6f, colour::bg); }
    }
}

void filter_picture(Paint& p, const Snapshot& s, float x, float y, float w, float h, juce::Colour own = {}) {
    const auto& panel = s.ui.plugin_panel;
    double hz = 0;
    if (!value_in(panel.picture_params[0].text, true, hz))
        hz = 20.0 * std::pow(1000.0, panel.picture_params[0].value / double(plugin_param_max));
    float q = 0.7f + 5.0f * std::clamp(panel.picture_params[1].value / float(plugin_param_max), 0.0f, 1.0f);
    draw_filter_curve(p, x, y, w, h, 0, 2, float(std::clamp(hz, 20.0, 20000.0)), q, own.isTransparent() ? colour_of(s) : own,
                      own.isTransparent() ? part_colour(s, panel.picture_slots[0]) : own, own.isTransparent());
}
// Up to three values large (a page file's "value" picture): each its name in its knob's colour while its row is in
// hand, its value, a gauge.
void value_picture(Paint& p, const Snapshot& s, float x, float y, float w, float h) {
    const auto& panel = s.ui.plugin_panel;
    unsigned count = 0;
    while (count < 3 && panel.picture_params[count].used) ++count;
    if (!count) return;
    float cw = w / float(count);
    for (unsigned n = 0; n < count; ++n) {
        const auto& param = panel.picture_params[n];
        auto c = part_colour(s, panel.picture_slots[n]);
        float cx = x + n * cw;
        juce::String name = juce::String::fromUTF8(param.name), value = juce::String::fromUTF8(param.text);
        auto name_style = plain(c == colour::mid ? colour::sec : c, type::small, Weight::SemiBold);
        p.text(name, cx, y + 4, cw, 14, p.fit(name, name_style, cw - 8, type::floor), juce::Justification::centred);
        auto value_style = plain(colour::text, 30, Weight::Medium);
        p.text(value, cx, y + 20, cw, 36, p.fit(value, value_style, cw - 8, type::value), juce::Justification::centred);
        float gw = std::min(cw - 24, 80.0f), gx = cx + (cw - gw) / 2, gy = y + h - 14;
        p.fill(gx, gy, gw, 2, colour::line, 1);
        p.fill(gx, gy, gw * std::clamp(param.value / float(plugin_param_max), 0.0f, 1.0f), 2, c == colour::mid ? colour::sec : c, 1);
    }
}

// An EQ (a page file's "eq" picture): its bands' response summed, each band a dot at its frequency and gain.
// Frequency and gain from the values' text where it names Hz and dB, else from where they sit (20 Hz-20 kHz,
// -18-+18 dB); Q from a plain number, else from where it sits (0.3-10). The scale follows the largest gain.
void eq_picture(Paint& p, const Snapshot& s, float x, float y, float w, float h, juce::Colour own = {}) {
    const auto& panel = s.ui.plugin_panel;
    struct Band { double hz, db, q; int slot; };
    std::vector<Band> bands;
    for (unsigned b = 0; b + 2 < PagePictureSlots::most; b += 3) {
        const auto& f = panel.picture_params[b], &g = panel.picture_params[b + 1], &q = panel.picture_params[b + 2];
        if (!f.used && !g.used) continue;
        Band band {1000, 0, 0.7, panel.picture_slots[b] >= 0 ? panel.picture_slots[b] : panel.picture_slots[b + 1]};
        if (f.used && !value_in(f.text, true, band.hz)) band.hz = 20.0 * std::pow(1000.0, f.value / double(plugin_param_max));
        if (g.used && !decibels_in(g.text, band.db)) band.db = (g.value / double(plugin_param_max) * 2 - 1) * 18;
        if (q.used) {
            double plain_q = std::atof(q.text);
            band.q = plain_q > 0.05 && plain_q < 40 ? plain_q : 0.3 * std::pow(33.0, q.value / double(plugin_param_max));
        }
        band.hz = std::clamp(band.hz, 20.0, 20000.0);
        bands.push_back(band);
    }
    double most = 0; // the scale: the largest band gain, up to the next 6, 12, 18 or 24 dB
    for (const auto& b : bands) most = std::max(most, std::abs(b.db));
    const double range = std::clamp(std::ceil(most / 6.0) * 6.0, 6.0, 24.0);
    auto x_of = [&](double hz) { return x + float(std::log(hz / 20.0) / std::log(1000.0)) * w; };
    auto y_of = [&](double db) { return y + h / 2 - float(std::clamp(db, -range, range) / range) * (h / 2 - 2); };
    p.fill(x, y + h / 2 - 0.5f, w, 1, colour::line);
    juce::Path line, area;
    for (int n = 0; n <= 120; ++n) {
        double hz = 20.0 * std::pow(1000.0, n / 120.0), db = 0;
        for (const auto& b : bands) { // a peaking band's gain at hz (its analogue prototype)
            double r = hz / b.hz, a = std::pow(10.0, b.db / 40.0), one = (1 - r * r) * (1 - r * r);
            double num = one + std::pow(r * a / b.q, 2), den = one + std::pow(r / (a * b.q), 2);
            db += 10 * std::log10(std::max(num / den, 1e-9));
        }
        float px = x_of(hz), py = y_of(db);
        if (n == 0) { line.startNewSubPath(px, py); area.startNewSubPath(px, y + h / 2); }
        line.lineTo(px, py); area.lineTo(px, py);
    }
    area.lineTo(x + w, y + h / 2); area.closeSubPath();
    auto curve = own.isTransparent() ? colour_of(s) : own;
    p.g.setColour(curve.withAlpha(0.14f)); p.g.fillPath(area);
    p.g.setColour(curve); p.g.strokePath(line, juce::PathStrokeType(1.5f));
    for (const auto& b : bands) {
        auto c = part_colour(s, b.slot);
        p.circle(x_of(b.hz), y_of(b.db), 3.5f, c);
        p.circle(x_of(b.hz), y_of(b.db), 1.8f, colour::bg);
    }
    auto axis = plain(colour::dim, type::small, Weight::SemiBold);
    p.text("20 Hz", x, y + h + 3, 44, 12, axis); p.text("200 Hz", x + w / 3 - 18, y + h + 3, 50, 12, axis);
    p.text("2 kHz", x + 2 * w / 3 - 16, y + h + 3, 50, 12, axis); p.text("20 kHz", x + w - 50, y + h + 3, 50, 12, axis, juce::Justification::centredRight);
}

// The plug-in's sound as measured here, left over right, a block a bar, newest on the right.
void scope_lanes(Paint& p, const Snapshot& s, float x, float y, float w, float h, juce::Colour own = {}) {
    const auto& panel = s.ui.plugin_panel;
    auto c = own.isTransparent() ? colour_of(s) : own;
    float lane = (h - 4) / 2, step = w / float(panel.meter_scope.size());
    for (int side = 0; side < 2; ++side) {
        float mid = y + side * (lane + 4) + lane / 2;
        p.fill(x, mid - 0.5f, w, 1, colour::line);
        for (size_t n = 0; n < panel.meter_scope.size(); ++n) {
            uint32_t v = panel.meter_scope[n] >> (side * 16);
            float lo = int8_t(v & 0xff) / 127.0f, hi = int8_t((v >> 8) & 0xff) / 127.0f;
            if (hi - lo < 0.01f) continue;
            p.fill(x + n * step + 0.4f, mid - hi * lane / 2, std::max(1.2f, step - 1.2f), (hi - lo) * lane / 2,
                   c.withAlpha(0.35f + 0.65f * float(n) / float(panel.meter_scope.size())), 0.8f);
        }
    }
}
// --- Skins (docs/PLUGIN_PAGES.md, Skins; plugin/plugin_skin.h) ---

const PluginSkin::Page* skin_page_of(const Snapshot& s) {
    const auto& panel = s.ui.plugin_panel;
    const auto& skin = s.plugin_skin;
    if (!skin) return nullptr;
    if (panel.skin_page == -2) return skin->has_strip ? &skin->strip : nullptr;
    if (panel.skin_page >= 0 && size_t(panel.skin_page) < skin->pages.size()) return &skin->pages[size_t(panel.skin_page)].second;
    return nullptr;
}

// A label's or value's text in the skin's font (Melodee's where it has none), 11 px and up.
void skin_text(Paint& p, const PluginSkin& skin, const juce::String& text, juce::Rectangle<float> box, int size, juce::Colour c, int align) {
    auto just = align < 0 ? juce::Justification::centredLeft : align > 0 ? juce::Justification::centredRight : juce::Justification::centred;
    float height = float(std::max(11, size));
    if (!skin.font) { p.text(text, box.getX(), box.getY(), box.getWidth(), box.getHeight(), plain(c, height, Weight::SemiBold), just); return; }
    p.g.setColour(c);
    p.g.setFont(juce::Font(juce::FontOptions(skin.font).withHeight(height)));
    p.g.drawText(text, box, just, false);
}

// A filmstrip's frame `frame` into `box`.
void skin_frame(Paint& p, const juce::Image& strip, unsigned frames, unsigned frame, juce::Rectangle<float> box) {
    int fh = strip.getHeight() / int(std::max(1u, frames));
    if (fh <= 0) return;
    p.g.drawImage(strip, int(box.getX()), int(box.getY()), int(box.getWidth()), int(box.getHeight()),
                  0, int(std::min(frame, frames - 1)) * fh, strip.getWidth(), fh);
}

// Melodee's marks on a control K1-K3 turn (one a parameter: its knob, switch or slider, else its value). The maker
// draws each control once, so Melodee adds only what the art cannot know: a white dot where the pot sits while it has
// not reached the value (on a knob's sweep, else along the control's bottom), the fader glyph while the fader drives
// it, the Shift mark while Shift reaches its second parameter.
void skin_ring(Paint& p, const PluginSkin::Control& c, juce::Rectangle<float> box, juce::Colour shade, const ui::KnobCard* card) {
    if (!card) return;
    const bool pickup = card->state == ui::KnobCard::State::Pickup;
    if (pickup && card->pot >= 0) {
        float dx = 0, dy = 0;
        if (c.kind == SkinKind::Knob) {
            float r = std::max(box.getWidth(), box.getHeight()) / 2 + 3;
            float a = (-0.75f + 1.5f * std::clamp(card->pot, 0.0f, 1.0f)) * juce::MathConstants<float>::pi;
            dx = box.getCentreX() + r * std::sin(a);
            dy = box.getCentreY() - r * std::cos(a);
        } else {
            auto under = box.expanded(4);
            dx = under.getX() + under.getWidth() * std::clamp(card->pot, 0.0f, 1.0f);
            dy = under.getBottom();
        }
        p.circle(dx, dy, 3.2f, colour::text);
        p.circle(dx, dy, 1.9f, colour::bg);
    }
    if (card->fader) {
        float fx = box.getRight() + 1, fy = box.getY() - 8;
        p.fill(fx - 6, fy - 1, 14, 14, colour::bg, 3);
        icon(p, Icon::Fader, fx - 4, fy + 1, 10, pickup ? colour::mid : shade, 1.4f);
    }
    if (card->shift == 2) shift_mark(p, box.getX() - 9, box.getY() - 6, shade, true);
}

// A skin page (or strip) at `ox`, `oy`: its background, each control, then Melodee's rings on those K1-K3 turn.
void draw_skin_page(Paint& p, const Snapshot& s, const PluginSkin& skin, const PluginSkin::Page& page, float ox, float oy,
                    const std::array<ui::KnobCard, 3>& cards) {
    const auto& panel = s.ui.plugin_panel;
    if (page.background.isValid()) p.g.drawImageAt(page.background, int(ox), int(oy));
    size_t at = 0; // the next control's place in panel.skin_params
    struct Ring { const PluginSkin::Control* control; juce::Rectangle<float> box; unsigned k; };
    std::vector<Ring> rings;
    std::vector<int> handled; // parameters with a knob, switch or slider here: their values get no ring
    for (const auto& c : page.controls)
        if (c.param >= 0 && (c.kind == SkinKind::Knob || c.kind == SkinKind::Switch || c.kind == SkinKind::Slider)) handled.push_back(c.param);
    std::vector<int> ringed;
    for (const auto& c : page.controls) {
        auto box = c.box.toFloat().translated(ox, oy);
        const PluginPanelParam* param = nullptr;
        if (c.param >= 0) {
            if (at >= panel.skin_params.size()) continue;
            int slot = panel.skin_slots[at];
            param = &panel.skin_params[at++];
            bool value_only = c.kind == SkinKind::Value && std::find(handled.begin(), handled.end(), c.param) != handled.end();
            bool once = std::find(ringed.begin(), ringed.end(), c.param) == ringed.end();
            if (slot >= 0 && unsigned(slot) / 3 == panel.row && !value_only && once) {
                rings.push_back({&c, box, unsigned(slot) % 3});
                ringed.push_back(c.param);
            }
        }
        float v = param ? std::clamp(param->value / float(plugin_param_max), 0.0f, 1.0f) : 0.0f;
        juce::Colour ink = c.coloured ? c.colour : colour::text;
        switch (c.kind) {
        case SkinKind::Image: p.g.drawImage(c.image, box); break;
        case SkinKind::Knob: case SkinKind::Switch:
            skin_frame(p, c.image, c.frames, unsigned(std::lround(v * float(c.frames - 1))), box);
            break;
        case SkinKind::Meter: {
            float level = (c.of == "in" ? panel.level_in : panel.level_out) / 1000.0f;
            skin_frame(p, c.image, c.frames, unsigned(std::lround(std::clamp(level, 0.0f, 1.0f) * float(c.frames - 1))), box);
            break;
        }
        case SkinKind::Slider: {
            if (c.image.isValid()) p.g.drawImage(c.image, box);
            if (!c.cap.isValid()) break;
            float cw = float(c.cap.getWidth()), ch = float(c.cap.getHeight());
            float cx = c.horizontal ? box.getX() + v * (box.getWidth() - cw) : box.getCentreX() - cw / 2;
            float cy = c.horizontal ? box.getCentreY() - ch / 2 : box.getY() + (1 - v) * (box.getHeight() - ch);
            p.g.drawImageAt(c.cap, int(std::lround(cx)), int(std::lround(cy)));
            break;
        }
        case SkinKind::Value: if (param) skin_text(p, skin, juce::String::fromUTF8(param->text), box, c.text_size, ink, c.align); break;
        case SkinKind::Label: skin_text(p, skin, c.text, box, c.text_size, ink, c.align); break;
        case SkinKind::Curve: {
            juce::Colour own = c.coloured ? c.colour : colour_of(s);
            bool labels_fit = box.getHeight() > 40;
            float h = labels_fit ? box.getHeight() - 15 : box.getHeight();
            if (c.of == "envelope" && panel.picture == uint8_t(PagePicture::Envelope)) envelope_picture(p, s, box.getX(), box.getY(), box.getWidth(), box.getHeight(), own);
            else if (c.of == "filter" && panel.picture == uint8_t(PagePicture::Filter)) filter_picture(p, s, box.getX(), box.getY(), box.getWidth(), h, own);
            else if (c.of == "eq" && panel.picture == uint8_t(PagePicture::Eq)) eq_picture(p, s, box.getX(), box.getY(), box.getWidth(), h, own);
            break;
        }
        case SkinKind::Scope: if (panel.measured) scope_lanes(p, s, box.getX(), box.getY(), box.getWidth(), box.getHeight(), c.coloured ? c.colour : juce::Colour()); break;
        }
    }
    for (const auto& r : rings) skin_ring(p, *r.control, r.box, knob_shade(s.ui.track, r.k), &cards[r.k]);
}
} // namespace

// A plug-in's own picture (plugin/plugin_screen.h) at x, y. Made into an image again only when a new one came; the
// screen thread draws every page, so the image kept here is that thread's alone.
void draw_screen_picture(Paint& p, const ScreenPicture& picture, float x, float y) {
    static juce::Image image;
    static uint32_t serial = 0;
    static uint8_t entry = no_plugin;
    const int w = picture.width, h = picture.height;
    if (picture.pixels.size() < size_t(w) * size_t(h) || w <= 0 || h <= 0) return;
    if (!image.isValid() || image.getWidth() != w || image.getHeight() != h || serial != picture.serial || entry != picture.entry) {
        if (!image.isValid() || image.getWidth() != w || image.getHeight() != h) image = juce::Image(juce::Image::ARGB, w, h, false);
        juce::Image::BitmapData bits(image, juce::Image::BitmapData::writeOnly);
        for (int row = 0; row < h; ++row) {
            auto* line = reinterpret_cast<juce::PixelARGB*>(bits.getLinePointer(row));
            const uint32_t* from = picture.pixels.data() + size_t(row) * size_t(w);
            for (int col = 0; col < w; ++col) {
                uint32_t c = from[col];
                line[col].setARGB(0xff, uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c));
            }
        }
        serial = picture.serial;
        entry = picture.entry;
    }
    p.g.drawImageAt(image, int(x), int(y));
}

bool plugin_screen_shown(const Snapshot& s, uint8_t area) {
    const auto at = plugin_screen_place(s.project, s.ui);
    const auto& picture = s.plugin_picture;
    return at.entry >= 0 && at.area == area && s.ui.plugin_panel.screen && picture && picture->entry == unsigned(at.entry)
        && picture->area == area;
}

bool plugin_page_shown(const Snapshot& s, unsigned entry) {
    const auto& panel = s.ui.plugin_panel;
    if (entry >= max_plugins || panel.entry != entry || panel.status != PluginStatus::Ready || (!panel.rows && !panel.screen)) return false;
    if (panel.workshop && !panel.screen) return false;
    auto cards = cards_of(s);
    return std::none_of(cards.begin(), cards.end(), [](const ui::KnobCard& c) { return c.knob_fx; });
}

void draw_plugin_page(Paint& p, const Snapshot& s, unsigned entry, const SoftKeys& keys) {
    frame_keys(p, s, keys);
    const auto& panel = s.ui.plugin_panel;
    auto cards = cards_of(s);
    if (auto* seen = controls_sink()) { // the preview's controls audit: the arcs are this page's pots
        seen->knobs_drawn = true;
        for (unsigned k = 0; k < 3; ++k) seen->knobs[k] = panel.params[k].used ? juce::String::fromUTF8(panel.params[k].name) : juce::String();
    }
    if (plugin_screen_shown(s) && s.plugin_picture->entry == entry) {
        draw_screen_picture(p, *s.plugin_picture, 0, float(zone::header_y)); // its own page under B1-B3, all of it
        return;
    }
    if (s.ui.settings.plugin_skins && panel.skin_page >= 0) // the maker's skin, under B1-B3; Melodee's dots over it
        if (const auto* page = skin_page_of(s)) {
            draw_skin_page(p, s, *s.plugin_skin, *page, 0, float(zone::header_y), cards);
            if (panel.pages > 1 && panel.pages <= 24) page_dots(p, zone::width - zone::pad, 33, int(panel.pages), int(panel.page), colour_of(s), true);
            return;
        }
    page_line(p, s, entry, 26);
    if (panel.picture == uint8_t(PagePicture::Envelope)) {
        envelope_picture(p, s, 14, 46, 292, 80);
        arc_row(p, s, panel.row, 132, cards);
    } else if (panel.picture == uint8_t(PagePicture::Filter)) {
        filter_picture(p, s, 14, 46, 292, 62);
        arc_row(p, s, panel.row, 132, cards);
    } else if (panel.picture == uint8_t(PagePicture::Value)) {
        value_picture(p, s, 14, 46, 292, 80);
        arc_row(p, s, panel.row, 132, cards);
    } else if (panel.picture == uint8_t(PagePicture::Eq)) {
        eq_picture(p, s, 14, 46, 292, 62);
        arc_row(p, s, panel.row, 132, cards);
    } else if (panel.picture == uint8_t(PagePicture::Scope) && panel.measured) {
        scope_lanes(p, s, 14, 52, 292, 70);
        arc_row(p, s, panel.row, 132, cards);
    } else {
        for (unsigned r = 0; r < panel.window_rows; ++r) arc_row(p, s, panel.window_first / 3u + r, 44 + 96 * float(r), cards);
    }
}

bool plugin_strip_shown(const Snapshot& s, unsigned entry) {
    const auto& panel = s.ui.plugin_panel;
    return s.ui.settings.plugin_skins && panel.entry == entry && panel.status == PluginStatus::Ready && panel.skin_page == -2
        && skin_page_of(s) != nullptr;
}

void draw_plugin_strip(Paint& p, const Snapshot& s, float x, float y) {
    if (const auto* strip = skin_page_of(s)) draw_skin_page(p, s, *s.plugin_skin, *strip, x, y, cards_of(s));
}

// An effect in a chain: its page line, then its output as it plays (left over right, a block a bar) and its levels in
// and out.
void draw_plugin_sound(Paint& p, const Snapshot& s, float x, float y, float w, float h) {
    const auto& panel = s.ui.plugin_panel;
    auto c = colour_of(s);
    juce::String name = panel.page_name[0] ? juce::String::fromUTF8(panel.page_name).toUpperCase() : juce::String();
    if (name.isNotEmpty()) p.text(name, x, y, w - 90, 14, p.fit(name, caps_text(c), w - 90, type::floor));
    if (panel.pages > 1 && panel.pages <= 24) page_dots(p, x + w, y + 7, int(panel.pages), int(panel.page), c, true);
    float sy = y + 18, sw = w - 96;
    if (h < 30 || !panel.measured) return;
    scope_lanes(p, s, x, sy, sw, std::max(16.0f, h - 18));
    float mx = x + sw + 14, mw = w - sw - 14;
    for (int n = 0; n < 2; ++n) {
        float my = sy + n * 20;
        p.text(n ? "OUT" : "IN", mx, my, 30, 14, caps_text(colour::dim));
        float level = (n ? panel.level_out : panel.level_in) / 1000.0f;
        p.fill(mx + 30, my + 5, mw - 30, 4, colour::line, 2);
        if (level > 0.001f) p.fill(mx + 30, my + 5, (mw - 30) * std::min(1.0f, level), 4, n ? c : colour::mid, 2);
    }
}

} // namespace melodee
