// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Tape Workshop contributors
// Instrument-specific host faces. Every moving trace is measured plug-in output;
// parameter dials use the host values and piano keys show held notes.
#include "melodee/pages/common.h"
#include "melodee/pages/strings_view.h"
#include "melodee/pages/electric_view.h"
#include "melodee/pages/orchestra_view.h"
#include "melodee/pages/plugin_pro_ui.h"
#include "melodee/pages/melody_forge_view.h"
#include "melodee/pages/plugin_waveform.h"
#include "melodee/pages/second_spin_vintage.h"
#include "melodee/pages/plugin_parameter_pages.h"
#include "melodee/shell/stage_map.h"
#include <string_view>

namespace melodee {
namespace {
using namespace ui;
enum class PluginFace { Piano, FM, Analogue, Atmosphere, Drums, Tape, Dynamics, Vocal };
struct Design { const char* name; PluginFace face; uint32_t accent, panel; const char* subtitle; };
constexpr Design designs[] {
    {"Afterglow Piano", PluginFace::Piano, 0xffffbd7d, 0xff29201b, "FELT / CHARACTER PIANO"},
    {"Room Grand", PluginFace::Piano, 0xffe6ce96, 0xff242220, "ACOUSTIC / GRAND PIANO"},
    {"Dex FM", PluginFace::FM, 0xff81e8a7, 0xff10241f, "FM SYNTHESIZER"},
    {"Sloop FM6", PluginFace::FM, 0xff74dce8, 0xff10212c, "SIX OPERATOR / FM"},
    {"Dexed Move", PluginFace::FM, 0xffa4baff, 0xff171c32, "FM / DIGITAL VOICE"},
    {"Dexed", PluginFace::FM, 0xffa4baff, 0xff171c32, "FM / DIGITAL VOICE"},
    {"Aphex Move", PluginFace::FM, 0xffff9fcb, 0xff2b192b, "DIGITAL / SYNTHESIZER"},
    {"Aphex", PluginFace::FM, 0xffff9fcb, 0xff2b192b, "DIGITAL / SYNTHESIZER"},
    {"Layer House", PluginFace::Analogue, 0xff6ce4d6, 0xff122824, "LAYERED / SYNTHESIZER"},
    {"Hera Move", PluginFace::Analogue, 0xffe3a7ff, 0xff24192d, "POLYPHONIC / SYNTHESIZER"},
    {"Hera", PluginFace::Analogue, 0xffe3a7ff, 0xff24192d, "POLYPHONIC / SYNTHESIZER"},
    {"Braids Move", PluginFace::Analogue, 0xffe8df98, 0xff242618, "MACRO / OSCILLATOR"},
    {"Braids", PluginFace::Analogue, 0xffe8df98, 0xff242618, "MACRO / OSCILLATOR"},
    {"Plaits Move", PluginFace::Atmosphere, 0xffffc58d, 0xff2b201d, "MACRO / VOICE"},
    {"Plaits", PluginFace::Atmosphere, 0xffffc58d, 0xff2b201d, "MACRO / VOICE"},
    {"Astral Loom", PluginFace::Atmosphere, 0xffb9a5ff, 0xff211a34, "CINEMATIC / TEXTURES"},
    {"Stillwater", PluginFace::Atmosphere, 0xff86caff, 0xff142433, "AMBIENT / INSTRUMENT"},
    {"Velvet Choir", PluginFace::Atmosphere, 0xfff0a9d1, 0xff2d1a2a, "CHOIR / ENSEMBLE"},
    {"Break 26", PluginFace::Drums, 0xffffb264, 0xff2a1d16, "RHYTHM / DRUM MACHINE"},
    {"Second Spin", PluginFace::Tape, 0xffe9bf89, 0xff29221b, "TAPE / COLOUR"},
    {"Rack 76", PluginFace::Dynamics, 0xff95baff, 0xff18202e, "DYNAMICS / LIMITING"},
    {"Gate Keeper", PluginFace::Dynamics, 0xff83e5bc, 0xff142820, "DYNAMICS / GATE"},
    {"Vocal Tune", PluginFace::Vocal, 0xff84e5f3, 0xff14262d, "VOCAL / PITCH"},
};
Design design_of(const PluginPanel& panel) {
    std::string_view name(panel.name);
    if (name.size() > 5 && name.substr(name.size()-5) == " (TW)") name.remove_suffix(5);
    for (const auto& d : designs) if (name == d.name) return d;
    return {"", panel.instrument ? PluginFace::Analogue : PluginFace::Dynamics, 0xffc49aff, 0xff211a2c,
            panel.instrument ? "INSTRUMENT / OUTPUT" : "EFFECT / OUTPUT"};
}
void spectrum(Paint& p, const PluginPanel& panel, float x, float y, float w, float h, juce::Colour ink, bool bypass) {
    p.fill(x, y, w, h, juce::Colour(0xff08090d), 3);
    p.text("SPECTRUM", x + 5, y + 2, w - 10, 11, p.fit("SPECTRUM", plain(ink, 8, Weight::Bold), w - 10, type::floor));
    const float floor = y + h - 4;
    for (unsigned i = 0; i < 32; ++i) {
        const float level = bypass ? 0.f : std::clamp(panel.eq_spectrum[i * 2], 0.f, 1.f);
        p.fill(x + 4 + i * (w - 8) / 32, floor - level * (h - 19),
               std::max(1.f, (w - 8) / 32 - 1), std::max(.5f, level * (h - 19)), ink.withAlpha(.8f));
    }
}

}

void draw_plugin_workshop_face(ui::Paint& p, const Snapshot& s, float top, bool expanded, bool bypass) {
    using namespace ui;
    if (is_workshop_electric(s.ui.plugin_panel)) { draw_electric_view(p,s,top,bypass); return; }
    if (is_workshop_orchestra(s.ui.plugin_panel)) { draw_orchestra_view(p,s,top,bypass); return; }
    if (is_workshop_strings(s.ui.plugin_panel)) { draw_strings_view(p, s, top, bypass); return; }
    if (s.ui.plugin_panel.forge_ready) { melody_forge_view(p, s, top); return; }
    if(draw_plugin_pro_ui(p,s,zone::pad,top,zone::width-2*zone::pad,zone::picture_h,expanded,bypass))return;
    const auto& panel = s.ui.plugin_panel;
    const auto d = design_of(panel);
    const auto ink = juce::Colour(d.accent), body = juce::Colour(d.panel);
    constexpr float x = zone::pad, w = zone::width - 2 * x;
    const bool meter_view = panel.meter_page && !(s.ui.page == PageId::Stage &&
        stage_page_now(s.ui,s.track(),false).kind == StageKind::PluginSound);
    p.fill(x, top + 2, w, 140, body, 5);
    p.ring(x, top + 2, w, 140, ink.withAlpha(.3f), 5);
    p.fill(x + 1, top + 3, 3, 28, ink, 1);
    const auto name = juce::String::fromUTF8(panel.name);
    p.text(name, x + 9, top + 3, w - 80, 22,
           p.fit(name, plain(colour::text, 17, Weight::Bold), w - 80, type::floor));
    if(!meter_view) p.text(d.subtitle, x + 9, top + 25, w - 18, 11, plain(ink, 8, Weight::Bold));
    p.text(panel.rows ? juce::String(panel.row + 1) + "/" + juce::String(panel.rows) : "LIVE",
           x + w - 65, top + 7, 56, 14, plain(ink, 10, Weight::Bold), juce::Justification::centredRight);
    const float gx = x + 6, gy = top + 41, gw = w - 12;
    if(meter_view) {
        draw_analog_output_meter(p,panel,gx,top+30,gw,106,bypass);
        return;
    }
    const bool fm_right = std::string_view(d.name) == "Sloop FM6" || std::string_view(d.name) == "Aphex Move";
    const bool preset_page = s.ui.page == PageId::Stage &&
        stage_page_now(s.ui,s.track(),false).kind == StageKind::PluginSound;
    const bool parameter_page = !expanded && !preset_page &&
        draw_plugin_parameter_page(p,s,gx,gy,gw,77,ink,bypass);
    if (parameter_page) {
        // The current bank owns its layout; instrument identity remains above.
    } else if (d.face == PluginFace::FM) {
        const float controls_x = fm_right ? gx + gw - 88 : gx;
        p.fill(controls_x, gy, 88, 77, juce::Colour(0xff090d12), 3);
        for (unsigned k = 0; k < 3; ++k) {
            const auto& param = panel.params[k];
            const float yy = gy + k * 25;
            const auto label = "K" + juce::String(k + 1) + " " + juce::String::fromUTF8(param.name);
            p.text(label, controls_x + 5, yy + 2, 78, 12, p.fit(label, plain(ink, 8, Weight::Bold), 78, type::floor));
            p.fill(controls_x + 5, yy + 18, 77, 2, ink.withAlpha(.15f));
            if (param.used) p.fill(controls_x + 5, yy + 18, 77 * std::clamp(param.value / float(plugin_param_max), 0.f, 1.f), 2, ink);
        }
        draw_workshop_waveform(p, panel, fm_right ? gx : gx + 93, gy, gw - 93, 77, bypass, ink);
    } else if (d.face == PluginFace::Analogue) {
        draw_workshop_waveform(p, panel, gx, gy, gw - 76, 77, bypass, ink);
        spectrum(p, panel, gx + gw - 71, gy, 71, 77, ink, bypass);
    } else if (d.face == PluginFace::Piano) {
        draw_workshop_waveform(p, panel, gx, gy, gw, 57, bypass, ink);
    } else if (d.face == PluginFace::Atmosphere) {
        draw_workshop_waveform(p, panel, gx, gy, gw, 48, bypass, ink);
        spectrum(p, panel, gx, gy + 51, gw * .42f, 26, ink, bypass);
        const auto preset = juce::String("STEREO / TEXTURE");
        p.text(preset.isEmpty() ? "STEREO / TEXTURE" : preset, gx + gw * .45f, gy + 54, gw * .55f, 21,
               p.fit(preset.isEmpty() ? "STEREO / TEXTURE" : preset, plain(ink, 10, Weight::Bold), gw * .55f, type::floor));
    } else if (d.face == PluginFace::Dynamics) {
        draw_workshop_waveform(p, panel, gx, gy, gw - 67, 77, bypass, ink);
        p.fill(gx + gw - 62, gy, 62, 77, juce::Colour(0xff090b10), 3);
        p.text("OUTPUT", gx + gw - 58, gy + 2, 54, 11, plain(ink, 8, Weight::Bold), juce::Justification::centred);
        for (unsigned side = 0; side < 2; ++side) {
            const auto& scope = side ? panel.scope_right : panel.scope;
            float peak = 0;
            if (!bypass) for (auto pair : scope) peak = std::max({peak, std::abs(int16_t(pair) / 32768.f), std::abs(int16_t(pair >> 16) / 32768.f)});
            const float db = peak > .00001f ? 20.f * std::log10(peak) : -100.f;
            const float level = std::clamp((db + 60) / 60, 0.f, 1.f);
            for (int segment = 0; segment < 12; ++segment) {
                const auto colour = segment >= 11 ? juce::Colour(0xffff6464) : segment >= 9 ? juce::Colour(0xffffcd70) : ink;
                p.fill(gx + gw - 52 + side * 25, gy + 62 - segment * 4, 18, 3,
                       colour.withAlpha(segment < level * 12 ? 1.f : .12f), .5f);
            }
            p.text(side ? "R" : "L", gx + gw - 52 + side * 25, gy + 65, 18, 11, plain(ink, 8, Weight::Bold), juce::Justification::centred);
        }
    } else if (d.face == PluginFace::Tape) {
        const bool audible = std::any_of(panel.scope.begin(), panel.scope.end(), [](uint32_t pair) { return pair != 0; }) ||
                             std::any_of(panel.scope_right.begin(), panel.scope_right.end(), [](uint32_t pair) { return pair != 0; });
        draw_second_spin_vintage(p, panel, gx, gy, gw, 77, s.now, s.playing || audible, bypass, expanded);
    } else if (d.face == PluginFace::Vocal) {
        draw_workshop_waveform(p, panel, gx, gy, gw - 86, 77, bypass, ink);
        p.fill(gx + gw - 81, gy, 81, 77, juce::Colour(0xff0b141a), 3);
        p.text("PITCH", gx + gw - 77, gy + 5, 73, 13, plain(ink, 9, Weight::Bold), juce::Justification::centred);
        const auto cents = panel.vocal_tune_ready && !bypass ? juce::String(panel.vocal_cents) : juce::String("--");
        p.text(cents, gx + gw - 77, gy + 24, 73, 26, plain(colour::text, 21, Weight::Bold), juce::Justification::centred);
        p.text("CENTS", gx + gw - 77, gy + 56, 73, 12, plain(ink, 8, Weight::Bold), juce::Justification::centred);
    } else {
        // Rhythm tools put the spectral energy above the transient waveform.
        spectrum(p, panel, gx, gy, gw, 29, ink, bypass);
        draw_workshop_waveform(p, panel, gx, gy + 32, gw, 45, bypass, ink);
    }
    if (!parameter_page && d.face == PluginFace::Piano) {
        unsigned held = 0;
        for (int key : s.held_keys) if (key >= 0) held |= 1u << (key % 12);
        constexpr int notes[] {0,2,4,5,7,9,11};
        constexpr int sharps[] {1,3,-1,6,8,10,-1};
        const float kw = gw / 14, ky = top + 102;
        for (int k = 0; k < 14; ++k) p.fill(gx + k * kw, ky, kw - 1, 17,
            held & (1u << notes[k % 7]) ? ink : juce::Colour(0xffe8dfce), 1);
        for (int k = 0; k < 14; ++k) if (sharps[k % 7] >= 0)
            p.fill(gx + (k + .7f) * kw, ky, kw * .55f, 10,
                   held & (1u << sharps[k % 7]) ? ink : juce::Colour(0xff141416), 1);
    }
    auto preset = juce::String::fromUTF8(panel.preset_name);
    const bool browsing = s.ui.page == PageId::Stage &&
        stage_page_now(s.ui, s.track(), false).kind == StageKind::PluginSound;
    if (s.ui.preset_due && s.ui.presets_entry == panel.entry &&
        s.presets.entry == panel.entry && s.ui.presets_cursor < s.presets.list.size())
        preset = juce::String::fromUTF8(s.presets.list[s.ui.presets_cursor].name.c_str());
    const auto footer = expanded ? juce::String("RELEASE SHIFT  CONTROLS")
                    : browsing && preset.isEmpty() ? juce::String("DATA  PRESETS")
                    : preset.isEmpty() ? juce::String("SHIFT  LIVE AUDIO") : preset;
    p.text(footer, gx + 2, top + 123, gw - 4, 13,
           p.fit(footer, plain(ink, 9, Weight::Bold), gw - 4, type::floor));
}
} // namespace melodee
