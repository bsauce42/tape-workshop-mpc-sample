// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
// PLUG-INS (docs/MELODEE.md, Plug-ins): the card's Plugins folder, opened for
// a PLUGIN track's instrument (Sample Select on it, or PLUGIN chosen on K1 of
// Sounds) or for a chain's slot (the effect picker's Plug-ins family). Folders
// first, then the plug-ins, each with its format and whether it is an
// instrument or an effect (found by opening it once, so it fills in a moment
// after the folder lists). A bundle holding several plug-ins (LSP's) lists as
// one row with how many, and Push opens it: a row for each. One that cannot
// run here says why: a Windows or Mac download, a PC build. The wheel and Minus / Plus move, Push opens a folder
// or loads the plug-in (on the track, or into the slot) and goes back; B1
// goes up a folder, past the Plugins folder as far as the card's top (a
// plug-in anywhere on the card can be picked), and there back where it came
// from. The pads play the track.
#include "melodee/shell/browser_search.h"
#include "melodee/pages/common.h"
#include "melodee/pages/pages.h"
#include "melodee/pages/mixer_look.h"
#include "melodee/plugin/plugin_rack.h"
#include "melodee/ui/widgets.h"
#include "melodee/ui/plugin_preview_art.h"
#include <filesystem>

namespace melodee {
namespace {
using namespace ui;
constexpr unsigned visible = list_rows;

// Cache only the selected artwork on the screen thread. No rack probes, DSP,
// file reads or image decoding occur on the audio thread.
const juce::Image& preview_image(const PluginFile& file) {
    static thread_local std::string selected;
    static thread_local juce::Image image;
    auto key = file.path + ":" + std::to_string(file.id) + ":" + file.name;
    if (key == selected) return image;
    selected = key;
    image = {};
    const auto library = std::filesystem::path(file.library).filename().string();
    for (const auto& art : plugin_preview_art) {
        if (library == art.library || file.name == art.name || file.name == std::string(art.name) + " (TW)"
            || std::filesystem::path(file.path).filename() == std::string("Tape Workshop - VST - ") + art.name) {
            juce::MemoryOutputStream png;
            if (juce::Base64::convertFromBase64(png, art.png)) image = juce::ImageFileFormat::loadFrom(png.getData(), png.getDataSize());
            break;
        }
    }
    return image;
}

void preview_card(Paint& p, const PluginFile& file, juce::Colour accent) {
    constexpr float x = 180, y = zone::picture_y + 3, w = 134;
    p.fill(x, y, w, 138, colour::panel, 4);
    const auto& image = preview_image(file);
    if (image.isValid() && !file.folder) {
        p.g.drawImage(image, juce::Rectangle<float>(x + 3, y + 3, w - 6, 96), juce::RectanglePlacement::centred);
    } else {
        // A clear generic card for third-party plug-ins without bundled art;
        // folders stay recognisable and are never presented as a plug-in UI.
        icon(p, file.folder ? Icon::Folder : file.instrument == 1 ? Icon::Keyboard : Icon::Sliders,
             x + 43, y + 25, 48, accent, 1.5f);
    }
    auto style = plain(colour::text, type::small, Weight::Bold);
    auto lines = p.wrap(juce::String(file.name), style, w - 10, 2);
    for (int i = 0; i < std::min(2, lines.size()); ++i)
        p.text(lines[i], x + 5, y + 101 + i * 14, w - 10, 14, style, juce::Justification::centred);
}

// What the picker loads into, for the header: the track, or the chain and its slot.
juce::String target_text(const Snapshot& s) {
    if (s.ui.plugin_target == UiState::PluginTarget::Instrument) return "INSTRUMENTS";
    unsigned n = s.ui.plugin_chain;
    juce::String where = n < track_count ? "TRACK" : n < master_chain ? juce::String("BUS ") + char('A' + (n - track_count) % bus_count)
                       : n == master_chain ? juce::String("MASTER") : juce::String("LOOP");
    return where + " FX " + juce::String(s.ui.plugin_slot + 1);
}

// The entry the target holds now, so its plug-in can say LOADED.
const PluginRef* loaded_ref(const Snapshot& s) {
    if (s.ui.plugin_target == UiState::PluginTarget::Instrument) {
        const auto& t = s.track();
        return t.engine == EngineKind::Plugin && t.plugin < max_plugins ? &s.project.plugins[t.plugin] : nullptr;
    }
    const auto& chain = s.project.chain(s.ui.plugin_chain);
    if (s.ui.plugin_slot >= chain.size) return nullptr;
    const auto& slot = chain.slots[s.ui.plugin_slot];
    if (slot.kind != plugin_effect_kind || unsigned(slot.params[0]) >= max_plugins) return nullptr;
    return &s.project.plugins[size_t(slot.params[0])];
}

void draw(Paint& p, const Snapshot& s) {
    if (s.ui.mixer_fx && s.ui.plugin_target == UiState::PluginTarget::Effect && !s.ui.name.open()) {
        mixer_ui::Look look;
        mixer_ui::header(p, "ADD PLUG-IN", target_text(s), {"BACK", "", "LOAD"});
        const auto& view = s.plugins; unsigned count = unsigned(view.files.size());
        mixer_ui::label(p, view.root.empty() ? "NO CARD" : "PLUG-IN LIBRARY", 8, 51, 304, 18, 11, colour::mid);
        unsigned cursor = count ? std::min<unsigned>(s.ui.plugins_cursor, count - 1) : 0, first = cursor / 4 * 4;
        for (unsigned row = 0; row < 4 && first + row < count; ++row) {
            const auto& file = view.files[first + row]; bool selected = first + row == cursor; float y = 73 + row * 27;
            p.fill(8, y, 304, 23, selected ? colour::raised : colour::panel, 3);
            if (selected) p.ring(8, y, 304, 23, mixer_ui::ink(), 3);
            mixer_ui::label(p, file.name, 18, y + 2, 267, 20, 12, file.folder || file.loadable() ? colour::text : colour::dim);
            mixer_ui::label(p, ">", 287, y + 2, 18, 20, 12, mixer_ui::ink());
        }
        if (!count) mixer_ui::label(p, "No effects found", 16, 98, 288, 28, 14, colour::mid);
        mixer_ui::cards(p, s); return;
    }
    auto c = s.ui.plugin_target == UiState::PluginTarget::Instrument ? colour_of(s) : colour::text;
    const auto& v = s.plugins;
    Header h = header_of(s);
    juce::String folder = v.root.empty() ? juce::String("NO CARD") : v.home ? juce::String("PLUGINS")
                        : v.top ? juce::String("CARD") : juce::String(std::filesystem::path(v.folder).stem().string());
    h.left.push_back(chip(target_text(s), c));
    if (s.ui.plugin_target == UiState::PluginTarget::Effect && v.home) folder = "EFFECTS";
    h.left.push_back(pill(folder + juce::String::fromUTF8(" \xc2\xb7 ") + juce::String(int(v.files.size())), c));
    if (s.ui.plugin_target == UiState::PluginTarget::Instrument) h.right.push_back(tag_of(s, s.ui.track));
    frame(p, s, {{s.ui.shift ? "SEARCH" : !browser_query(s.ui).empty() ? "CLEAR" : v.top ? "CANCEL" : "BACK", "", ""}, -1, c, {KeyIcon::Back, KeyIcon::None, KeyIcon::None}}, h);
    if (s.ui.name.target == NameTarget::BrowserSearch) soft_keys(p, {{"BACK", "", "DONE"}, 2, c});
    if (v.files.empty()) { // a glyph and two words: no card, or an empty Plugins folder
        float mid = zone::width / 2.0f, y = zone::picture_y + 40;
        icon(p, v.root.empty() ? Icon::Card : Icon::Folder, mid - 14, y, 28, colour::dim, 1.4f);
        p.text(v.root.empty() ? "NO CARD" : !browser_query(s.ui).empty() ? "NO MATCHES" : "NO PLUG-INS", 0, y + 36, zone::width, 14, caps_text(colour::dim), juce::Justification::centred);
        if (!v.root.empty() && v.home) // where they go: unzipped plug-in folders in the card's Melodee/Plugins folder
            p.text(juce::String::fromUTF8("Card \xe2\x80\xba Melodee \xe2\x80\xba Plugins"), 0, y + 54, zone::width, 14, plain(colour::dim, type::small), juce::Justification::centred);
        return;
    }
    const auto* ref = loaded_ref(s);
    std::string loaded = ref ? ref->path : "";
    int32_t loaded_id = ref ? ref->id : 0;
    unsigned count = unsigned(v.files.size()), cursor = std::min<unsigned>(s.ui.plugins_cursor, count - 1);
    unsigned first = cursor < 2 ? 0 : std::min(cursor - 2, count > visible ? count - visible : 0);
    if (s.ui.name.target != NameTarget::BrowserSearch) preview_card(p, v.files[cursor], c);
    for (unsigned row = 0; row < visible && first + row < count; ++row) {
        const auto& file = v.files[first + row];
        ListRow r;
        r.name = juce::String(file.name);
        r.accent = c;
        r.current = first + row == cursor;
        if (file.folder) { r.icon = Icon::Folder; r.chevron = true; }
        else if (file.several()) { r.icon = Icon::Plug; r.fact = juce::String(int(file.plugins)); r.chevron = true; } // a bundle of several
        else if (!file.loadable()) { r.icon = Icon::PlugBroken; r.fact = juce::String(file.why); r.dim = true; }
        else {
            r.icon = Icon::Plug;
            r.fact = format_tag(file.format);
            if (!loaded.empty() && file.path == loaded && file.id == loaded_id) r.fact_icon = Icon::Check; // the one loaded
            else if (file.instrument >= 0) r.fact_icon = file.instrument ? Icon::Keyboard : Icon::Sliders;
        }
        p.g.saveState();
        list_row(p, zone::pad, zone::picture_y + 0.5f + row * (list_row_h + list_row_gap), 168, r);
        p.g.restoreState();
    }
}

#ifndef PLUGIN_BROWSER_PREVIEW
void wheel(Context& c, int delta) {
    auto view = c.plugins ? c.plugins->view(c.ui.plugin_target == UiState::PluginTarget::Instrument) : PluginRack::View {};
    filter_browser_files(view.files, browser_query(c.ui));
    c.ui.plugins_cursor = uint16_t(std::clamp<int>(c.ui.plugins_cursor + delta, 0, std::max(int(view.files.size()) - 1, 0)));
}

void push(Context& c) {
    if (!c.plugins) return;
    auto view = c.plugins->view(c.ui.plugin_target == UiState::PluginTarget::Instrument);
    filter_browser_files(view.files, browser_query(c.ui));
    if (c.ui.plugins_cursor >= view.files.size()) return;
    const auto picked = view.files[c.ui.plugins_cursor];
    if (picked.folder || picked.several()) c.ui.browser_queries[1].clear();
    c.choose_plugin(picked);
}

void soft(Context& c, int key) {
    if (c.ui.mixer_fx && key == 2) { push(c); return; }
    if (key != 0 || !c.plugins) return;
    if (!browser_query(c.ui).empty()) { c.ui.browser_queries[1].clear(); c.ui.plugins_cursor = 0; return; }
    if (c.plugins->view().top) { // CANCEL: an instrument not picked yet leaves the track as it was, on the page it came from
        bool unpicked = c.ui.plugin_target == UiState::PluginTarget::Instrument && c.track().engine != EngineKind::Plugin;
        c.open(unpicked ? c.ui.plugin_from : c.ui.plugin_back);
        return;
    }
    c.plugin_came_back = false; // the cursor lands on the folder it came out of
    c.plugins->browse_up();
}
#endif
} // namespace

#ifndef PLUGIN_BROWSER_PREVIEW
const PageSpec plugins_page {draw, play_pad, wheel, push, soft, wheel}; // Minus / Plus move the list

#endif
} // namespace melodee
