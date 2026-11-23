// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Tape Workshop contributors
#pragma once
#include "melodee/plugin/plugin_panel.h"
#include "melodee/ui/paint.h"
#include <algorithm>
#include <cmath>
#include <string_view>

namespace melodee {
inline bool keep_workshop_plugin_ui(const PluginPanel& panel) {
    std::string_view name(panel.name);
    if (name.size() > 5 && name.substr(name.size() - 5) == " (TW)") name.remove_suffix(5);
    return name == "Sub Foundry" || name == "Contour Comp" || name == "Prism EQ";
}

// Draw only the existing, bounded output snapshots. No generated motion or
// parameter-dependent scaling: a silent plug-in stays flat and gain stays honest.
inline void draw_workshop_waveform(ui::Paint& p, const PluginPanel& panel,
                                   float x, float y, float w, float h, bool bypass = false,
                                   juce::Colour accent = juce::Colour(0xffc49aff)) {
    using namespace ui;
    if (w < 70 || h < 20) return;
    juce::Graphics::ScopedSaveState saved(p.g);
    p.g.reduceClipRegion(juce::Rectangle<float>(x, y, w, h).getSmallestIntegerContainer());
    const auto purple = accent, cyan = accent.withRotatedHue(.13f);
    const auto grid = juce::Colour(0xff282331), white = juce::Colour(0xfff5f2ff);
    p.fill(x, y, w, h, juce::Colour(0xff08080c), 3);
    p.ring(x, y, w, h, grid, 3);
    const bool labelled = h >= 48;
    const float header = labelled ? 15.f : 2.f;
    const float left = x + 13, span = w - 70, lane = (h - header - 3) / 2;
    if (labelled) {
        p.text(bypass ? "BYPASSED" : w < 220 ? "WAVE" : "AUDIO WAVEFORM", x + 6, y + 1, w - 55, 12,
               plain(white, 9, Weight::Bold));
        p.text("dBFS", x + w - 47, y + 1, 41, 12, plain(colour::sec, 8, Weight::Bold),
               juce::Justification::centredRight);
    }
    for (unsigned side = 0; side < 2; ++side) {
        const auto ink = side ? cyan : purple;
        const auto& samples = side ? panel.scope_right : panel.scope;
        const float top = y + header + side * lane, mid = top + lane / 2;
        const float amplitude = std::max(1.f, lane / 2 - 2);
        for (int n = 0; n <= 8; ++n) p.fill(left + span * n / 8, top, .5f, lane - 1, grid);
        p.fill(left, mid, span, .6f, ink.withAlpha(.28f));
        p.text(side ? "R" : "L", x + 3, mid - 5, 9, 10, plain(ink, 8, Weight::Bold));
        float peak = 0;
        juce::Path upper, lower, area;
        area.startNewSubPath(left, mid);
        for (unsigned n = 0; n < samples.size(); ++n) {
            const uint32_t pair = bypass ? 0u : samples[n];
            const float lo = int16_t(pair & 0xffffu) / 32768.f;
            const float hi = int16_t(pair >> 16) / 32768.f;
            peak = std::max({peak, std::abs(lo), std::abs(hi)});
            const float px = left + span * n / float(samples.size() - 1);
            const float high = mid - hi * amplitude, low = mid - lo * amplitude;
            if (n == 0) { upper.startNewSubPath(px, high); lower.startNewSubPath(px, low); }
            else { upper.lineTo(px, high); lower.lineTo(px, low); }
            area.lineTo(px, high);
        }
        for (unsigned n = unsigned(samples.size()); n-- > 0;) {
            const float lo = bypass ? 0.f : int16_t(samples[n] & 0xffffu) / 32768.f;
            area.lineTo(left + span * n / float(samples.size() - 1), mid - lo * amplitude);
        }
        area.closeSubPath();
        p.g.setColour(ink.withAlpha(.24f)); p.g.fillPath(area);
        p.g.setColour(ink); p.g.strokePath(upper, juce::PathStrokeType(1.f));
        p.g.setColour(ink.withAlpha(.72f)); p.g.strokePath(lower, juce::PathStrokeType(1.f));
        const float db = peak > .00001f ? 20.f * std::log10(peak) : -100.f;
        const auto level = peak > .00001f ? juce::String(db, 1) : juce::String("-inf");
        const auto meter = db >= -.1f ? juce::Colour(0xffff626c) : db > -6 ? juce::Colour(0xffffc66d) : cyan;
        p.text(level, x + w - 49, mid - (h < 36 ? 5.5f : 9.f), 43, h < 36 ? 11.f : 12.f, plain(white, 9, Weight::Bold), juce::Justification::centredRight);
        if (h >= 36) {
            p.fill(x + w - 49, mid + 5, 43, 3, grid, 1);
            p.fill(x + w - 49, mid + 5, 43 * std::clamp((db + 60) / 60, 0.f, 1.f), 3, meter, 1);
        }
    }
}
} // namespace melodee
