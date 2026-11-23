// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
// THE NAME SHEET: typing a name, over any page. The pads type like phone
// keys (two letters a pad, a second tap within 0.6 s for the other; pad 14
// space, pad 15 digits and signs, pad 16 delete): each pad shows both, the
// one a tap gives now bright, swapping as you tap. The wheel moves the
// cursor, Shift + wheel or K2 turns the character under it, Erase deletes
// (with Shift, everything), K1 fills in a ready name; a thin gauge under the
// name is how much of its room it takes. B1 cancels, B2 is Aa, B3 SAVE or
// Push keeps it. Opens for Save and Rename on Project, Shift +
// Push on a Song marker, Save sound, Save Sample and Save Bank (Shift +
// Sample Select).
#include "melodee/engine/engine_info.h"
#include "melodee/pages/common.h"
#include "melodee/pages/page.h"
#include <iterator>

namespace melodee {
using namespace ui;

void draw_name_sheet(Paint& p, const Snapshot& s) {
    const auto& e = s.ui.name;
    bool sound = e.target == NameTarget::Sound || e.target == NameTarget::Sample || e.target == NameTarget::Bank;
    auto accent = sound ? colour_of(s) : colour::text;
    const bool search = e.target == NameTarget::SampleSearch || e.target == NameTarget::BrowserSearch;
    // Sample Select owns the button row while its search panel is open.
    if (!search) {
        p.fill(0, 0, zone::width, zone::knobs_y, colour::bg);
        soft_keys(p, {{"CANCEL", e.caps_only ? "" : "Aa", "SAVE"}, e.lower ? 1 : 2, accent,
                      {KeyIcon::Back, KeyIcon::None, KeyIcon::Check}});
    }

    juce::String what;
    switch (e.target) {
    case NameTarget::ProjectSave: what = "SAVE PROJECT AS"; break;
    case NameTarget::ProjectSaveAs: what = "SAVE AS NEW"; break;
    case NameTarget::ProjectRename: what = "RENAME PROJECT"; break;
    case NameTarget::Marker: what = "MARKER " + juce::String::fromUTF8("\xc2\xb7 BAR ") + juce::String(s.ui.song_cursor + 1); break;
    case NameTarget::Sound: what = "SAVE SOUND " + juce::String::fromUTF8("\xc2\xb7 ") + engine_info(s.track().engine).name; break;
    case NameTarget::Sample: what = "SAVE SAMPLE"; break;
    case NameTarget::Bank: what = "SAVE BANK " + juce::String::fromUTF8("\xc2\xb7 ") + juce::String::charToString(juce::juce_wchar('A' + s.track().bank % sampler_banks)); break;
    case NameTarget::Mixdown: { // what Song's Export makes (the wheel's choice)
        static const char* const made[] {"AUDIO MIXDOWN", "AUDIO MIXDOWN", "AUDIO MIXDOWN", "STEMS", "MIDI FILE", "MPC 3 PROJECT", "ABLETON LIVE SET"};
        static_assert(std::size(made) == export_kinds::count);
        what = made[s.ui.export_kind % export_kinds::count];
        break;
    }
    case NameTarget::Scene: what = "SEQUENCE " + juce::String(s.project.scene % pattern_count + 1); break; // roadmap P5
    case NameTarget::BrowserSearch: what = "SEARCH FILES"; break;
    case NameTarget::SampleSearch: what = "SEARCH SAMPLES"; break;
    case NameTarget::Track: what = "TRACK " + juce::String(s.ui.track % track_count + 1); break;
    case NameTarget::None: break;
    }
    Header h = header_of(s);
    h.left.clear(); h.right.clear();
    h.left.push_back(pill(what, accent));
    if (!search) header(p, h);

    // The field with the cursor, and how much of its room the name takes as a thin gauge under it.
    const float left = search ? zone::width - zone::pad - 134 : zone::pad;
    const float width = zone::width - zone::pad - left;
    if (search) {
        p.fill(left, zone::picture_y + 1, width, zone::picture_h - 2, colour::surf, 4);
        p.text(e.target == NameTarget::SampleSearch ? "FIND SAMPLES" : "FIND FILES", left + 6, zone::picture_y + 4, width - 12, 14, caps_text(colour_of(s)));
    }
    float fx = left + (search ? 4 : 0), fy = zone::picture_y + (search ? 20 : 4);
    float fw = width - (search ? 8 : 0), fh = search ? 22 : 28;
    p.fill(fx, fy, fw, fh, colour::panel, 4);
    juce::String text = juce::String::fromUTF8(e.text, int(e.length));
    auto style = plain(colour::text, search ? type::list : type::title, Weight::SemiBold);
    if (!search) style = p.fit(text, style, fw - 20, type::floor);
    const float cursor = p.width(text.substring(0, e.cursor), style);
    const float offset = search ? std::max(0.f, cursor - (fw - 20)) : 0.f;
    const float tx = fx + 8 - offset;
    p.g.saveState();
    p.g.reduceClipRegion(juce::Rectangle<int>(int(fx + 4), int(fy), int(fw - 8), int(fh)));
    p.text(text, tx, fy, std::max(fw - 16, p.width(text, style) + 2), fh, style);
    p.fill(tx + cursor + 1, fy + 5, 2, fh - 10, accent);
    p.g.restoreState();
    p.fill(fx, fy + fh + 3, fw, 2, colour::line, 1);
    p.fill(fx, fy + fh + 3, fw * e.length / std::max<float>(1, e.limit), 2, colour::sec, 1);

    const bool kit_preview = e.target == NameTarget::SampleSearch && s.ui.shift && s.track().engine == EngineKind::Sampler;
    if (search)
        p.text(e.target == NameTarget::BrowserSearch ? "TYPE PART OF A NAME" : kit_preview ? "PLAY KIT PADS" : "SHIFT + PAD PLAYS KIT", fx + 2, fy + fh + 5, fw - 4, 12,
               caps_text(colour::mid, type::floor));

    // The pads as they sit: both letters, the one a tap gives now bright; the pad just hit lit.
    constexpr float gx = 4, gy = 3;
    const float top = zone::picture_y + (search ? 60 : 42);
    float cw = (fw - 3 * gx) / 4, ch = (zone::knobs_y - 1 - top - 3 * gy) / 4;
    bool fresh = e.last_pad >= 0 && s.now - e.last_at < NameEntry::retap_ms;
    for (unsigned n = 0; n < 16; ++n) {
        unsigned row = 3 - n / 4, column = n % 4;
        float x = fx + column * (cw + gx), y = top + row * (ch + gy), mid = y + ch / 2;
        if (kit_preview) {
            const auto& track = s.track();
            const unsigned kit = track.bank % sampler_banks * 16 + n;
            const bool held = s.ui.held_pads & (1u << n);
            char label[4]; pad_label(kit, label);
            p.fill(x, y, cw, ch, held ? colour_of(s) : colour::panel, 3);
            p.text(label, x, y, cw, ch, plain(held ? colour::bg : track.pads[kit].used() ? colour::text : colour::dim,
                                              type::small), juce::Justification::centred);
            continue;
        }
        bool lit = fresh && e.last_pad == int(n);
        p.fill(x, y, cw, ch, lit ? accent : colour::surf, 3);
        auto ink = lit ? colour::bg : colour::text, faint = lit ? colour::bg.withAlpha(0.5f) : colour::mid;
        if (n < 13) {
            unsigned choices = e.pad_choices(n), next = lit && choices > 1 ? (e.taps + 1u) % choices : 0;
            auto letter = [&](unsigned k) { return juce::String::charToString(juce::juce_wchar(e.pad_char(n, k))); };
            if (choices < 2) { p.text(letter(0), x, y, cw, ch, plain(ink, type::list, Weight::SemiBold), juce::Justification::centred); continue; }
            for (unsigned k = 0; k < 2; ++k)
                p.text(letter(k), x + cw * k / 2, y, cw / 2, ch, plain(k == next ? ink : faint, search ? type::small : type::list, k == next ? Weight::SemiBold : Weight::Medium),
                       juce::Justification::centred);
        } else if (n == 13) {
            icon(p, Icon::SpaceBar, x + cw / 2 - 9, mid - 9, 18, lit ? ink : colour::sec, 1.5f);
        } else if (n == 14) {
            p.text(e.digits ? "ABC" : "123", x, y, cw, ch, plain(lit ? ink : colour::sec, type::body, Weight::SemiBold), juce::Justification::centred);
        } else {
            icon(p, Icon::ArrowLeft, x + cw / 2 - 7, mid - 7, 14, lit ? ink : colour::sec, 1.6f);
        }
    }
}

} // namespace melodee
