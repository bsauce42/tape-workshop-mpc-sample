// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
// The effects as Stage's FX page and the FX Select pages draw them alike: a
// chain's six slots as a strip, and under it the effect in hand drawn as what
// it does to sound, from its own values (pages/common.h).
#include "melodee/pages/common.h"
#include "melodee/pages/plugin_pro_ui.h"
#include "melodee/pages/second_spin_vintage.h"
#include "melodee/pages/plugin_parameter_pages.h"
#include "melodee/shell/mix_suite.h"
#include "melodee/pages/prism_colours.h"
#include "melodee/plugin/contour_transfer.h"
#include "melodee/plugin/gate_curve.h"
#include "melodee/plugin/prism_response.h"
#include "melodee/plugin/plugin_skin.h"
#include "melodee/ui/widgets.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>

namespace melodee {
using namespace ui;

namespace {
void break26_display(Paint& p, const Snapshot& s, float x, float y, float w, bool bypass) {
    const auto& panel = s.ui.plugin_panel;
    const auto ink = juce::Colour(0xfff3b15c);
    const auto dim = juce::Colour(0xff835b33);
    p.fill(x, y, w, 46, juce::Colour(0xff241d18), 4);
    p.fill(x + 7, y + 7, 73, 32, juce::Colour(0xff3b2b20), 3);
    p.text("BREAK", x + 12, y + 7, 63, 14, caps_text(ink), juce::Justification::centred);
    p.text("26", x + 12, y + 20, 63, 17, plain(ink, type::list, Weight::SemiBold), juce::Justification::centred);

    const float left = x + 89, width = w - 97, mid = y + 23;
    p.fill(left, y + 6, width, 34, juce::Colour(0xff191513), 2);
    for (int n = 1; n < 8; ++n)
        p.fill(left + width * n / 8, y + 7, 1, 32, juce::Colour(0xff35291e));
    p.fill(left + 1, mid, width - 2, 1, juce::Colour(0xff483523));

    // A sample-and-hold illustration driven by the active knob bank.
    auto fraction = [&](int k) { return std::clamp(panel.params[k].value / float(plugin_param_max), 0.f, 1.f); };
    const bool main = panel.row == 0;
    const int steps = main ? 8 + int(32 * fraction(0)) : 26;
    const float levels = main ? 3 + 22 * fraction(1) : 15;
    const float detail = main ? fraction(2) : fraction(0);
    const float amplitude = main ? 12.f : (3 + 10 * fraction(1)) * (.25f + .75f * fraction(2));
    const bool moving = s.playing && !bypass;
    const float scroll = moving ? float((s.now / 65) % 48) / 48 : 0;
    juce::Path wave;
    float previous = mid;
    for (int n = 0; n <= steps; ++n) {
        const float phase = 6.2831853f * (2 * float(n) / steps + scroll);
        const float sample = std::sin(phase) * .72f + std::sin(phase * 3) * (.08f + .20f * detail);
        const float py = mid - amplitude * std::round(sample * levels) / levels;
        const float px = left + 2 + (width - 4) * n / steps;
        if (n == 0) wave.startNewSubPath(px, py);
        else { wave.lineTo(px, previous); wave.lineTo(px, py); }
        previous = py;
    }
    p.g.setColour(bypass ? dim : ink);
    p.g.strokePath(wave, juce::PathStrokeType(1.45f));
}

void compressor_history(Paint& p, const PluginPanel& panel, float x, float y, float w, float h, bool bypass) {
    const auto input = juce::Colour(0xff84919e);
    const auto output = juce::Colour(0xffe7c88f);
    const auto reduction = juce::Colour(0xfff77369);
    p.fill(x, y, w, h, juce::Colour(0xff161a1e), 3);
    p.text("IN", x + 5, y + 1, 24, 12, caps_text(input));
    p.text("OUT", x + 30, y + 1, 32, 12, caps_text(output));
    p.text("GR", x + 67, y + 1, 25, 12, caps_text(reduction));
    if (bypass) p.text("BYPASS", x + w - 70, y + 1, 62, 12, caps_text(colour::mid), juce::Justification::centredRight);

    const float top = y + 14, bottom = y + h - 3, mid = (top + bottom) * .5f;
    p.fill(x + 5, mid, w - 10, 1, colour::line);
    juce::Path gain;
    for (unsigned i = 0; i < panel.compressor_history.size(); ++i) {
        const auto value = bypass ? 0u : panel.compressor_history[i];
        const float px = x + 5 + (w - 11) * i / (panel.compressor_history.size() - 1);
        const float in = std::sqrt((value & 1023u) / 1023.f) * (mid - top);
        const float out = std::sqrt(((value >> 10) & 1023u) / 1023.f) * (mid - top);
        if (in > 0) p.fill(px, mid - in, 2.5f, 2 * in, input.withAlpha(.65f));
        if (out > 0) p.fill(px, mid - out, 1.4f, 2 * out, output);
        const float gr = ((value >> 20) & 1023u) / 1023.f;
        const float gy = top + gr * (bottom - top);
        if (i == 0) gain.startNewSubPath(px, gy); else gain.lineTo(px, gy);
    }
    p.g.setColour(reduction.withAlpha(bypass ? .25f : 1.f));
    p.g.strokePath(gain, juce::PathStrokeType(1.2f));
}

void contour_curve(Paint& p, const PluginPanel& panel, float x, float y, float w, float h, bool bypass) {
    const auto lime = juce::Colour(0xffc7db62);
    p.fill(x, y, w, h, juce::Colour(0xff161a1e), 3);
    const auto& settings = panel.contour_curve;
    const float left = x + 6, top = y + 4, width = w - 12, height = h - 8;
    for (int i = 1; i < 4; ++i) {
        p.fill(left + i * width / 4, top, 1, height, colour::line);
        p.fill(left, top + i * height / 4, width, 1, colour::line);
    }
    const float knee_x = left + width * (settings[0] + 60) / 60;
    p.fill(knee_x, top, 1, height, lime.withAlpha(.35f));
    juce::Path curve;
    for (int i = 0; i <= 60; ++i) {
        const float db = float(i - 60);
        const float gr = bypass ? 0 : contour_reduction(db, settings[0], settings[1], settings[2], settings[3], settings[4]);
        const float px = left + width * i / 60;
        const float py = top + height * std::clamp((gr - db) / 60, 0.f, 1.f);
        if (i == 0) curve.startNewSubPath(px, py); else curve.lineTo(px, py);
    }
    p.g.setColour(lime);
    p.g.strokePath(curve, juce::PathStrokeType(1.6f));
    p.text("TRANSFER", x + 7, y + 4, 68, 13, caps_text(lime));
}

void rack76_display(Paint& p, const PluginPanel& panel, float x, float y, float w, bool bypass) {
    const auto face = juce::Colour(panel.rack76_blue ? 0xff2f6378 : 0xff303437);
    const auto metal = juce::Colour(0xffaab0ac);
    const auto paper = juce::Colour(0xffe5c996);
    const auto ink = juce::Colour(0xff302319);
    const auto red = juce::Colour(0xff9b3824);
    p.fill(x, y, w, 53, face, 3);
    p.ring(x, y, w, 53, metal.withAlpha(.55f), 3);
    const float left = x + w * .26f, meter_w = w * .50f;
    p.fill(left, y + 4, meter_w, 45, juce::Colour(0xff191915), 3);
    p.fill(left + 4, y + 8, meter_w - 8, 36, paper, 2);
    const float cx = left + meter_w / 2, cy = y + 56;
    const auto angle = [](float t) { return -1.f + t * 2.f; };
    const auto polar = [&](float a, float r) { return juce::Point<float>(cx + std::sin(a) * r, cy - std::cos(a) * r); };
    juce::Path scale;
    scale.addCentredArc(cx, cy, 38, 38, 0, angle(0), angle(1), true);
    p.g.setColour(ink);
    p.g.strokePath(scale, juce::PathStrokeType(1));
    for (int n = 0; n <= 8; ++n) {
        const float a = angle(n / 8.f);
        p.g.setColour(n >= 7 && panel.rack76_meter ? red : ink);
        p.g.drawLine(juce::Line<float>(polar(a, 38), polar(a, 43)), 1.f);
    }
    const auto meter_text = plain(ink, type::small, Weight::SemiBold);
    p.text(panel.rack76_meter == 0 ? "-24" : "-42", left + 9, y + 15, 28, 13, meter_text);
    p.text(panel.rack76_meter == 0 ? "0" : "+3", left + meter_w - 30, y + 15, 21, 13, meter_text, juce::Justification::centredRight);
    const float needle = bypass ? (panel.rack76_meter == 0 ? 1.f : 0.f) : panel.rack76_needle;
    const float a = angle(needle);
    p.g.setColour(red);
    p.g.drawLine(juce::Line<float>(polar(a, 18), polar(a, 42)), 1.6f);
    static constexpr const char* ratios[] = {"4:1", "8:1", "12:1", "20:1", "ALL"};
    static constexpr const char* meters[] = {"GR", "IN", "OUT"};
    p.text(ratios[panel.rack76_ratio], x + 8, y + 7, w * .23f, 18, plain(paper, type::list, Weight::SemiBold));
    p.text(panel.rack76_blue ? "BLUE STRIPE" : "BLACKFACE", x + 8, y + 30,
           w * .23f, 14, plain(metal, type::small, Weight::Medium));
    p.text(meters[panel.rack76_meter], left + meter_w + 7, y + 7, w * .20f, 17, caps_text(paper));
    p.text(bypass ? "BYPASS" : panel.rack76_off ? "COMP OFF" : "COMP ON", left + meter_w + 7, y + 30,
           w * .20f, 14, plain(metal, type::small, Weight::Medium));
}

juce::Colour prism_overlap_colour(float relief, bool quiet = false) {
    const auto red = juce::Colour(0xffff5860);
    const auto amber = juce::Colour(0xffffbd64);
    const auto green = juce::Colour(0xff75e6a0);
    return quiet ? colour::mid : relief < .5f ? red.interpolatedWith(amber, relief * 2.f) :
                                               amber.interpolatedWith(green, relief * 2.f - 1.f);
}

juce::String prism_overlap_source(const Snapshot& s, int source, int reference_pad) {
    if (reference_pad >= 0) {
        const unsigned bank = s.ui.page == PageId::PadFx ? (s.track().last_pad % sampler_pads) / MixSpectrum::pad_count :
                                                           s.track().bank % sampler_banks;
        const auto pad_label = [&](unsigned pad) {
            return juce::String::charToString(juce::juce_wchar('A' + bank)) + juce::String(pad + 1).paddedLeft('0', 2);
        };
        return "Pads " + pad_label(unsigned(reference_pad)) + " / " + pad_label(unsigned(source) - track_count);
    }
    if (unsigned(source) < track_count)
        return "T" + juce::String(source + 1) + " " + juce::String::fromUTF8(s.project.tracks[size_t(source)].name);
    return {};
}

juce::String prism_overlap_frequency(unsigned bin) {
    const double hz = eq_hz((bin + .5) / prism_spectrum_bins);
    return hz < 1000.0 ? juce::String(int(std::lround(hz))) + " Hz" : juce::String(hz / 1000.0, 1) + " kHz";
}

void prism_overlap_list(Paint& p, const Snapshot& s, float x, float y, float w, float h) {
    constexpr const char* names[] {"LOWS", "MIDS", "HIGHS"};
    const float group_h = h / 3.f;
    for (unsigned band = 0; band < s.prism_overlaps.bands.size(); ++band) {
        const float top = y + band * group_h;
        p.fill(x, top, w, group_h - 2, juce::Colour(0xff111a25), 3);
        p.text(names[band], x + 5, top, 44, group_h - 2, caps_text(prism_band_colour(band + 1)));
        const auto& rows = s.prism_overlaps.bands[band];
        if (rows[0].source < 0) {
            p.text("No overlap", x + 52, top, w - 56, group_h - 2, plain(colour::mid, type::small));
            continue;
        }
        const float row_h = (group_h - 2) / 2.f;
        for (unsigned i = 0; i < rows.size(); ++i) {
            const auto& row = rows[i];
            if (row.source < 0) break;
            const auto ink = prism_overlap_colour(row.relief);
            const auto style = plain(ink, type::small);
            const auto source = prism_overlap_source(s, row.source, row.reference_pad);
            const auto frequency = prism_overlap_frequency(row.bin);
            const float text_y = rows[1].source < 0 ? top + row_h * .5f : top + i * row_h;
            p.text(source, x + 52, text_y, w - 108, row_h, p.fit(source, style, w - 108, type::floor));
            p.text(frequency, x + w - 58, text_y, 53, row_h, style, juce::Justification::centredRight);
        }
    }
}

void prism_graph(Paint& p, const Snapshot& s, float x, float y, float w, float height, bool bypass) {
    const auto& panel = s.ui.plugin_panel;
    const auto& overlap = s.mix_overlap;
    const bool tagged = !bypass && overlap.source >= 0 && overlap.strength > .09f;
    const bool fixed = panel.rows == 5;
    const unsigned bands = fixed ? 5 : 4;
    const unsigned selected = prism_selected_band(panel);
    p.fill(x, y, w, height, juce::Colour(0xff111a25), 4);
    const float mid = y + height / 2;
    p.fill(x + 1, mid, w - 2, 1, juce::Colour(0xff3b4b62));
    for (float hz : {50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f})
        p.fill(x + w * std::log10(hz / 20) / 3, y + 2, 1, height - 4, juce::Colour(0xff263448));
    if (!bypass) {
        const float bottom = y + height - 3;
        for (unsigned i = 0; i < panel.eq_spectrum.size(); ++i) {
            const float pitch = (i + .5f) / panel.eq_spectrum.size();
            const float bar = panel.eq_spectrum[i] * (height - 15);
            const auto tint = prism_band_colour(pitch < .366f ? 1 : pitch < .667f ? 2 : 3);
            if (bar > .5f)
                p.fill(x + 2 + (w - 4) * i / panel.eq_spectrum.size(), bottom - bar,
                       (w - 4) / panel.eq_spectrum.size() - .5f, bar, tint.withAlpha(.45f), 1);
        }
    }
    if (tagged) {
        const float step = (w - 4) / float(overlap.glow.size());
        const float relief = overlap.relief;
        const auto ink = prism_overlap_colour(relief, overlap.quiet);
        for (unsigned i = 0; i < overlap.glow.size(); ++i) {
            if (overlap.glow[i] < .07f) continue;
            const float strength = std::min(1.f, overlap.glow[i] * 2.f);
            p.fill(x + 2 + step * i, y + 13, step + 1, height - 15, ink.withAlpha(.12f + strength * .31f));
        }

    }
    // Stable scale marks replace the changing source-name banner. Source details
    // remain in the deliberate Shift overlap list, never over the main EQ graph.
    p.text("0", x + w - 14, mid - 11, 11, 10, plain(juce::Colour(0xffaeb9cb), 8));
    const auto legend = [&](unsigned band) {
        return plain(prism_band_colour(band).withAlpha(selected == band ? 1.f : .7f),
                     type::small, selected == band ? Weight::SemiBold : Weight::Medium);
    };
    p.text("Lows", x + 5, y + 1, 50, 12, legend(1));
    p.text("Mids", x + w / 2 - 25, y + 1, 50, 12, legend(2), juce::Justification::centred);
    p.text("Highs", x + w - 55, y + 1, 50, 12, legend(3), juce::Justification::centredRight);
    const auto level_y = [&](double db) {
        return mid - float(std::clamp(db, -24.0, 24.0)) * (height / 2 - 3) / 24;
    };
    const auto& active = panel.eq_bands[selected];
    const int shape = int(std::lround(active[EQ_SHAPE] * 8));
    const double range = active[EQ_RANGE] * 36 - 18;
    if (!fixed && !bypass && active[EQ_ENABLE] >= .5 && (shape <= EQ_HIGH_SHELF || shape == EQ_TILT) && std::abs(range) > .01) {
        const auto boundary = eq_curve(active.data(), range);
        juce::Path limit;
        for (int i = 0; i <= 100; ++i) {
            const double u = i / 100.0;
            const float px = x + 2 + float(u) * (w - 4);
            const float py = level_y(eq_magnitude(&boundary, eq_hz(u)));
            if (i == 0) limit.startNewSubPath(px, py); else limit.lineTo(px, py);
        }
        p.g.setColour(prism_band_colour(selected).withAlpha(.35f));
        p.g.strokePath(limit, juce::PathStrokeType(1.3f));
    }
    for (unsigned n = 0; n < bands; ++n) {
        const unsigned band = (selected + n + 1) % bands;
        const auto& values = panel.eq_bands[band];
        const auto curve = fixed ? eq_fixed_curve(band, values[0], values[1], values[2]) : eq_curve(values.data(), 0);
        const auto ink = prism_band_colour(band);
        if (curve.count) {
            juce::Path line;
            for (int i = 0; i <= 100; ++i) {
                const double u = i / 100.0;
                const float px = x + 2 + float(u) * (w - 4);
                const float py = level_y(eq_magnitude(&curve, eq_hz(u)));
                if (i == 0) line.startNewSubPath(px, py); else line.lineTo(px, py);
            }
            p.g.setColour(ink.withAlpha(bypass ? .20f : band == selected ? 1.f : .55f));
            p.g.strokePath(line, juce::PathStrokeType(band == selected ? 2.f : 1.f));
        }
        const float cx = x + 4 + float(values[EQ_FREQ]) * (w - 8);
        const float cy = level_y(eq_magnitude(&curve, eq_hz(values[EQ_FREQ])));
        p.circle(cx, cy, band == selected ? 4.f : 2.7f, ink.withAlpha(curve.count && !bypass ? 1.f : .32f));
    }
}

// A parameter of the slot found by its label: its value, and its place over its range (0..1).
int raw(const EffectSlot& slot, const EffectInfo& info, const char* label, int fallback) {
    for (unsigned n = 0; n < effect_params; ++n)
        if (info.params[n].label && !std::strcmp(info.params[n].label, label)) return slot.params[n];
    return fallback;
}
float norm(const EffectSlot& slot, const EffectInfo& info, const char* label, float fallback) {
    for (unsigned n = 0; n < effect_params; ++n)
        if (info.params[n].label && !std::strcmp(info.params[n].label, label)) {
            const auto& pi = info.params[n];
            return std::clamp(float(slot.params[n] - pi.min) / float(std::max(1, pi.max - pi.min)), 0.0f, 1.0f);
        }
    return fallback;
}

// A curve y = f(u), u 0..1 across x..x+w, as a path.
juce::Path curve(const std::function<float(float)>& f, float x, float w, int steps = 96) {
    juce::Path path;
    for (int i = 0; i <= steps; ++i) {
        float u = float(i) / float(steps);
        if (i == 0) path.startNewSubPath(x, f(u)); else path.lineTo(x + w * u, f(u));
    }
    return path;
}
void stroke(Paint& p, const juce::Path& path, juce::Colour c, float width) {
    p.g.setColour(c);
    p.g.strokePath(path, juce::PathStrokeType(width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
// The area between a curve and a base line, faint.
void under(Paint& p, juce::Path path, float x, float w, float base, juce::Colour c) {
    path.lineTo(x + w, base); path.lineTo(x, base); path.closeSubPath();
    p.g.setColour(c.withAlpha(0.14f)); p.g.fillPath(path);
}

// Space: the dry hit, then the tail it leaves, as long as the decay and as loud as the mix, after its predelay.
void draw_space(Paint& p, const EffectSlot& slot, const EffectInfo& info, float x, float y, float w, float h, juce::Colour c) {
    float decay = norm(slot, info, "DECAY", norm(slot, info, "TIME", 0.5f)), mix = norm(slot, info, "MIX", 0.35f);
    float pre = norm(slot, info, "PREDELAY", 0), base = y + h;
    p.fill(x + 2, y + 2, 2, h - 2, colour::text, 1);
    float x0 = x + 10 + pre * w * 0.12f, length = (w - (x0 - x)) * (0.3f + 0.7f * decay), height = (h - 6) * (0.35f + 0.65f * mix);
    auto tail = curve([&](float u) { float t = u * (w - (x0 - x)) / std::max(1.0f, length); return base - height * std::exp(-4.0f * t) * (t <= 1.2f ? 1.0f : 0.0f); },
                      x0, w - (x0 - x));
    under(p, tail, x0, w - (x0 - x), base, c);
    stroke(p, tail, c, 1.8f);
}

// Time: the dry hit, then the repeats, a time apart, each fainter by the feedback.
void draw_time(Paint& p, const EffectSlot& slot, const EffectInfo& info, float x, float y, float w, float h, juce::Colour c) {
    float t = norm(slot, info, "TIME", norm(slot, info, "LENGTH", 0.4f)), fb = norm(slot, info, "FEEDBACK", norm(slot, info, "KEEP", 0.4f));
    float mix = norm(slot, info, "MIX", 0.3f), base = y + h, gap = w * (0.07f + 0.2f * t);
    bool ping = EffectKind(slot.kind) == EffectKind::PingPong;
    if (EffectKind(slot.kind) == EffectKind::SampleDelay) { // two delays, left and right, in milliseconds
        float l = norm(slot, info, "LEFT", 0), r = norm(slot, info, "RIGHT", 0.05f);
        p.fill(x + 2, y + 2, 3, h - 2, colour::text, 1);
        p.fill(x + 10 + l * (w - 20), y + h * 0.15f, 3, h * 0.85f, c, 1);
        p.fill(x + 10 + r * (w - 20), y + h * 0.15f, 3, h * 0.85f, c.withAlpha(0.6f), 1);
        return;
    }
    p.fill(x + 2, y + 2, 3, h - 2, colour::text, 1);
    float level = (h - 4) * (0.3f + 0.7f * mix);
    for (int i = 1; x + 2 + i * gap < x + w - 3 && level > 1.5f; ++i) {
        float tx = x + 2 + i * gap;
        if (ping && i % 2 == 0) p.fill(tx, y, 3, level * 0.5f, c.withAlpha(0.7f), 1); // the other side: from the top
        else p.fill(tx, base - level, 3, level, c, 1);
        level *= std::max(0.05f, fb);
    }
}

// Motion: the sweep as a wave, as many turns as its rate and as deep as its depth.
void draw_motion(Paint& p, const EffectSlot& slot, const EffectInfo& info, float x, float y, float w, float h, juce::Colour c) {
    float rate = norm(slot, info, "RATE", norm(slot, info, "SPEED", 0.3f)), depth = norm(slot, info, "DEPTH", 0.5f);
    float mid = y + h / 2, amp = (h / 2 - 2) * (0.15f + 0.85f * depth), turns = 1 + rate * 5;
    p.fill(x, mid - 0.5f, w, 1, colour::line);
    bool square = EffectKind(slot.kind) == EffectKind::Tremolo && raw(slot, info, "SHAPE", 0) == 1;
    auto wave = [&](float u, float sign) {
        float v = std::sin(u * turns * juce::MathConstants<float>::twoPi);
        if (square) v = v >= 0 ? 1.0f : -1.0f;
        return mid - sign * amp * v;
    };
    if (EffectKind(slot.kind) == EffectKind::AutoPan) stroke(p, curve([&](float u) { return wave(u, -1); }, x, w, 160), colour::mid, 1.2f); // the other side
    stroke(p, curve([&](float u) { return wave(u, 1); }, x, w, 160), c, 1.8f);
}

// Dirt: a wave before (faint) and after, and on the right the curve that bends it.
void draw_dirt(Paint& p, const EffectSlot& slot, const EffectInfo& info, float x, float y, float w, float h, juce::Colour c) {
    float drive = norm(slot, info, "DRIVE", norm(slot, info, "FUZZ", norm(slot, info, "AMOUNT", norm(slot, info, "SATURATION", 0.5f))));
    float k = 1 + drive * 6, wave_w = w * 0.66f, mid = y + h / 2, amp = h / 2 - 2;
    bool crush = EffectKind(slot.kind) == EffectKind::Crush;
    int levels = 3 + int(std::lround((raw(slot, info, "BITS", 8) - 2) / 14.0f * 9));
    auto shape = [&](float v) {
        if (crush) return std::round(v * levels) / levels;
        return std::tanh(k * v) / std::tanh(k);
    };
    auto sine = [&](float u) { return std::sin(u * 3 * juce::MathConstants<float>::twoPi); };
    stroke(p, curve([&](float u) { return mid - amp * sine(u); }, x, wave_w, 120), colour::dim, 1.2f);
    stroke(p, curve([&](float u) { return mid - amp * shape(sine(u)); }, x, wave_w, crush ? 360 : 120), c, 1.8f);
    float tx = x + w * 0.72f, tw = w - (tx - x);
    juce::Path diagonal; diagonal.startNewSubPath(tx, y + h); diagonal.lineTo(tx + tw, y);
    juce::Path dashed; juce::PathStrokeType(1).createDashedStroke(dashed, diagonal, std::array<float, 2> {2, 3}.data(), 2);
    p.g.setColour(colour::dim); p.g.fillPath(dashed);
    stroke(p, curve([&](float u) { return mid - (h / 2) * shape(u * 2 - 1); }, tx, tw, crush ? 200 : 60), c, 1.8f);
}

// Tone: what it does to each frequency, low to high, 0 dB the middle line.
void draw_tone(Paint& p, const EffectSlot& slot, const EffectInfo& info, float x, float y, float w, float h, juce::Colour c) {
    float mid = y + h / 2, scale = (h / 2 - 3) / 24.0f;
    std::function<float(float)> db;
    switch (EffectKind(slot.kind)) {
    case EffectKind::Filter: {
        float cut = norm(slot, info, "CUTOFF", 0.7f), reso = norm(slot, info, "RESO", 0.2f);
        int type = raw(slot, info, "TYPE", 0);
        db = [=](float u) {
            float d = u - cut, bump = reso * 16 * std::exp(-(d / 0.045f) * (d / 0.045f));
            float slope = type == 0 ? (d > 0 ? -60 * d : 0) : type == 2 ? (d < 0 ? 60 * d : 0) : -60 * std::abs(d);
            return std::max(-24.0f, slope + bump);
        };
        break;
    }
    case EffectKind::Eq: {
        float low = float(raw(slot, info, "LOW", 0)), midg = float(raw(slot, info, "MID", 0)), high = float(raw(slot, info, "HIGH", 0));
        float f = 0.25f + 0.5f * norm(slot, info, "MID FREQ", 0.5f);
        db = [=](float u) {
            auto shelf = [](float v) { return 1 / (1 + std::exp(-v)); };
            return low * shelf((0.18f - u) / 0.05f) + high * shelf((u - 0.82f) / 0.05f) + midg * std::exp(-((u - f) / 0.12f) * ((u - f) / 0.12f));
        };
        break;
    }
    case EffectKind::Width: { // the stereo field: a centre and two sides as wide as the width
        float width = norm(slot, info, "WIDTH", 0.7f), cx = x + w / 2;
        p.fill(x, mid - 0.5f, w, 1, colour::line);
        for (float side : {-1.0f, 1.0f}) {
            juce::Path ray; ray.startNewSubPath(cx, y + h); ray.lineTo(cx + side * w * 0.48f * width, y + 2);
            stroke(p, ray, c, 1.8f);
        }
        p.circle(cx, y + h - 2, 3, c);
        return;
    }
    default: { // Formant and Auto-wah: the peaks they sweep
        float centre = norm(slot, info, "CENTER", norm(slot, info, "VOWEL", 0.4f)), reso = norm(slot, info, "RESO", 0.6f);
        bool formant = EffectKind(slot.kind) == EffectKind::Formant;
        float f1 = formant ? 0.22f + 0.25f * centre : 0.15f + 0.7f * centre, f2 = 0.55f + 0.3f * (1 - centre);
        db = [=](float u) {
            float a = 18 * (0.4f + 0.6f * reso) * std::exp(-((u - f1) / 0.05f) * ((u - f1) / 0.05f));
            float b = formant ? 14 * (0.4f + 0.6f * reso) * std::exp(-((u - f2) / 0.05f) * ((u - f2) / 0.05f)) : 0;
            return std::max(-24.0f, a + b - 6);
        };
        break;
    }
    }
    p.fill(x, mid - 0.5f, w, 1, colour::line);
    auto path = curve([&](float u) { return mid - std::clamp(db(u), -24.0f, 24.0f) * scale; }, x, w, 140);
    under(p, path, x, w, mid, c);
    stroke(p, path, c, 1.8f);
}

// Dynamics: what comes out for what goes in (in dB), the threshold marked; the pumper its shape over the bar.
void draw_dynamics(Paint& p, const EffectSlot& slot, const EffectInfo& info, float x, float y, float w, float h, juce::Colour c) {
    auto kind = EffectKind(slot.kind);
    if (kind == EffectKind::Pumper || kind == EffectKind::Transient) { // a shape in time
        float depth = norm(slot, info, "DEPTH", 0.7f), attack = norm(slot, info, "ATTACK", 0.5f), sustain = norm(slot, info, "SUSTAIN", 0.5f);
        auto path = kind == EffectKind::Pumper
            ? curve([&](float u) { float t = std::fmod(u * 4, 1.0f); return y + 2 + (h - 4) * depth * std::exp(-5.0f * t); }, x, w, 160)
            : curve([&](float u) { return y + h - (h - 4) * (u < 0.04f ? u / 0.04f * (0.4f + 0.6f * attack) : (0.4f + 0.6f * attack) * std::exp(-(u - 0.04f) * (6 - 5 * sustain))); }, x, w, 160);
        under(p, path, x, w, y + h, c);
        stroke(p, path, c, 1.8f);
        return;
    }
    float side = std::min(w, h * 2.2f), bx = x + (w - side) / 2; // a square-ish graph, input along, output up
    auto px = [&](float db) { return bx + (db + 48) / 48 * side; };
    auto py = [&](float db) { return y + h - (db + 48) / 48 * h; };
    juce::Path diagonal; diagonal.startNewSubPath(px(-48), py(-48)); diagonal.lineTo(px(0), py(0));
    juce::Path dashed; juce::PathStrokeType(1).createDashedStroke(dashed, diagonal, std::array<float, 2> {2, 3}.data(), 2);
    p.g.setColour(colour::dim); p.g.fillPath(dashed);
    float threshold = float(raw(slot, info, "THRESH", raw(slot, info, "CEILING", -18)));
    float ratio = float(std::max(1, raw(slot, info, "RATIO", kind == EffectKind::Limiter ? 20 : 4)));
    float makeup = float(raw(slot, info, "MAKEUP", 0));
    auto out = [&](float in) {
        if (kind == EffectKind::Gate) return in < threshold ? -48.0f : in;
        if (kind == EffectKind::Limiter) return std::min(in, threshold);
        return std::min(0.0f, (in < threshold ? in : threshold + (in - threshold) / ratio) + makeup);
    };
    juce::Path path;
    for (int i = 0; i <= 96; ++i) {
        float in = -48 + 48.0f * float(i) / 96, o = std::max(-48.0f, out(in));
        if (i == 0) path.startNewSubPath(px(in), py(o)); else path.lineTo(px(in), py(o));
    }
    stroke(p, path, c, 1.8f);
    float t = std::clamp(threshold, -48.0f, 0.0f);
    p.circle(px(t), py(std::max(-48.0f, out(t))), 3, colour::text);
}
} // namespace

Icon family_icon(Family f) {
    switch (f) {
    case Family::Space: return Icon::Space;
    case Family::Time: return Icon::Time;
    case Family::Motion: return Icon::Modulation;
    case Family::Dirt: return Icon::Dirt;
    case Family::Tone: return Icon::Tone;
    case Family::Dynamics: return Icon::Dynamics;
    default: return Icon::Plug;
    }
}

void effect_drawing(Paint& p, const EffectSlot& slot, float x, float y, float w, float h, juce::Colour c) {
    const auto& info = effect_info(EffectKind(slot.kind));
    if (slot.bypass) c = colour::dim;
    switch (info.family) {
    case Family::Space: draw_space(p, slot, info, x, y, w, h, c); break;
    case Family::Time: draw_time(p, slot, info, x, y, w, h, c); break;
    case Family::Motion: draw_motion(p, slot, info, x, y, w, h, c); break;
    case Family::Dirt: draw_dirt(p, slot, info, x, y, w, h, c); break;
    case Family::Tone: draw_tone(p, slot, info, x, y, w, h, c); break;
    case Family::Dynamics: draw_dynamics(p, slot, info, x, y, w, h, c); break;
    default: break;
    }
}

void fx_strip(Paint& p, const Project& project, const Chain& chain, unsigned cursor, juce::Colour c, float y, bool compact) {
    constexpr float gap = 4;
    const float h = compact ? 20 : 40;
    unsigned count = chain.count(), size = std::clamp<unsigned>(chain.size, 1, max_slots);
    float w = (zone::width - 2 * zone::pad - (size - 1) * gap) / size;
    for (unsigned n = 0; n < size; ++n) {
        float x = zone::pad + n * (w + gap), cx = x + w / 2;
        if (n >= count) { // open: a dot, the next one to fill a plus (lit when it is the one in hand)
            if (n == count) {
                if (n == cursor) p.fill(x, y, w, h, c.withAlpha(0.16f), 4);
                icon(p, Icon::Plus, cx - 7, y + h / 2 - 7, 14, n == cursor ? c : colour::dim, 1.8f);
            } else p.circle(cx, y + h / 2, 2, colour::dim);
            continue;
        }
        const auto& slot = chain.slots[n];
        const auto& info = effect_info(EffectKind(slot.kind));
        bool here = n == cursor;
        p.fill(x, y, w, h, here ? c.withAlpha(0.2f) : colour::surf, 4);
        auto ink = slot.bypass ? colour::dim : here ? c : colour::sec;
        if (!compact) icon(p, family_icon(info.family), cx - 9, y + 4, 18, ink, 1.5f);
        const auto* suite = mix_suite_name(project, slot, true);
        juce::String name = suite ? juce::String(suite) : info.family == Family::Plugins ? juce::String("Plug-in") : juce::String(info.name);
        auto style = plain(slot.bypass ? colour::dim : here ? colour::text : colour::sec, type::small, Weight::Medium);
        if (slot.kind == uint8_t(EffectKind::Compressor) && p.width(name, p.fit(name, style, w - 4, type::floor)) > w - 4) name = "Comp"; // too long for a slot
        p.text(name, x + 2, y + (compact ? 3 : 24), w - 4, 14, p.fit(name, style, w - 4, type::floor), juce::Justification::centred);
        if (slot.bypass) p.fill(x + 6, y + h / 2, w - 12, 1.2f, colour::dim); // struck through
    }
}

static void gate_keeper_draw(Paint& p, const PluginPanel& panel, bool bypass,
                             float x, float y, float w, float h) {
    const auto blue = juce::Colour(0xff96c9ff);
    const auto violet = juce::Colour(0xff857ca8);
    const auto gold = juce::Colour(0xffe9cd81);
    const auto field = juce::Colour(0xff161a2b);
    const auto grid = juce::Colour(0xff33364c);
    const auto centre = juce::Justification::centred;
    static constexpr const char* pages[] = {"GATE", "TIMING", "SHAPE", "SIDECHAIN", "LINK", "LEVELS", "PAN"};
    const auto page = juce::String(pages[std::min<unsigned>(panel.row, 6)]) + "  " + juce::String(panel.row + 1) + "/7";
    const auto& curve = panel.contour_curve;
    const int style = int(curve[4]);
    p.g.setGradientFill(juce::ColourGradient(juce::Colour(0xff353450), x, y, field, x, y + h, false));
    p.g.fillRoundedRectangle(x, y, w, h, 5);
    p.ring(x, y, w, h, grid.brighter(.3f), 5);
    p.text("Gate Keeper", x + 8, y + 3, 128, 19, plain(colour::text, 16, Weight::SemiBold));
    p.text(bypass ? juce::String("BYPASS") : page, x + w - 121, y + 3, 112, 19, caps_text(blue),
           juce::Justification::centredRight);

    const float gx = x + 6, gy = y + 25, gh = h - 31, gw = w - 120;
    p.fill(gx, gy, gw, gh, field, 3);
    p.text("IN", gx + 5, gy + 2, 20, 13, caps_text(violet));
    p.text("OUT", gx + 29, gy + 2, 31, 13, caps_text(blue));
    p.text(style == GATE_UPWARD ? "BOOST" : "CUT", gx + 65, gy + 2, 52, 13, caps_text(gold));
    const float top = gy + 19, bottom = gy + gh - 6;
    for (int i = 0; i <= 4; ++i) p.fill(gx + 4, top + (bottom - top)*i/4, gw - 8, 1, grid);
    const auto level_y = [&](unsigned value) {
        const float db = 20 * std::log10(std::max(1.f, float(value))/1023.f);
        return top + std::clamp(-db / 60.f, 0.f, 1.f) * (bottom - top);
    };
    juce::Path gain;
    const auto& hist = panel.compressor_history;
    for (unsigned i = 0; i < hist.size(); ++i) {
        const auto value = bypass ? 0u : hist[i];
        const float px = gx + 5 + (gw - 10) * i / (hist.size() - 1);
        const float in = level_y(value & 1023u), out = level_y((value >> 10) & 1023u);
        if (in < bottom) p.fill(px, in, 2.8f, bottom - in, violet.withAlpha(.4f));
        if (out < bottom) p.fill(px, out, 1.45f, bottom - out, blue.withAlpha(.75f));
        const float amount = ((value >> 20) & 1023u) / 1023.f;
        const float yy = top + (style == GATE_UPWARD ? 1 - amount : amount)*(bottom - top);
        if (i == 0) gain.startNewSubPath(px, yy); else gain.lineTo(px, yy);
    }
    p.g.setColour(gold);
    p.g.strokePath(gain, juce::PathStrokeType(1.25f));

    const float tx = gx + gw + 5, tw = x + w - tx - 6;
    p.fill(tx, gy, tw, gh, field, 3);
    p.text("TRANSFER", tx, gy + 2, tw, 13, caps_text(blue), centre);
    const float left = tx + 5, width = tw - 10;
    for (int i = 0; i <= 4; ++i) {
        p.fill(left + width*i/4, top, 1, bottom - top, grid);
        p.fill(left, top + (bottom - top)*i/4, width, 1, grid);
    }
    const float threshold = left + width*(curve[0]+60)/60;
    p.fill(threshold, top, 1, bottom - top, gold.withAlpha(.7f));
    juce::Path response;
    for (unsigned i = 0; i <= 80; ++i) {
        const float level = -60.f + 60.f * i / 80;
        const float db = bypass ? 0 : gate_gain_db(level, curve[0], curve[1], curve[3], curve[2], style);
        const float py = top + std::clamp(-(level + db)/60.f, 0.f, 1.f)*(bottom-top);
        const float px = left + width*i/80;
        if (i == 0) response.startNewSubPath(px, py); else response.lineTo(px, py);
    }
    p.g.setColour(colour::text);
    p.g.strokePath(response, juce::PathStrokeType(1.55f));
}

static void vocal_tune_draw(Paint& p, const PluginPanel& panel, bool bypass,
                            float x, float y, float w, float h) {
    const auto cyan = juce::Colour(0xff5ce3ec);
    const auto amber = juce::Colour(0xffffc375);
    const auto steel = juce::Colour(0xff344855);
    const auto deep = juce::Colour(0xff0b1721);
    const auto muted = juce::Colour(0xffb6cad6);
    const auto light = juce::Colour(0xffeff8fc);
    const auto centre = juce::Justification::centred;
    const bool voiced = !bypass && panel.vocal_target > 0 && panel.vocal_pitch > 0;
    const int input = int(std::lround(panel.vocal_pitch));
    const int target = panel.vocal_target;
    static constexpr const char* notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    static constexpr const char* pages[] = {"TUNE", "BLEND", "TRACKING"};
    const auto note = [&](int midi) { return juce::String(notes[midi % 12]) + juce::String(midi / 12 - 1); };

    p.g.setGradientFill(juce::ColourGradient(steel, x, y, deep, x, y + h, false));
    p.g.fillRoundedRectangle(x, y, w, h, 6);
    p.ring(x, y, w, h, steel.brighter(.25f), 6);
    p.fill(x + 2, y + 2, w - 4, 19, deep, 4);
    p.text("Vocal Tune", x + 10, y + 3, 140, 17, plain(light, 15, Weight::SemiBold));
    const auto page = juce::String(pages[std::min<unsigned>(panel.row, 2)]) +
                      "  " + juce::String(panel.row + 1) + "/3";
    p.text(page, x + w - 112, y + 3, 102, 17, caps_text(cyan), juce::Justification::centredRight);

    const float cx = x + w / 2, cy = y + 54;
    const float side = (w - 80) / 2 - 12;
    p.fill(x + 8, y + 30, side, 48, deep.withAlpha(.62f), 4);
    p.fill(x + w - side - 8, y + 30, side, 48, deep.withAlpha(.62f), 4);
    p.text("INPUT", x + 8, y + 34, side, 13, caps_text(muted), centre);
    p.text(voiced ? note(input) : "--", x + 8, y + 49, side, 23, plain(amber, 20, Weight::SemiBold), centre);
    p.text("CORRECTION", x + w - side - 8, y + 34, side, 13, caps_text(muted), centre);
    const auto correction = juce::String(panel.vocal_cents > 0 ? "+" : "") + juce::String(panel.vocal_cents) + " ct";
    p.text(voiced ? correction : "--", x + w - side - 8, y + 49, side, 23,
           plain(cyan, 18, Weight::SemiBold), centre);

    const float radius = 27;
    p.circle(cx, cy, 31, deep);
    p.g.setGradientFill(juce::ColourGradient(juce::Colour(0xff205768), cx, cy - 6,
                                             deep, cx, cy + 24, true));
    p.g.fillEllipse(cx - 22, cy - 22, 44, 44);
    constexpr float span = 2.1f;
    juce::Path arc;
    arc.addCentredArc(cx, cy, radius, radius, 0, -span, span, true);
    p.g.setColour(steel.brighter(.25f));
    p.g.strokePath(arc, juce::PathStrokeType(3));
    for (int i = -3; i <= 3; ++i) {
        const float a = i * span / 3;
        p.circle(cx + 30 * std::sin(a), cy - 30 * std::cos(a), i == 0 ? 1.4f : .85f, muted);
    }
    if (voiced) {
        const float offset = std::clamp((panel.vocal_pitch - target) / 1.5f, -1.f, 1.f) * span;
        const float start = std::min(0.f, offset), end = std::max(0.f, offset);
        if (end - start > .025f) {
            juce::Path active;
            active.addCentredArc(cx, cy, radius, radius, 0, start, end, true);
            p.g.setColour(cyan);
            p.g.strokePath(active, juce::PathStrokeType(3));
        }
        p.circle(cx + radius * std::sin(offset), cy - radius * std::cos(offset), 2.3f, amber);
    }
    p.text("TARGET", cx - 26, cy - 13, 52, 12, caps_text(muted), centre);
    p.text(voiced ? note(target) : "--", cx - 23, cy - 2, 46, 19, plain(cyan, 17, Weight::Bold), centre);

    const float key_x = x + 8, key_y = y + h - 23, key_w = (w - 16) / 14;
    const int base = voiced ? std::min(input, target) / 12 * 12 : 60;
    static constexpr int whites[] = {0, 2, 4, 5, 7, 9, 11};
    static constexpr int blacks[] = {1, 3, -1, 6, 8, 10};
    const auto key_colour = [&](int midi, juce::Colour idle) {
        return voiced && midi == target ? cyan : voiced && midi == input ? amber : idle;
    };
    for (int k = 0; k < 14; ++k)
        p.fill(key_x + key_w * k, key_y, key_w - 1, 18,
               key_colour(base + 12 * (k / 7) + whites[k % 7], juce::Colour(0xffd8e3e9)), 1.5f);
    for (int k = 0; k < 14; ++k)
        if (k % 7 < 6 && blacks[k % 7] >= 0)
            p.fill(key_x + key_w * (k + .68f), key_y, key_w * .56f, 11,
                   key_colour(base + 12 * (k / 7) + blacks[k % 7], deep), 1.5f);
    if (bypass) {
        p.fill(cx - 36, key_y, 72, 18, deep, 3);
        p.text("BYPASS", cx - 36, key_y, 72, 18, caps_text(amber), centre);
    }
}

bool prism_in_hand(const Snapshot& s, const Chain& chain, unsigned cursor) {
    if (cursor >= chain.count()) return false;
    const auto& slot = chain.slots[cursor];
    return slot.kind == plugin_effect_kind && s.ui.plugin_panel.eq_ready &&
           s.ui.plugin_panel.entry == unsigned(slot.params[0]);
}

static void split_dynamics_display(Paint& p, const Snapshot& s, const EffectSlot& slot, float x, float y,
                                   float w, float h, bool deesser) {
    if (h < 22) return;
    p.fill(x, y, w, h, juce::Colour(0xff121922), 4);
    const auto position = [](float hz) { return std::log10(hz / 20.f) / 3.f; };
    const float split[] {0.f, position(deesser ? float(slot.params[0]) : 180.f), deesser ? 1.f : position(3000.f), 1.f};
    const juce::Colour tones[] {juce::Colour(0xff69badc), juce::Colour(0xffbdde72), juce::Colour(0xffc899ec)};
    const auto labels = deesser ? std::array<const char*, 3>{"BODY", "ESS", ""} :
                                  std::array<const char*, 3>{"LOW", "MID", "HIGH"};
    const auto db_y = [=](float db) { return y + h - 3.f - (h - 17.f) * std::clamp((db + 78.f) / 78.f, 0.f, 1.f); };
    for (unsigned band = 0; band < (deesser ? 2u : 3u); ++band) {
        const float left = x + w * split[band], width = w * (split[band + 1] - split[band]);
        p.fill(left + 1, y + 1, width - 2, h - 2, tones[band].withAlpha(.09f), 2);
        p.text(labels[band], left + 4, y + 1, width - 8, 12, caps_text(tones[band]));
        if (!deesser || band == 1)
            p.fill(left + 1, db_y(float(slot.params[deesser ? 1u : band])), width - 2, 1, tones[band].withAlpha(.75f));
    }
    if (slot.bypass) return;
    const float bin_width = w / s.mix_reference.size();
    for (unsigned bin = 0; bin < s.mix_reference.size(); ++bin) {
        const float level = s.mix_reference[bin];
        if (level <= .02f) continue;
        const float pos = (bin + .5f) / s.mix_reference.size();
        const unsigned band = pos < split[1] ? 0 : pos < split[2] ? 1 : 2;
        const float level_y = db_y(level * 78.f - 78.f);
        p.fill(x + bin * bin_width, level_y, bin_width - .5f, y + h - 3 - level_y, tones[band].withAlpha(.45f));
    }
}

void fx_in_hand(Paint& p, const Snapshot& s, const Chain& chain, unsigned cursor, juce::Colour c, float y, float bottom) {
    constexpr float x = zone::pad, w = zone::width - 2 * zone::pad;
    unsigned count = chain.count();
    if (cursor >= count) { // the open slot: Push adds one
        if (count >= chain.size) return;
        float mid = zone::width / 2.0f, gy = y + (bottom - y) / 2 - 13;
        icon(p, Icon::Push, mid - 28, gy, 26, colour::sec, 1.5f);
        icon(p, Icon::Plus, mid + 6, gy + 4, 18, colour::sec, 1.8f);
        return;
    }
    const auto& slot = chain.slots[cursor];
    const auto& info = effect_info(EffectKind(slot.kind));
    bool plugin = info.family == Family::Plugins;
    if (plugin && !slot.bypass && plugin_screen_shown(s, 1) && s.plugin_picture->entry == unsigned(slot.params[0])) { // its own strip, to the bottom
        draw_screen_picture(p, *s.plugin_picture, 0, float(zone::height) - float(s.plugin_picture->height));
        return;
    }
    if (plugin && !slot.bypass && plugin_strip_shown(s, unsigned(slot.params[0]))) { draw_plugin_strip(p, s, 0, float(zone::height - skin_strip_rows)); return; } // the maker's skin strip, to the bottom
    const auto* suite = mix_suite_name(s.project, slot);
    juce::String name = suite ? juce::String(suite) : plugin ? plugin_name(s, unsigned(slot.params[0])) : juce::String(info.name);
    const auto& panel = s.ui.plugin_panel;
    const bool ours = plugin && panel.entry == unsigned(slot.params[0]);
    if(ours && panel.status==PluginStatus::Ready &&
       draw_plugin_pro_ui(p,s,x,y,w,bottom-y,false,slot.bypass))return;
    if (!s.ui.shift && ours && panel.gate_ready && bottom - y >= 90) {
        gate_keeper_draw(p, panel, slot.bypass, x, y, w, bottom - y);
        return;
    }
    if (!s.ui.shift && ours && panel.vocal_tune_ready && bottom - y >= 108) {
        vocal_tune_draw(p, panel, slot.bypass, x, y, w, bottom - y);
        return;
    }
    const bool prism = ours && panel.eq_ready;
    juce::String family = plugin ? juce::String("PLUG-IN") : juce::String(family_name(info.family)).toUpperCase();
    auto fam = caps_text(colour::mid);
    float fw = p.width(family, fam) + 8;
    auto title = plain(slot.bypass ? colour::mid : colour::text, type::title, prism ? Weight::Bold : Weight::SemiBold);
    float room = prism ? w - 100 : w - fw - 28;
    float nw = p.text(name, x, y, room, 24, p.fit(name, title, room, type::floor));
    if (!prism) {
        p.fill(x + nw + 8, y + 5, fw, 15, colour::surf, 3);
        p.text(family, x + nw + 8, y + 5, fw, 15, fam, juce::Justification::centred);
    }
    if (slot.bypass) draw_glyph(p, Glyph::LedOff, x + nw + 8 + (prism ? 0 : fw + 6), y + 12.5f, colour::mid);
    float dy = y + (prism ? 24 : 30), dh = bottom - dy;
    if (!plugin) {
        const auto kind = EffectKind(slot.kind);
        if (kind == EffectKind::DeEsser || kind == EffectKind::BandCompressor)
            split_dynamics_display(p, s, slot, x, dy, w, dh, kind == EffectKind::DeEsser);
        else
            effect_drawing(p, slot, x, dy, w, dh - (suite ? 5.f : 0.f), c);
        if (suite && !slot.bypass) {
            const float step = w / s.mix_reference.size();
            for (unsigned i = 0; i < s.mix_reference.size(); ++i)
                p.fill(x + i * step, bottom - 1, step - .5f, 1, c.withAlpha(s.mix_reference[i] * .65f));
        }
        if (suite && !slot.bypass && s.mix_overlap.strength > .09f) {
            const auto red = juce::Colour(0xffff5860);
            for (unsigned i = 0; i < s.mix_overlap.glow.size(); ++i)
                if (s.mix_overlap.glow[i] > .09f)
                    p.fill(x + i * w / s.mix_overlap.glow.size(), bottom - 2, 2, 2, red.withAlpha(.75f));
        }
        return;
    }
    const auto live = [&] {
        const auto ink = c == colour::text ? juce::Colour(0xff76e1cc) : c;
        draw_live_analyzer(p, panel, x, dy, w, dh - 16, ink, slot.bypass);
        const auto preset = juce::String::fromUTF8(panel.preset_name);
        const auto label = panel.programs ? (preset.isEmpty() ? juce::String("SHIFT + DATA  PRESETS") : preset)
                                          : panel.rows ? juce::String("PARAMETERS  ") + juce::String(panel.row + 1) + "/" + juce::String(panel.rows)
                                                       : juce::String();
        const auto style = plain(colour::sec, type::small);
        p.text(label, x + 2, bottom - 15, w - 4, 14, p.fit(label, style, w - 4, type::floor));
    };
    if(ours && panel.meter_page) { draw_analog_output_meter(p,panel,x,dy,w,dh,slot.bypass); return; }
    if (s.ui.shift && ours && panel.eq_ready && !slot.bypass && dh >= 46) {
        const auto status = s.prism_overlaps_held ? "LAST READING" : "LIVE OVERLAPS";
        p.text(status, x + 154, y + 3, w - 156, 17, caps_text(colour::sec), juce::Justification::centredRight);
        prism_overlap_list(p, s, x, dy, w, dh);
        return;
    }
    if (s.ui.shift && ours && panel.live_analyzer && panel.status == PluginStatus::Ready && dh >= 45) { live(); return; }
    // Show the selected plug-in and the current parameter row.
    if (ours && panel.contour_ready && dh >= 46) {
        static constexpr const char* pages[] = {"DYNAMICS", "TIMING", "SHAPE", "DETECTOR",
                                                "GAIN", "BLEND", "SIDECHAIN", "SC BELL"};
        const auto page = juce::String(pages[std::min<unsigned>(panel.row, 7)]) +
                          "  " + juce::String(panel.row + 1) + "/8";
        p.text(page, x + 185, y + 3, w - 187, 17, caps_text(juce::Colour(0xffc7db62)),
               juce::Justification::centredRight);
        const float transfer_w = w * .34f;
        compressor_history(p, panel, x, dy, w - transfer_w - 4, dh, slot.bypass);
        contour_curve(p, panel, x + w - transfer_w, dy, transfer_w, dh, slot.bypass);
        return;
    }
    if (!s.ui.shift && ours && panel.status == PluginStatus::Ready &&
        draw_plugin_parameter_page(p,s,x,dy,w,dh,c,slot.bypass)) return;
    if (ours && panel.rack76_ready && dh >= 46) {
        static constexpr const char* pages[] = {"LEVELS", "TIMING", "CIRCUIT", "BLEND"};
        p.text(juce::String(pages[std::min<unsigned>(panel.row, 3)]) + "  " + juce::String(panel.row + 1) + "/4",
               x + 154, y + 3, w - 156, 17,
               caps_text(colour::sec), juce::Justification::centredRight);
        if (panel.compressor_history_ready && dh >= 76) {
            compressor_history(p, panel, x, dy, w, dh - 56, slot.bypass);
            dy += dh - 53;
        }
        rack76_display(p, panel, x, dy, w, slot.bypass);
        return;
    }
    if (ours && panel.eq_ready && dh >= 46) {
        static const char* const views[] = {"EQ", "SHAPE", "DYNAMICS", "OPTIONS"};
        static const char* const sections[] = {"HIGH PASS", "LOWS", "MIDS", "HIGHS", "LOW PASS"};
        const auto band = prism_selected_band(panel);
        const auto page = panel.rows == 5 ? juce::String(sections[band])
            : juce::String("BAND ") + juce::String(panel.row / 4 + 1) + "  " + views[panel.row % 4];
        p.text(page, x + 154, y + 3, w - 156, 17, caps_text(prism_band_colour(band)), juce::Justification::centredRight);
        prism_graph(p, s, x, dy, w, dh, slot.bypass);
        return;
    }
    if (ours && panel.status == PluginStatus::Ready && name == "Break 26" && dh >= 46) {
        break26_display(p, s, x, dy + (dh - 46) * .5f, w, slot.bypass);
        if (panel.rows > 1) page_dots(p, x + w - 3, y + 12, 2, int(panel.row), c, true);
        return;
    }
    if (ours && panel.status == PluginStatus::Ready && name == "Second Spin" && dh >= 52) {
        if (panel.tape_deck_ready) {
            static const char* pages[] = {"CASSETTE", "BLEND", "TRANSPORT", "LEVELS", "CALIBRATION", "NOISE", "METER"};
            p.text(pages[std::min<unsigned>(panel.row, 6)], x + 154, y + 3, w - 156, 17,
                   caps_text(c), juce::Justification::centredRight);
            draw_second_spin_vintage(p, panel, x, dy, w, dh, s.now,
                                     s.playing || panel.tape_vu > .001f, slot.bypass);
            return;
        }
        draw_second_spin_vintage(p, panel, x, dy, w, dh, s.now, s.playing, slot.bypass);
        return;
    }
    if (ours && panel.live_analyzer && panel.status == PluginStatus::Ready && dh >= 45) {
        if (dh < 55) { live(); return; }
        const float plot_h = dh - 15;
        const float control_w = panel.rows ? 109.f : 0.f;
        draw_plugin_scene(p, panel, s.now, x, dy, w - control_w, plot_h, slot.bypass);
        if (control_w) {
            const float cx = x + w - control_w + 4;
            p.fill(cx, dy, control_w - 4, plot_h, colour::panel, 3);
            // The lower, compact strip uses the full height for the current K1–K3 values.
            draw_plugin_parameter_levels(p, s, cx + 4, dy + 1, control_w - 12, plot_h - 2, slot.bypass);
        }
        const auto preset = juce::String::fromUTF8(panel.preset_name);
        const auto label = preset.isNotEmpty() ? preset : panel.rows ?
            "PAGE " + juce::String(panel.row + 1) + " / " + juce::String(panel.rows) : juce::String();
        const auto caption = plain(colour::sec, type::small);
        p.text(label, x + 2, bottom - 14, w - 117, 13, p.fit(label, caption, w - 117, type::floor));
        p.text("SHIFT  LIVE VIEW", x + w - 115, bottom - 14, 113, 13, caption, juce::Justification::centredRight);
        return;
    }
    icon(p, panel.status == PluginStatus::Failed || panel.status == PluginStatus::Crashed ? Icon::PlugBroken : Icon::Plug, x, dy + 4, 28, colour::dim, 1.4f);
    juce::String state = !ours || panel.status == PluginStatus::Loading || panel.status == PluginStatus::None ? "OPENING"
                       : panel.status == PluginStatus::Failed ? "WON'T OPEN" : panel.status == PluginStatus::Crashed ? "STOPPED" : "";
    if (state.isNotEmpty()) p.text(state, x + 38, dy + 10, w - 38, 16, caps_text(panel.status == PluginStatus::Loading || !ours ? colour::mid : colour::rec));
    else if (panel.rows > 1) page_dots(p, x + 38, dy + 18, int(std::min<unsigned>(panel.rows, 24)), int(panel.row), c);
}

} // namespace melodee
