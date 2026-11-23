// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Tape Workshop contributors
#pragma once
#include "melodee/plugin/plugin_panel.h"
#include "melodee/ui/paint.h"
#include <algorithm>
#include <cmath>

namespace melodee {
inline void draw_plugin_output(ui::Paint& p, const PluginPanel& panel, float x, float y, float width) {
    using namespace ui;
    p.fill(x, y, width, 28, juce::Colour(0xff080910), 3);
    for (unsigned channel = 0; channel < 2; ++channel) {
        const auto& scope = channel ? panel.scope_right : panel.scope;
        const auto ink = juce::Colour(channel ? 0xff67dfdb : 0xffc49aff);
        const float mid = y + 7 + channel * 14, left = x + 13, span = width - 68;
        p.fill(left, mid, span, .5f, colour::line);
        float peak = 0;
        for (unsigned n = 0; n < scope.size(); ++n) {
            const auto pair = scope[n];
            const float high = float(pair >> 16) / 32767.f;
            const int bottom = int(pair & 65535);
            const float low = (bottom >= 32768 ? bottom - 65536 : bottom) / 32767.f;
            peak = std::max({peak, high, -low});
            const float above = std::sqrt(std::clamp(high, 0.f, 1.f)) * 5.8f;
            const float below = std::sqrt(std::clamp(-low, 0.f, 1.f)) * 5.8f;
            if (above + below > .05f)
                p.fill(left + n * span / scope.size(), mid - above, std::max(1.f, span / scope.size() - .5f), above + below, ink);
        }
        p.text(channel ? "R" : "L", x + 2, mid - 6, 10, 12, plain(ink, 9, Weight::Bold));
        const auto db = peak > .00001f ? juce::String(20.f * std::log10(peak), 1) : juce::String("-inf");
        p.text(db + " dB", x + width - 54, mid - 6, 51, 12, plain(ink, 9, Weight::Bold), juce::Justification::centredRight);
    }
}

// A portable host editor: every value and label comes from the loaded plug-in.
// Drawing only; the existing Melodee bindings still own K1-K3, pickup and undo.
inline void draw_plugin_controls(ui::Paint& p, const PluginPanel& panel, float top, unsigned track = 0) {
    using namespace ui;
    constexpr float x = zone::pad, width = zone::width - 2 * x;
    const auto accent = juce::Colour(0xffc49aff);
    const auto dark = juce::Colour(0xff111018);
    p.fill(x, top + 1, width, 140, dark, 4);
    const auto name = juce::String::fromUTF8(panel.name);
    p.text(name, x + 7, top + 3, width - 88, 20,
           p.fit(name, plain(colour::text, 15, Weight::Bold), width - 88, type::floor));
    p.text(juce::String(panel.row + 1) + " / " + juce::String(panel.rows),
           x + width - 76, top + 4, 68, 18, caps_text(accent), juce::Justification::centredRight);
    if (panel.live_analyzer) draw_plugin_output(p, panel, x + 4, top + 26, width - 8);
    const bool live = panel.live_analyzer;
    constexpr float gap = 4, cw = (width - 16) / 3;
    for (unsigned k = 0; k < panel.params.size(); ++k) {
        const float left = x + 4 + k * (cw + gap), y = top + (live ? 58 : 27);
        const auto& control = panel.params[k];
        const auto ink = knob_shade(track, k);
        p.fill(left, y, cw, live ? 66 : 93, colour::panel, 3);
        if (!control.used) {
            p.text("UNUSED", left, y + (live ? 25 : 38), cw, 15, caps_text(colour::dim), juce::Justification::centred);
            continue;
        }
        const auto label = juce::String::fromUTF8(control.name);
        auto style = plain(colour::text, live ? 9 : 10, Weight::Bold);
        const auto lines = p.wrap(label, style, cw - 8, 2);
        for (int line = 0; line < lines.size(); ++line)
            p.text(lines[line], left + 4, y + 3 + line * 10, cw - 8, 11, style, juce::Justification::centred);
        const float value = std::clamp(control.value / float(plugin_param_max), 0.f, 1.f);
        const float cx = left + cw / 2, cy = y + (live ? 35 : 49), radius = live ? 11 : 18;
        constexpr float begin = -2.35f, end = 2.35f;
        juce::Path track, filled;
        track.addCentredArc(cx, cy, radius, radius, 0, begin, end, true);
        filled.addCentredArc(cx, cy, radius, radius, 0, begin, begin + value * (end - begin), true);
        p.g.setColour(colour::line);
        p.g.strokePath(track, juce::PathStrokeType(live ? 2 : 3));
        p.g.setColour(ink);
        p.g.strokePath(filled, juce::PathStrokeType(live ? 2 : 3));
        p.circle(cx, cy, radius - (live ? 3 : 5), dark);
        const float angle = begin + value * (end - begin);
        p.g.setColour(ink);
        p.g.drawLine(cx, cy, cx + std::sin(angle) * (live ? 7 : 10), cy - std::cos(angle) * (live ? 7 : 10), 2);
        const auto text = juce::String::fromUTF8(control.text);
        p.text(text, left + 3, y + (live ? 48 : 72), cw - 6, 17,
               p.fit(text, plain(ink, live ? 11 : 12, Weight::Bold), cw - 6, type::floor), juce::Justification::centred);
    }
    p.text("K1-K3: EDIT   -/+: PAGE", x + 5, top + 126, width - 10, 14,
           plain(colour::sec, 9, Weight::Medium), juce::Justification::centred);
}
} // namespace melodee
