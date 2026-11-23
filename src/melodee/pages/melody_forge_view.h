// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Tape Workshop contributors
#pragma once
#include "melodee/pages/common.h"
#include "melodee/ui/widgets.h"
#include <cmath>

namespace melodee {
inline void melody_forge_view(ui::Paint &p, const Snapshot &s, float top) {
    using namespace ui;
    const auto &panel = s.ui.plugin_panel;
    const auto &v = panel.forge_values;
    constexpr float x = 6, w = 308;
    const juce::Colour cyan(0xff50e6ed), violet(0xffbb8bed), amber(0xfff1c16b), mint(0xff70e8b0);
    const juce::Colour colours[] = {cyan, violet, amber, mint};
    static const char *sounds[] = {
        "Off",   "Felt Piano", "Grand Piano", "Electric Piano", "Violin",   "Cello", "Trumpet",  "Soft Brass",
        "Flute", "Choir",      "Glass Bell",  "Warm Pad",       "Saw Lead", "Pluck", "Sub Bass", "Organ"};
    static const char *pages[] = {
        "TONE / SPACE",     "PLAY / TEMPO", "STYLE / SCALE",  "RHYTHM",  "VARIATIONS", "AMP ENVELOPE",
        "RELEASE / FILTER", "EFFECTS",      "DELAY / OUTPUT", "A SOUND", "A SPACE",    "A SHAPE",
        "B SOUND",          "B SPACE",      "B SHAPE",        "C SOUND", "C SPACE",    "C SHAPE",
        "D SOUND",          "D SPACE",      "D SHAPE"};
    p.fill(x, top, w, zone::knobs_y - top - 2, juce::Colour(0xff111924), 4);
    p.text("MELODY FORGE", x + 5, top + 2, 185, 19, plain(cyan, 15, Weight::Bold));
    p.text(juce::String(v[4] > .5f ? s.tempo / 10. : 40 + 200 * v[5], 1) + " BPM", x + 204, top + 3, 98, 16,
           caps_text(colour::text), juce::Justification::centredRight);
    juce::String preset = panel.preset_name[0] ? juce::String(panel.preset_name) : juce::String("DATA: PRESETS");
    auto name_style = plain(colour::text, 12, Weight::Medium);
    p.text(preset, x + 5, top + 22, w - 10, 16, p.fit(preset, name_style, w - 10, type::floor));
    const int row = std::min<int>(panel.row, 20), selected = row >= 9 ? (row - 9) / 3 : -1;
    for (int layer = 0; layer < 4; ++layer) {
        const int base = 27 + layer * 9;
        const float y = top + 43 + layer * 21;
        auto ink = colours[layer];
        p.fill(x + 4, y, 143, 19, selected == layer ? ink.withAlpha(.22f) : juce::Colour(0xff202a37), 3);
        p.text(juce::String::charToString('A' + layer), x + 7, y, 13, 18, plain(ink, 12, Weight::Bold));
        const auto label = juce::String(sounds[std::clamp(int(std::lround(v[base] * 15)), 0, 15)]);
        p.text(label, x + 22, y, 119, 13,
               plain(v[base + 1] > .001f ? colour::text : colour::dim, 11, Weight::Medium));
        p.fill(x + 23, y + 15, 116, 2, colour::line, 1);
        p.fill(x + 23, y + 15, 116 * v[base + 1], 2, ink, 1);
    }
    constexpr float gx = x + 154, gw = 148;
    p.text("LIVE OUTPUT", gx + 3, top + 40, gw - 6, 13, caps_text(cyan, 10));
    const float sy = top + 55, sh = 29, mid = sy + sh / 2;
    p.fill(gx, sy, gw, sh, colour::bg, 2);
    p.fill(gx, mid, gw, 1, colour::line);
    for (unsigned n = 0; n < panel.scope.size(); ++n) {
        auto sample = panel.scope[n];
        float low = int16_t(sample & 65535) / 32768.f, high = int16_t(sample >> 16) / 32768.f;
        float y = mid - high * sh * .48f;
        float height = std::max(1.f, (high - low) * sh * .48f);
        p.fill(gx + n * gw / panel.scope.size(), y, gw / panel.scope.size(), height, cyan);
    }
    const bool envelope = row == 5 || row == 6 || (row >= 9 && (row - 9) % 3 == 2);
    p.text(envelope ? "ENVELOPE" : "FILTER RESPONSE", gx + 3, top + 85, gw - 6, 12,
           caps_text(envelope ? amber : violet, 10));
    const float py = top + 100, ph = 25;
    p.fill(gx, py, gw, ph, colour::bg, 2);
    juce::Path curve;
    for (int n = 0; n < 100; ++n) {
        float u = n / 99.f, value;
        if (envelope) {
            float attack = selected >= 0 ? (v[15] + v[27 + selected * 9 + 7]) * .5f : v[15];
            float release = selected >= 0 ? (v[18] + v[27 + selected * 9 + 8]) * .5f : v[18];
            float a = .05f + attack * .32f, d = .08f + v[16] * .25f, sustain = v[17],
                  r = .1f + release * .35f;
            value = u < a       ? u / a
                    : u < a + d ? 1 - (1 - sustain) * (u - a) / d
                    : u < 1 - r ? sustain
                                : sustain * (1 - u) / r;
        } else {
            float layer_tone = selected >= 0 ? v[27 + selected * 9 + 6] : .7f;
            float cutoff = .08f + (v[19] * .4f + v[0] * .25f + layer_tone * .35f) * .84f;
            value = 1 / (1 + std::pow(u / std::max(.02f, cutoff), 6.f));
            value = std::min(1.2f, value + .4f * v[20] * std::exp(-std::pow((u - cutoff) / .045f, 2.f)));
        }
        float y = py + ph - 2 - std::clamp(value, 0.f, 1.2f) * (ph - 4) / 1.2f;
        if (n == 0)
            curve.startNewSubPath(gx, y);
        else
            curve.lineTo(gx + u * gw, y);
    }
    p.g.setColour(envelope ? amber : violet);
    p.g.strokePath(curve, juce::PathStrokeType(1.5f));
    p.text(juce::String(pages[row]) + "  " + juce::String(row + 1) + "/21", x + 5, top + 129, w - 10, 12,
           caps_text(colour::mid, 10));
}
} // namespace melodee
