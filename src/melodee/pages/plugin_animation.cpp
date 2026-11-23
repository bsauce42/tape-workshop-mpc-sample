// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/pages/common.h"
#include "melodee/pages/plugin_waveform.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

namespace melodee {
namespace {
using namespace ui;
using juce::Colour;
constexpr float tau = 6.283185307f;
constexpr float width = zone::width;
constexpr float centre = width * .5f, middle = 82.f;

enum class Form { Bloom, Orbit, Ridges, Tide, Piano, Reels, Prism, Meter, Gate, Radar, Steps, Matrix, Stereo, Sparks };
struct Style { std::string_view name; Form form; uint32_t colour; unsigned detail; };
constexpr Style styles[] {
    {"Afterglow Piano", Form::Piano, 0xffffad62, 0},
    {"Astral Loom", Form::Orbit, 0xffa8abff, 0},
    {"Dex FM", Form::Matrix, 0xff65f9b2, 0},
    {"Layer House", Form::Ridges, 0xff67e9ef, 0},
    {"Room Grand", Form::Piano, 0xffa6d6ff, 1},
    {"Sloop FM6", Form::Tide, 0xff56fee3, 0},
    {"Stillwater", Form::Tide, 0xff729aff, 1},
    {"Sub Foundry", Form::Meter, 0xffff693b, 0},
    {"Velvet Choir", Form::Bloom, 0xfff8a5eb, 0},
    {"Break 26", Form::Steps, 0xffffb94c, 0},
    {"Contour Comp", Form::Meter, 0xffdef071, 1},
    {"Gate Keeper", Form::Gate, 0xff65eebc, 0},
    {"Prism EQ", Form::Prism, 0xff91eaff, 0},
    {"Rack 76", Form::Meter, 0xff7da7ff, 2},
    {"Second Spin", Form::Reels, 0xffeab783, 0},
    {"Vocal Tune", Form::Stereo, 0xff80e7ff, 0},
    {"LFO Sweep Mover", Form::Radar, 0xff7fe2ff, 0},
    {"LFO SWEEP Move", Form::Radar, 0xff7fe2ff, 0},
    {"Mover", Form::Radar, 0xff7fe2ff, 0},
    {"Frodhes", Form::Sparks, 0xffffaa70, 0},
    {"Tenebrae", Form::Orbit, 0xff9a7aff, 1},
    {"FMini-1", Form::Bloom, 0xff83f2a7, 1},
    {"Aphex Move", Form::Matrix, 0xfff3b7ff, 3},
    {"Braids Move", Form::Stereo, 0xff9dffcf, 3},
    {"Hera Move", Form::Orbit, 0xffeeb8ff, 3},
    {"Plaits Move", Form::Bloom, 0xffffce9e, 4},
    {"Aphex", Form::Matrix, 0xfff4adff, 1},
    {"Braids", Form::Stereo, 0xff9de178, 1},
    {"Dexed", Form::Matrix, 0xff88a7ff, 2},
    {"Hera", Form::Orbit, 0xffed8ae0, 2},
    {"Plaits", Form::Bloom, 0xffffca82, 2},
    {"CS-1", Form::Radar, 0xff79ffa6, 1},
    {"DRUM-X", Form::Steps, 0xffff755f, 1},
    {"Elixir Kit", Form::Steps, 0xffc4f076, 2},
    {"FM-106", Form::Bloom, 0xff84d8ff, 3},
    {"Guru-600", Form::Ridges, 0xffffa2cb, 1},
    {"Hortator Kit", Form::Sparks, 0xffffc86b, 1},
    {"K-WASUB", Form::Tide, 0xffa8ed83, 2},
    {"K-WASUB Kit", Form::Steps, 0xff61e9c3, 3},
    {"MONO2K", Form::Gate, 0xffff8e68, 1},
    {"OberVave", Form::Ridges, 0xff96aaff, 2},
    {"Prophet", Form::Prism, 0xffe8a37b, 1},
    {"S1Vave", Form::Stereo, 0xff7beff6, 2},
    {"6W6", Form::Steps, 0xffe9c878, 4},
    {"8W8", Form::Steps, 0xffeaaa49, 5},
    {"9W9", Form::Steps, 0xff7195ff, 6},
    {"CW-78", Form::Steps, 0xff93f5c3, 7},
    {"TR-MPC", Form::Steps, 0xffff796d, 8},
    {"Sophie", Form::Sparks, 0xfff8a7fc, 2},
};

Style style_of(std::string_view name) {
    for (const auto& style : styles)
        if (name == style.name || (name.size() == style.name.size() + 5 &&
                name.substr(name.size() - 5) == " (TW)" && name.substr(0, style.name.size()) == style.name)) return style;
    uint32_t hash = 2166136261u;
    for (const unsigned char byte : name) hash = (hash ^ byte) * 16777619u;
    const auto ink = Colour::fromHSV(float(hash % 360) / 360.f, .46f, 1.f, 1.f);
    constexpr Form fallback[] {Form::Bloom, Form::Orbit, Form::Ridges, Form::Tide, Form::Prism,
                               Form::Radar, Form::Steps, Form::Matrix, Form::Stereo, Form::Sparks};
    return {name, fallback[(hash >> 16) % std::size(fallback)], ink.getARGB(), (hash >> 24) % 9};
}

struct Signal {
    const PluginPanel& panel;
    float time, left = 0, right = 0;
    bool bypass;
    float peak() const { return std::max(left, right); }
    float band(unsigned i) const {
        return bypass ? 0.f : std::clamp(panel.eq_spectrum[i % panel.eq_spectrum.size()], 0.f, 1.f);
    }
    float wave(unsigned i, bool r = false) const {
        if (bypass) return 0.f;
        const auto& channel = r ? panel.scope_right : panel.scope;
        const auto packed = channel[i % channel.size()];
        return float(int(int16_t(packed)) + int(int16_t(packed >> 16))) / 65536.f;
    }
};

float peak_of(const std::array<uint32_t, 64>& scope) {
    int peak = 0;
    for (const auto pair : scope)
        peak = std::max({peak, std::abs(int(int16_t(pair))), std::abs(int(int16_t(pair >> 16)))});
    return peak / 32768.f;
}

void stroke(Paint& p, const juce::Path& path, Colour ink, float thickness = 1.f) {
    p.g.setColour(ink);
    p.g.strokePath(path, juce::PathStrokeType(thickness, juce::PathStrokeType::curved));
}

void point(juce::Path& path, unsigned n, float x, float y) {
    if (n == 0) path.startNewSubPath(x, y);
    else path.lineTo(x, y);
}

void dot(Paint& p, float x, float y, float radius, Colour c) {
    p.g.setColour(c);
    p.g.fillEllipse(x - radius, y - radius, radius * 2, radius * 2);
}

void circle(Paint& p, float x, float y, float radius, Colour c) {
    p.g.setColour(c);
    p.g.drawEllipse(x - radius, y - radius, radius * 2, radius * 2, 1.f);
}

void glow(Paint& p, const juce::Path& path, Colour ink, float energy) {
    stroke(p, path, ink.withAlpha(.12f + .13f * energy), 4.f);
    stroke(p, path, ink.withAlpha(.70f + .30f * energy), 1.15f);
}

void draw_scene(Paint& p, Style style, const Signal& s) {
    const Colour ink(style.colour);
    const Colour other = ink.withRotatedHue(.19f + .033f * (style.detail % 5)).withMultipliedSaturation(.9f);
    const float t = s.time, e = s.peak();
    const float variation = float(style.detail);
    const float phase = variation * .83f;
    const auto line = [&](float x1, float y1, float x2, float y2, Colour c) {
        p.g.setColour(c);
        p.g.drawLine(x1, y1, x2, y2, 1.f);
    };
    switch (style.form) {
    case Form::Bloom: {
        // Petals open with spectral energy; each ring responds to a different set of bands.
        for (unsigned r = 0; r < 8; ++r) {
            juce::Path petal;
            for (unsigned n = 0; n <= 64; ++n) {
                const float a = n * tau / 64.f;
                const float radius = (13 + r * 3.9f) * (1 + .18f * std::cos(a * (5 + style.detail % 4) + t * .8f + r * .28f)
                      + .22f * s.band(n + r * 3) + .28f * e * std::sin(3 * a - t));
                point(petal, n, centre + 1.7f * radius * std::cos(a), middle + radius * std::sin(a));
            }
            stroke(p, petal, ink.interpolatedWith(other, r / 7.f).withAlpha(.31f + .55f * e), r == 7 ? 1.3f : .8f);
        }
        dot(p, centre, middle, 2 + 6 * e, ink);
        break;
    }
    case Form::Orbit: {
        for (unsigned orbit = 0; orbit < 5; ++orbit) {
            const float rx = 25 + orbit * (17 + variation * 2), ry = 9 + orbit * 7;
            const float tilt = .16f * (float(orbit) - 2) + .11f * variation;
            juce::Path path;
            for (unsigned n = 0; n <= 48; ++n) {
                const float a = n * tau / 48.f;
                const float x = rx * std::cos(a), y = ry * std::sin(a);
                point(path, n, centre + x, middle + y + x * tilt);
            }
            stroke(p, path, ink.withAlpha(.23f + .45f * s.band(orbit * 9)));
            const float a = t * (.65f + orbit * .25f) + orbit + phase;
            const float x = rx * std::cos(a);
            dot(p, centre + x, middle + ry * std::sin(a) + x * tilt, 2.1f + 4.f * s.band(orbit * 9), orbit & 1 ? other : ink);
        }
        dot(p, centre, middle, 6 + 12 * e, ink.withAlpha(.8f));
        break;
    }
    case Form::Ridges: case Form::Tide: {
        const bool ridges = style.form == Form::Ridges;
        for (unsigned row = 0; row < 9; ++row) {
            juce::Path trace;
            for (unsigned n = 0; n <= 56; ++n) {
                const float u = n / 56.f;
                const float crest = ridges ? std::abs(std::sin(u * (13 + variation * 3) + row * .64f + t * .65f))
                                           : std::sin(u * (9 + variation * 3) + row * .57f - t);
                const float signal = ridges ? s.band(n + row) : s.wave(n + row, (row & 1) != 0);
                point(trace, n, 7 + u * (width - 14), 55 + row * 7 + 8 * crest - (12 + row * 1.6f) * signal);
            }
            const auto shade = ink.interpolatedWith(other, row / 8.f);
            if (row == 7) glow(p, trace, shade, e);
            else stroke(p, trace, shade.withAlpha(.20f + row * .055f));
        }
        break;
    }
    case Form::Piano: {
        for (unsigned n = 0; n < 24; ++n) {
            const float x = 9 + n * 12.7f, power = s.band(n * 2);
            if (style.detail == 0) {
                const float y = 51 + 37 * (1.f - power);
                line(x, y, x, 107, ink.withAlpha(.22f + .66f * power));
                dot(p, x, y, 1.2f + 2 * power, other);
            } else {
                const float bias = 17 + 24 * std::abs(float(n) - 11.5f) / 11.5f;
                line(x, 34 + bias, centre + (x - centre) * .65f, 108, ink.withAlpha(.26f + .67f * power));
            }
            const auto ivory = Colour(0xffe2e6e2).interpolatedWith(ink, power);
            p.fill(x, 111, 11.4f, 20, ivory.withAlpha(.48f + .47f * power), 1);
            if (n % 7 != 2 && n % 7 != 6) p.fill(x + 8.1f, 111, 6, 11, Colour(0xff151b27), 1);
        }
        break;
    }
    case Form::Reels: {
        for (unsigned n = 0; n < 2; ++n) {
            const float x = n ? 224.f : 96.f, radius = 34.f;
            const float spin = s.bypass ? 0.f : t * (2.f + .8f * s.panel.tape_speed);
            circle(p, x, middle, radius, ink);
            circle(p, x, middle, radius - 6, ink.withAlpha(.45f));
            for (unsigned ray = 0; ray < 6; ++ray) {
                const float a = spin + ray * tau / 6.f;
                line(x + 8 * std::cos(a), middle + 8 * std::sin(a), x + 27 * std::cos(a),
                     middle + 27 * std::sin(a), ink.withAlpha(.6f));
            }
            dot(p, x, middle, 7, other);
        }
        line(97, 117, 224, 117, ink);
        p.fill(132, 78, 56, 4, other.withAlpha(.19f), 2);
        p.fill(132, 78, 56 * std::max(e, s.bypass ? 0.f : std::clamp(s.panel.tape_vu, 0.f, 1.f)), 4, ink, 2);
        break;
    }
    case Form::Prism: {
        const float split = style.detail ? 193.f : 147.f;
        juce::Path prism;
        prism.addTriangle(split, 41, split - 39, 113, split + 39, 113);
        stroke(p, prism, ink, 1.4f);
        line(13, middle + 8, split - 13, middle - 8, Colour(0xffe1f9ff));
        for (unsigned band = 0; band < 17; ++band) {
            const float power = s.band(band * 4);
            const auto ray = Colour::fromHSV(.81f * band / 16.f, .72f, 1.f, .34f + .62f * power);
            line(split + 12, middle + band * .6f, width - 10, 44 + band * 4.6f - 7 * power, ray);
        }
        break;
    }
    case Form::Meter: {
        if (style.detail == 2) {
            const float value = s.bypass ? 0.f : s.panel.rack76_ready ? std::clamp(s.panel.rack76_needle, 0.f, 1.f) : e;
            const float base = 122.f, radius = 78.f;
            for (unsigned n = 0; n <= 22; ++n) {
                const float a = -1.08f + n * 2.16f / 22.f;
                const float inner = radius - (n % 3 == 0 ? 10 : 5);
                line(centre + inner * std::sin(a), base - inner * std::cos(a), centre + radius * std::sin(a),
                     base - radius * std::cos(a), n > 17 ? Colour(0xffff7965) : ink);
            }
            const float a = -1.08f + 2.16f * value;
            line(centre, base, centre + (radius - 8) * std::sin(a), base - (radius - 8) * std::cos(a), Colour(0xffeef5ff));
            dot(p, centre, base, 5, ink);
        } else {
            for (unsigned n = 0; n < 48; ++n) {
                const float x = 9 + n * 6.4f;
                const float power = style.detail ? s.band(n) : s.band(n / 2);
                const float extent = (style.detail ? 18.f : 8.f) + 37.f * power;
                for (float y = 0; y < extent; y += 4.6f) {
                    const auto shade = ink.interpolatedWith(other, y / 55.f);
                    p.fill(x, middle - y, 4.6f, 2.7f, shade.withAlpha(.35f + .65f * power));
                    if (!style.detail) p.fill(x, middle + y + 4, 4.6f, 2.7f, shade.withAlpha(.26f + .65f * power));
                }
            }
            if (style.detail) {
                juce::Path trace;
                for (unsigned n = 0; n < 64; ++n) {
                    const float u = n / 63.f;
                    point(trace, n, 9 + u * (width - 18), 124 - 61 * u - 12 * s.wave(n) + 30 * std::max(0.f, u - .48f));
                }
                glow(p, trace, other, e);
            }
        }
        break;
    }
    case Form::Gate: {
        const float opening = 8 + 34 * std::sqrt(e);
        for (unsigned n = 0; n < 8; ++n) {
            const float at = n * 11.f, drift = std::sin(t * .9f + n * .45f + phase) * 3;
            line(centre - opening - at, 45 + drift, centre - opening - at, 122 - drift, ink.withAlpha(.67f - n * .062f));
            line(centre + opening + at, 45 - drift, centre + opening + at, 122 + drift, other.withAlpha(.67f - n * .062f));
        }
        juce::Path trace;
        for (unsigned n = 0; n < 64; ++n)
            point(trace, n, 8 + n * (width - 16) / 63.f, middle - 27 * s.wave(n));
        glow(p, trace, ink, e);
        break;
    }
    case Form::Radar: {
        for (unsigned n = 1; n <= 4; ++n) circle(p, centre, middle, n * 10.f, ink.withAlpha(.22f));
        const float angle = s.bypass ? 0.f : t * (1.4f + variation * .27f);
        for (unsigned n = 0; n < 18; ++n) {
            const float a = angle - n * .052f;
            line(centre, middle, centre + (45 + 4 * e) * std::cos(a),
                 middle + (45 + 4 * e) * std::sin(a), ink.withAlpha(.76f - n * .039f));
        }
        for (unsigned n = 0; n < 10; ++n) {
            const float power = s.band(n * 5), a = n * tau / 10.f;
            dot(p, centre + (17 + 26 * power) * std::cos(a), middle + (17 + 26 * power) * std::sin(a),
                1 + 3.5f * power, other.withAlpha(.35f + .63f * power));
        }
        break;
    }
    case Form::Steps: {
        const unsigned cols = 8 + 4 * (style.detail % 3);
        const float step = (width - 20) / cols;
        for (unsigned lane = 0; lane < 4; ++lane) {
            for (unsigned n = 0; n < cols; ++n) {
                const float power = s.band(3 * n + lane * 11);
                const float x = 10 + n * step, y = 43 + lane * 21;
                const bool pulse = !s.bypass && n == unsigned(t * (4 + style.detail * .38f)) % cols;
                const auto c = ink.interpolatedWith(other, lane / 3.f);
                p.fill(x, y, step - 2, 17.f, c.withAlpha(.12f + .55f * power + (pulse ? .20f : 0.f)), 2);
                if (power > .04f) p.fill(x + 1, y + 15, (step - 4) * power, 1.6f, c);
            }
        }
        break;
    }
    case Form::Matrix: {
        for (unsigned row = 0; row < 15; ++row) {
            for (unsigned n = 0; n < 50; ++n) {
                const float power = s.band(n + row * 3);
                const float x = 11 + n * 6.1f, y = 38 + row * 6.15f;
                const float a = std::sin(n * .29f - t + phase) + std::cos(row * .53f + t * .66f);
                const float edge = std::abs(float(row) - 7.f - 6.f * s.wave(n));
                if (a + power < .55f && edge > .85f) continue;
                const auto c = ink.interpolatedWith(other, row / 14.f);
                p.fill(x, y, 2.2f, 3.1f, c.withAlpha(std::clamp(.18f + power * .49f + (edge < 1 ? .30f : 0.f), 0.f, 1.f)));
            }
        }
        break;
    }
    case Form::Stereo: {
        for (unsigned ch = 0; ch < 2; ++ch) {
            juce::Path trace;
            for (unsigned n = 0; n < 64; ++n) {
                const float u = n / 63.f;
                const float sample = s.wave(n, ch != 0);
                const float wobble = std::sin(u * tau * (1 + style.detail) + t + ch * 1.6f);
                const float offset = style.detail ? wobble * e * 11.f : 0.f;
                const float tune = !s.bypass && s.panel.vocal_tune_ready ? std::clamp(float(s.panel.vocal_cents), -40.f, 40.f) * .24f : 0.f;
                point(trace, n, 7 + u * (width - 14), middle + (ch ? 8.f : -8.f) - 40 * sample + offset + tune);
            }
            glow(p, trace, ch ? other : ink, e);
        }
        break;
    }
    case Form::Sparks: {
        for (unsigned n = 0; n < 27; ++n) {
            const float a = n * tau / 27.f + t * .25f + phase, power = s.band(n * 2);
            const float outer = 29.f + (n % 3) * 8.f + 22 * power + 12 * e;
            const auto c = ink.interpolatedWith(other, (n % 7) / 6.f);
            line(centre + 12 * std::cos(a), middle + 9 * std::sin(a), centre + outer * 1.8f * std::cos(a),
                 middle + outer * .78f * std::sin(a), c.withAlpha(.28f + .68f * power));
        }
        dot(p, centre, middle, 4 + 10 * e, ink);
        break;
    }
    }
}
} // namespace

void draw_plugin_scene(ui::Paint& p, const PluginPanel& panel, uint32_t now,
                       float x, float y, float w, float h, bool bypass) {
    if (w <= 0 || h <= 0) return;
    if (!keep_workshop_plugin_ui(panel)) {
        draw_workshop_waveform(p, panel, x, y, w, h, bypass, Colour(style_of(panel.name).colour));
        return;
    }
    const Style style = style_of(panel.name);
    const Colour ink(style.colour);
    const auto value = [&](unsigned k) {
        const auto& param = panel.params[k];
        return param.used ? std::clamp(param.value / float(plugin_param_max), 0.f, 1.f) : .5f;
    };
    const float horizontal = .84f + .32f * value(0);
    const float vertical = .82f + .36f * value(1);
    const auto tint = ink.withRotatedHue((value(2) - .5f) * .14f);
    const Style reactive {style.name, style.form, tint.getARGB(), style.detail};
    const Signal signal {panel, bypass ? 0.f : (now % 600000u) * .001f,
                         bypass ? 0.f : peak_of(panel.scope), bypass ? 0.f : peak_of(panel.scope_right), bypass};
    juce::Graphics::ScopedSaveState saved(p.g);
    p.g.reduceClipRegion(juce::Rectangle<float>(x, y, w, h).getSmallestIntegerContainer());
    p.fill(x, y, w, h, Colour(0xff10141e), 3);
    p.g.setGradientFill(juce::ColourGradient(tint.withAlpha(.17f + .14f * signal.peak()), x + w * .5f, y + h * .5f,
                                            tint.withAlpha(0.f), x + w, y + h * .5f, true));
    p.g.fillRect(x, y, w, h);
    const float sx = (w - 4) * horizontal / width, sy = (h - 4) * vertical / 106.f;
    p.g.addTransform(juce::AffineTransform::scale(sx, sy).translated(x + w * .5f - centre * sx, y + h * .5f - middle * sy));
    draw_scene(p, reactive, signal);
}

void draw_plugin_parameter_levels(ui::Paint& p, const Snapshot& s, float x, float y, float w, float h, bool bypass) {
    const auto& panel = s.ui.plugin_panel;
    const float step = h / 3.f;
    for (unsigned k = 0; k < panel.params.size(); ++k) {
        const auto& param = panel.params[k];
        if (!param.used) continue;
        const float at = y + k * step;
        const auto accent = bypass ? colour::mid : knob_shade(s.ui.track, k);
        const auto name = juce::String::fromUTF8(param.name);
        const auto label = plain(colour::text, type::small, Weight::Medium);
        p.text("K" + juce::String(k + 1), x, at, 16, 12, caps_text(accent));
        p.text(name, x + 20, at, w - 20, 12, p.fit(name, label, w - 20, type::floor));
        p.fill(x + 20, at + 13, w - 20, 2, accent.withAlpha(.20f), 1);
        p.fill(x + 20, at + 13, (w - 20) * std::clamp(param.value / float(plugin_param_max), 0.f, 1.f), 2, accent, 1);
    }
}

bool draw_plugin_animation(ui::Paint& p, const Snapshot& s, unsigned entry, bool bypass) {
    const auto& panel = s.ui.plugin_panel;
    if (s.ui.ask != UiState::Ask::None || entry >= max_plugins || panel.entry != entry
        || panel.status != PluginStatus::Ready || !panel.live_analyzer) return false;
    if (!keep_workshop_plugin_ui(panel)) {
        if (!s.ui.shift) return false;
        draw_plugin_workshop_face(p, s, zone::picture_y, true, bypass);
        return true;
    }
    const Style style = style_of(panel.name);
    const Colour ink(style.colour);
    const Signal signal {panel, bypass ? 0.f : float(s.now % 600000u) * .001f,
                         bypass ? 0.f : peak_of(panel.scope), bypass ? 0.f : peak_of(panel.scope_right), bypass};
    juce::Graphics::ScopedSaveState saved(p.g);
    const auto night = Colour(0xff070912);
    constexpr float midpoint = zone::picture_y + middle;
    p.fill(0, 0, width, zone::height, night);
    p.g.setGradientFill(juce::ColourGradient(ink.withAlpha(.16f + .12f * signal.peak()), centre, midpoint,
                                            night.withAlpha(0.f), width, midpoint, true));
    p.g.fillRect(0.f, 0.f, width, float(zone::height));
    for (unsigned x = 8; x < zone::width; x += 16)
        for (unsigned y = 4; y < zone::height; y += 12)
            p.fill(float(x), float(y), 1, 1, ink.withAlpha(.18f));
    {
        juce::Graphics::ScopedSaveState scene(p.g);
        p.g.addTransform(juce::AffineTransform::scale(1.f, 1.16f).translated(0, midpoint - middle * 1.16f));
        draw_scene(p, style, signal);
    }
    if (!s.ui.shift) {
        // The original graph and controls draw next. Keep motion subdued in their open areas.
        p.fill(0, 0, width, zone::height, colour::bg.withAlpha(.76f));
        return false;
    }

    // Keep the standard theme's labels legible over the full-display background.
    p.fill(0, 0, width, zone::picture_y, colour::bg.withAlpha(.89f));
    p.fill(0, zone::knobs_y, width, zone::knobs_h, colour::bg.withAlpha(.82f));
    p.g.addTransform(juce::AffineTransform::translation(0, zone::picture_y));
    p.fill(0, 0, width, 32, night.withAlpha(.86f));
    p.fill(8, 30, width - 16, 1, ink.withAlpha(.28f));
    const auto name = juce::String::fromUTF8(panel.name);
    const auto title = plain(Colour(0xfff1f5ff), 14.f, Weight::SemiBold);
    p.text(name, 8, 0, 190, 19, p.fit(name, title, 190, type::floor));
    const auto caption = plain(Colour(0xffaebacd), type::small);
    const auto preset = juce::String::fromUTF8(panel.preset_name);
    if (preset.isNotEmpty()) p.text(preset, 8, 17, 188, 12, p.fit(preset, caption, 188, type::floor));
    p.text(bypass ? "BYPASSED" : "RELEASE SHIFT", 200, 2, 112, 12, caps_text(ink), juce::Justification::centredRight);
    if (panel.rows) p.text("PAGE " + juce::String(panel.row + 1) + "/" + juce::String(panel.rows),
                           201, 16, 111, 12, caption, juce::Justification::centredRight);
    // Always show measured L/R output, including on artistic displays.
    for (unsigned side = 0; side < 2; ++side) {
        const float x = side ? 244.f : 12.f, peak = side ? signal.right : signal.left;
        const auto level = peak > .001f ? juce::String(int(std::lround(20 * std::log10(peak)))) : "--";
        p.fill(x - 3, 128, 70, 15, night.withAlpha(.84f), 2);
        p.text((side ? "R " : "L ") + level, x, 129, 36, 13, caption);
        p.fill(x + 33, 135, 29, 2, ink.withAlpha(.20f));
        p.fill(x + 33, 135, 29 * std::sqrt(peak), 2, ink);
    }
    return true;
}
} // namespace melodee
