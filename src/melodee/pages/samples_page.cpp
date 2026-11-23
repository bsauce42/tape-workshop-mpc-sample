#include "melodee/engine/filename_tempo.h"
// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
// SAMPLES (Sample Select on a sampler track): the MPC's sample browser.
// MPC drum programs (KIT) first, then instruments (XPM, XTY, SFZ), then folders, as categories, then the WAV files. The wheel and Minus /
// Plus move, Push opens a folder or loads the file onto the last pad hit
// (keeping the pad's settings; the whole file, Trim or Chop shape it), or a
// bank file (BANK, Samples/Banks) into the kit's bank in view, or an MPC drum
// program (KIT) into the kit: the LOAD KIT sheet asks which of its filled
// banks go into which of the track's banks (LOAD), or all of it as the
// track's kit, as Load Kit does (WHOLE KIT); or installs an MPC
// expansion (.xpn, INSTALL) onto the card and opens its folder; Shift +
// Sample Select here saves that bank as a bank file (Save Bank). B1
// goes back out, B2 picks the drive (the card, or the internal one: the
// factory categories and the stock app's samples, read only), B3 hears each
// file as the cursor lands on it. Shift + pad loads the file onto the
// kit's next empty pad (here instead of the printed Shift + pad jobs). K1
// jumps. The pads play the kit. Shift + B3 stars the file under the cursor;
// the card's Samples folder and its top lead with STARRED and RECENT, which
// Push opens as folders (shell/sample_lists.h). A file loaded here goes first
// on RECENT.
// jumps. The pads play the kit.
#include "melodee/shell/browser_search.h"
#include "melodee/shell/smf_jobs.h"
#include "melodee/pages/common.h"
#include "melodee/pages/pages.h"
#include "melodee/engine/dsp.h"
#include "melodee/shell/sample_library.h"
#include "melodee/shell/project_store.h"
#include "melodee/shell/sample_lists.h"
#include "melodee/shell/new_track.h"
#include "melodee/shell/project_kits.h"
#include "melodee/ui/widgets.h"
#include "melodee/ui/waveform.h"
#include <cstdio>
#include <filesystem>
#include <vector>

namespace melodee {
namespace {
using namespace ui;
constexpr unsigned visible = list_rows;
static const char* const notes[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

juce::String folder_title(const Snapshot& s) {
    if (s.files_folder == starred_list) return "STARRED";
    if (s.files_folder == recent_list) return "RECENT";
    if (s.ui.files_drive == unsigned(Drive::Internal) && s.files_top) return "CATEGORIES";
    if (s.files_folder.empty()) return "NO CARD";
    return juce::String(std::filesystem::path(s.files_folder).filename().string());
}

void draw_import_tempo(Paint& p,const Snapshot& s,const Header& header,juce::Colour accent){
    const auto& pending=s.ui.import_tempo;
    frame(p,s,SoftKeys{{"BACK",pending.detected?"APPLY":"",pending.detected?"HEAR WAV":""},-1,accent},header);
    auto text=[&](juce::String value,float x,float y,float w,float h,juce::Colour color,float size=13){p.text(value,x,y,w,h,p.fit(value,plain(color,size,Weight::Bold),w,type::floor));};
    const bool named=pending.slot>=0&&size_t(pending.slot)<s.project.samples.size()&&filename_tempo(s.project.samples[size_t(pending.slot)].path);
    text(named?"FILENAME BPM":"AUDIO BPM",10,49,142,16,accent,11);text("SEQUENCE BPM",166,49,146,16,colour::mid,11);
    if(!pending.detected){text("Detecting sample BPM...",10,90,302,25,colour::text,17);return;}
    text(juce::String(pending.detected/10.,1),10,67,142,30,colour::text,26);text(juce::String(s.tempo/10.,1),166,67,146,30,colour::text,26);
    const char* labels[]={"USE WAV BPM","TIME WARP SAMPLE","KEEP ORIGINAL"};
    juce::String details[]={"Set sequence to "+juce::String(pending.detected/10.,1)+" BPM","Keep sequence at "+juce::String(s.tempo/10.,1)+" BPM","Keep tempo and sample speed"};
    for(unsigned n=0;n<3;++n){float y=106+n*29;p.fill(8,y,304,27,n==pending.choice?accent.withAlpha(.17f):colour::raised,3);text(labels[n],13,y,294,13,n==pending.choice?accent:colour::mid,11);text(details[n],13,y+12,294,15,colour::text,12);}
    text("Data: choose / B2 or Push: apply",9,207,302,20,accent,12);
}
void apply_import_choice(Context& c){if(!c.ui.import_tempo.detected)return;unsigned choice=c.ui.import_tempo.choice;if(choice==0)c.use_detected_import_bpm();else c.finish_import_tempo(choice==1);}
// What Shift + B3 can star: a file (a sample, an instrument, a kit, a bank, a MIDI file), not a folder, a list, an
// expansion or a preset inside an .sf2. A '#' in a file's own name ("KeysPh G#m90.wav") is no preset (shell/sample_lists.h).
bool starrable(const SampleFile& f) { return !f.folder && !f.list && !f.expansion && !part_of_file(f.path); }

void draw_list(Paint& p, const Snapshot& s) {
    const auto& track = s.track();
    auto c = colour_of(s);
    Header h = header_of(s);
    Drive drive = Drive(s.ui.files_drive);
    auto drive_chip = chip(drive_name(drive), c);
    drive_chip.spare = true; // a long folder name keeps its room: B2 names the other drive
    h.left.push_back(drive_chip);
    h.left.push_back(pill(folder_title(s) + juce::String::fromUTF8(" \xc2\xb7 ") + juce::String(s.files.size()), c));
    if (track.engine == EngineKind::Sampler) { // a kit: the pad Push loads onto, the last one hit
        char label[8];
        pad_label(track.last_pad % sampler_pads, label);
        h.right.push_back(pill(Icon::ArrowRight, juce::String(label), c)); // an arrow: the rows' pad glyph marks files already on a pad
    }
    h.right.push_back(tag_of(s, s.ui.track));
    if (s.ui.expansions && s.ui.expansion_home) {
        frame(p, s, SoftKeys {{"BACK", "", "OPEN"}, 2, c}, h);
        CardSheet m;
        m.chip = "EXPANSIONS"; m.title = "PACK LIBRARY";
        m.items[0] = card_item(Icon::Folder, "MY PACKS", "INSTALLED CONTENT");
        m.items[1] = card_item(Icon::ArrowDown, "INSTALL PACK", "CHOOSE .XPN ON CARD");
        m.items[2] = card_item(Icon::Layers, "IMPORT BANK", "BANK FILE OR DRUM KIT");
        m.items[3] = card_item(Icon::Pads, "IMPORT PAD", "ONE PAD OR SAMPLE");
        m.count = 4; m.current = s.ui.expansion_row % 4;
        char target[4]; pad_label(track.last_pad % sampler_pads, target);
        m.detail = "TO T" + juce::String(s.ui.track + 1) + " / BANK " + juce::String::charToString(char('A' + track.bank % sampler_banks)) + " / " + juce::String(target);
        m.detail_colour = c; card_sheet(p, m);
        return;
    }
    if (s.ui.import_tempo.slot >= 0) {
        draw_import_tempo(p, s, h, c);
        return;
    }
    juce::String other = drive == Drive::Card ? "INTERNAL" : "CARD";
    // The file under the cursor as it previews (mock 7): its row fills as it plays; B3 lit while there is sound.
    PreviewPlace heard = s.ui.settings.preview ? preview_place(s.heads, int(sampler_pads), uint16_t(preview_slot), dsp::model_rate) : PreviewPlace {};
    SoftKeys keys {{s.files_top ? "CANCEL" : "BACK", other, s.ui.shift ? "AUTO BPM" : "PREVIEW"}, -1, c, {KeyIcon::Back, KeyIcon::None, KeyIcon::None}};
    keys.toggles = 4; // PREVIEW names a thing that is on or off: its LED, never ON / OFF (the row fills while it sounds)
    keys.lit = (s.ui.shift ? s.ui.settings.auto_import_bpm : s.ui.settings.preview) ? 4 : 0;
    if (s.ui.midi_import) {
        const auto* file = s.ui.files_cursor < s.files.size() ? &s.files[s.ui.files_cursor] : nullptr;
        const char* action = file && file->midi ? (s.pattern.events.empty() ? "IMPORT" : "REPLACE MIDI") : "";
        if (s.ui.shift) action = s.ui.midi_autoplay ? "PREVIEW ON" : "PREVIEW OFF";
        keys = SoftKeys {{s.files_top ? "CANCEL" : "BACK", other, action}, -1, c,
                        {KeyIcon::Back, KeyIcon::None, KeyIcon::None}};
        h.left.insert(h.left.begin(), chip("IMPORT MIDI", c));
        if (s.ui.shift) { keys.toggles = 4; keys.lit = s.ui.midi_autoplay ? 4 : 0; }
    }
    if (s.ui.sample_folder_pick) {
        keys = SoftKeys {{s.files_top ? "CANCEL" : "BACK", "RESET", "USE FOLDER"}, -1, c};
        h.left = {chip("SAMPLE FOLDER", c), pill(folder_title(s), c)};
        h.right.clear();
    }
    if(s.ui.sample_key_pick){
        keys=SoftKeys{{s.files_top?"CANCEL":"BACK",other,"USE KEY"},-1,c};
        h.left.insert(h.left.begin(),chip("DETECT KEY",c));
    }
    const bool sample_browser = !s.ui.midi_import && !s.ui.sample_folder_pick && !s.ui.expansions;
    const auto& search = s.sample_search;
    const auto scan_label = search.index_paused ? "Scan paused"
                           : search.preview_priority ? "Buffering audio"
                           : search.indexing ? "Indexing WAVs" : "Searching index";
    const auto matches = juce::String(search.matches) + (search.matches == 1 ? " match" : " matches");
    if (sample_browser) {
        if (!search.query.empty()) {
            keys.labels[0] = "CLEAR";
            h.left = {chip(matches, c), pill(juce::String(search.query), c)};
        }
        if (track.engine == EngineKind::Sampler) {
            char target[4];
            pad_label(track.last_pad % sampler_pads, target);
            h.right.insert(h.right.begin(), pill("PAD " + juce::String(target), c));
            // Leave room for the destination on long folder names or queries.
            h.left.back().spare = true;
        }
        if (s.ui.shift) {
            keys.labels[0] = "SEARCH";
            keys.labels[1] = "HOLD TO TALK";
            if (!search.query.empty()) {
                keys.labels[2] = "REFRESH";
                keys.toggles = keys.lit = 0;
            }
        }
    }
    if (s.ui.drums.open && s.ui.drums.browsing && sample_browser) {
        h.left = {chip("LOAD " + juce::String(drums::role_name(s.ui.drums.sound_role)).toUpperCase(), c)};
    }
    if (s.ui.expansions && s.ui.import_tempo.slot < 0) {
        const auto* file = s.ui.files_cursor < s.files.size() ? &s.files[s.ui.files_cursor] : nullptr;
        const char* action = !file ? "" : file->folder ? "OPEN" : file->expansion ? "INSTALL" : file->kit || file->bank ?
            (s.ui.expansion_action == 3 ? "LOAD PAD" : "LOAD BANK") : file->instrument || file->midi || s.ui.expansion_action == 2 ? "" : "LOAD PAD";
        keys = SoftKeys {{"BACK", "LIBRARY", s.ui.shift ? "PREVIEW" : action}, -1, c};
        if (s.ui.shift) { keys.toggles = 4; keys.lit = s.ui.settings.preview ? 4 : 0; }
        h.left = {chip("EXPANSIONS", c), pill(folder_title(s), c)};
        h.right.clear();
        char target[4]; pad_label(track.last_pad % sampler_pads, target);
        const bool sample = file && !file->folder && !file->kit && !file->bank && !file->instrument && !file->expansion && !file->midi;
        h.right.push_back(pill(file && file->expansion ? juce::String("TO CARD") : s.ui.expansion_action == 3 || sample ? "TO " + juce::String(target) : "TO BANK " + juce::String::charToString(char('A' + track.bank % sampler_banks)), c));
    }
    if (s.ui.shift && !sample_browser && !(s.ui.expansions && s.ui.expansion_home)) keys.labels[0] = "SEARCH";
    const bool search_panel = (s.ui.name.target == NameTarget::SampleSearch || s.ui.name.target == NameTarget::BrowserSearch);
    if (!browser_query(s.ui).empty() && !sample_browser) { keys.labels[0] = "CLEAR"; h.left = {pill("FIND: " + juce::String(browser_query(s.ui)), c)}; }
    if (search_panel)
        keys = SoftKeys {{"BACK", "", "DONE"}, 2, c, {KeyIcon::Back, KeyIcon::None, KeyIcon::Check}};
    if (s.ui.sheet_open) keys = {{"BACK", "", ""}, -1, c, {KeyIcon::Back, KeyIcon::None, KeyIcon::None}}; // LOAD KIT's cards
    else if (s.ui.shift) { // Shift + B3 stars the file under the cursor: its LED says whether it is starred
        const SampleFile* at = s.files.empty() ? nullptr : &s.files[std::min<size_t>(s.ui.files_cursor, s.files.size() - 1)];
        keys.labels[2] = at && starrable(*at) ? "STAR" : "";
        keys.glyphs[2] = keys.labels[2].isEmpty() ? Icon::None : Icon::Star;
        keys.toggles = 4;
        keys.lit = at && at->starred ? 4 : 0;
    }
    frame(p, s, keys, h);
    constexpr float side_width = 86;
    const bool cover_panel = s.ui.expansions && s.expansion_cover.image && s.expansion_cover.image->isValid();
    if (cover_panel) {
        constexpr float x = zone::width - zone::pad - side_width, y = zone::picture_y + 2;
        p.fill(x, y, side_width, 132, colour::panel, 4);
        p.g.drawImage(*s.expansion_cover.image, juce::Rectangle<float>(x + 3, y + 3, side_width - 6, side_width - 6), juce::RectanglePlacement::centred);
        auto title = juce::String::fromUTF8(s.expansion_cover.title.c_str());
        auto author = juce::String::fromUTF8(s.expansion_cover.author.c_str());
        auto style = plain(colour::text, type::small, Weight::SemiBold);
        p.text(title, x + 4, y + 87, side_width - 8, 18, p.fit(title, style, side_width - 8, type::floor));
        p.text(author, x + 4, y + 108, side_width - 8, 15, p.fit(author, plain(colour::mid, type::small), side_width - 8, type::floor));
    }
    if (s.browser_voice) {
        constexpr float x = zone::width - zone::pad - side_width, y = zone::picture_y + 1;
        p.fill(x, y, side_width, 62, colour::panel, 4);
        p.text("VOICE SEARCH", x + 5, y + 5, side_width - 10, 15, caps_text(c));
        p.text(s.browser_listening ? "Listening" : "Searching", x + 5, y + 21, side_width - 10, 20,
               plain(colour::text, type::list));
        if (s.browser_listening)
            p.text("Release B2", x + 5, y + 42, side_width - 10, 14, plain(colour::mid, type::small));
    }
    if (s.ui.expansions && s.files.empty()) {
        p.text("No expansion content here", zone::pad, zone::picture_y + 35, zone::width - 2 * zone::pad, 22, plain(colour::text, type::list));
        p.text("B2 LIBRARY > INSTALL PACK", zone::pad, zone::picture_y + 61, zone::width - 2 * zone::pad, 18, caps_text(c));
        return;
    }
    const bool in_list = list_folder(s.files_folder);
    if (s.files.empty()) { // a glyph and two words: no card, or nothing here
        if (s.browser_voice || search_panel) return;
        if (search.scanning) {
            constexpr float centre = zone::picture_y + 54;
            p.text(scan_label, 0, centre, zone::width, 20,
                   plain(colour::text, type::list, Weight::SemiBold), juce::Justification::centred);
            p.text(juce::String(search.checked) + " entries checked / " + matches, 0, centre + 24, zone::width, 16,
                   plain(colour::mid, type::small), juce::Justification::centred);
            return;
        }
        bool no_card = drive == Drive::Card && s.sample_root.empty();
        icon(p, no_card ? Icon::Card : Icon::Folder, zone::width / 2.0f - 14, zone::picture_y + 40, 28, colour::dim, 1.4f);
        p.text(no_card ? "NO CARD" : !search.query.empty() ? "NO MATCHES" : "EMPTY", 0, zone::picture_y + 76, zone::width, 14, caps_text(colour::dim), juce::Justification::centred);
        return;
    }
    const unsigned count = unsigned(s.files.size());
    const unsigned cursor = std::clamp<unsigned>(s.ui.files_cursor, s.files_first, s.files_first + count - 1) - s.files_first;
    const auto& selected = s.files[cursor];
    const bool scan_panel = search.scanning && !search_panel && !s.browser_voice;
    if (scan_panel) {
        constexpr float x = zone::width - zone::pad - side_width, y = zone::picture_y + 1;
        p.fill(x, y, side_width, 48, colour::panel, 4);
        p.text(scan_label, x + 5, y + 5, side_width - 10, 15, caps_text(c));
        p.text(juce::String(search.checked) + " checked", x + 5, y + 22, side_width - 10, 16,
               plain(colour::text, type::small));
    }
    const bool key_panel = !scan_panel && !search_panel && !s.browser_voice && sample_browser && !selected.folder && !selected.bank && !selected.kit
        && !selected.instrument && !selected.expansion && !selected.midi;
    if (key_panel) {
        constexpr float x = zone::width - zone::pad - side_width, y = zone::picture_y + 1;
        const auto& key = s.browser_key;
        const bool ready = key.ready && key.path == selected.path;
        const bool root = ready && key.root >= 0;
        juce::String value = "Checking";
        if (root) value = juce::String(notes[key.root%12]) + juce::String(key.root/12-1);
        else if (ready && key.tonic >= 0) value = juce::String(notes[key.tonic]) + (key.minor ? " minor" : " major");
        else if (ready) value = "Unclear";
        p.fill(x,y,side_width,62,colour::panel,4);
        p.text(root ? "ROOT NOTE" : "SAMPLE KEY",x+5,y+5,side_width-10,15,caps_text(c));
        const auto style = plain(colour::text,type::list);
        p.text(value,x+5,y+21,side_width-10,22,p.fit(value,style,side_width-10,type::floor));
        if (ready && (root || key.tonic >= 0)) p.text(s.ui.sample_key_pick&&root?"Root only":"Estimate",x+5,y+43,side_width-10,14,plain(colour::mid,type::small));
    }
    unsigned first = cursor < 2 ? 0 : std::min(cursor - 2, count > visible ? count - visible : 0);
    for (unsigned row = 0; row < visible && first + row < count; ++row) {
        const auto& file = s.files[first + row];
        ListRow r;
        r.name = name_case(juce::String(file.name)); r.accent = c; r.current = first + row == cursor;
        if (!file.presets && file.path.find(".sf2#") == std::string::npos && file.path.find(".SF2#") == std::string::npos)
            r.name = juce::String(std::filesystem::path(file.path).filename().string());
        r.fixed_name = true;
        r.name_age_ms = s.now - s.browser_selected_at;
        const juce::String path(file.path);
        juce::String format = path.endsWithIgnoreCase(".xpm") ? "XPM" : path.endsWithIgnoreCase(".xty") ? "XTY" : path.endsWithIgnoreCase(".exs") ? "EXS"
                            : path.endsWithIgnoreCase(".xpn") ? "XPN" : path.endsWithIgnoreCase(".sf2") || path.contains(".sf2#") || path.contains(".SF2#") ? "SF2"
                            : path.endsWithIgnoreCase(".dspreset") ? "DS" : "SFZ";
        r.starred = file.starred && s.files_folder != starred_list; // on STARRED every row is
        if (file.list) { r.icon = file.list == 1 ? Icon::Star : Icon::Clock; r.fact = juce::String(file.where); }
        else if (file.folder) { r.icon = Icon::Folder; r.chevron = true; }
        else if (file.bank) r.icon = Icon::Layers;
        else if (file.kit) { r.icon = Icon::Pads; r.fact = format; }
        else if (file.expansion) { r.icon = Icon::ArrowDown; r.fact = format; } // Push installs it
        else if (file.instrument) { r.icon = Icon::Keyboard; r.fact = format; }
        else if (file.midi) { r.icon = Icon::Note; r.fact = "MIDI"; } // roadmap P4: Push lays it on the pattern
        else {
            r.icon = Icon::Wav;
            if (path.endsWithIgnoreCase(".mp3")) r.fact = "MP3";
            else if (path.endsWithIgnoreCase(".flac")) r.fact = "FLAC";
            else if (path.endsWithIgnoreCase(".ogg") || path.endsWithIgnoreCase(".oga")) r.fact = "OGG";
            else if (path.endsWithIgnoreCase(".aif") || path.endsWithIgnoreCase(".aiff")) r.fact = "AIFF";
            for (unsigned k = 0; k < sampler_pads && r.pad.isEmpty(); ++k) // which pad of this kit plays it, on any layer
                for (unsigned l = 0; l < pad_layers; ++l) {
                    const PadLayer layer = track.pads[k].layer(l);
                    if (!layer.used() || file.path != s.project.samples[layer.sample].path) continue;
                    char label[4]; pad_label(k, label); r.pad = juce::String(label);
                    break;
                }
        }
        if (in_list && !file.where.empty()) r.fact = name_case(juce::String(file.where)); // the folder it is in
        if (r.current && heard.shown && !file.folder && !file.bank) { // the time heard of its length
            char now[16], of[24];
            std::snprintf(now, sizeof(now), "%.1f", heard.heard);
            std::snprintf(of, sizeof(of), " / %.1f s", heard.length);
            r.tag_lead = now; r.tag = of; r.fact = {};
            r.played = heard.through; r.played_wake = heard.wake; r.played_head = heard.strength;
        }
        p.g.saveState();
        list_row(p, zone::pad, zone::picture_y + 0.5f + row * (list_row_h + list_row_gap), zone::width - 2 * zone::pad - (search_panel ? 139 : (key_panel || scan_panel || s.browser_voice || cover_panel) ? side_width + 5 : 0), r);
        p.g.restoreState();
    }
}

// LOAD KIT's cards: LOAD (the banks picked), WHOLE KIT, then the track's banks A-H, each with the kit's bank going in.
constexpr unsigned kit_cards = 2 + sampler_banks;

juce::String pads_text(unsigned n) { return juce::String(n) + (n == 1 ? " pad" : " pads"); }

void draw_kit_sheet(Paint& p, const Snapshot& s) {
    const auto& sheet = s.ui.kit_sheet;
    const auto& track = s.track();
    auto c = colour_of(s);
    const auto have = kit_bank_pads(track);
    CardSheet m;
    m.chip = "LOAD KIT"; m.chip_colour = c; m.accent = c;
    m.title = name_case(juce::String(sheet.name));
    unsigned kit_pads = 0;
    juce::String banks;
    for (unsigned b = 0; b < sampler_banks; ++b) {
        if (!sheet.pads[b]) continue;
        kit_pads += sheet.pads[b];
        banks += juce::String::fromUTF8(" \xc2\xb7 ") + juce::String::charToString(juce::juce_wchar('A' + b)) + " " + juce::String(sheet.pads[b]);
    }
    m.detail = pads_text(kit_pads) + banks;
    unsigned picked = 0;
    for (unsigned b = 0; b < sampler_banks; ++b) if (sheet.into[b] >= 0) ++picked;
    m.items[0] = card_item(Icon::ArrowDown, "LOAD", picked == 1 ? "1 bank" : juce::String(picked) + " banks", false, false, !picked);
    bool any = false;
    for (auto n : have) any |= n > 0;
    m.items[1] = card_item(Icon::Pads, "WHOLE KIT", "all banks", false, any);
    for (unsigned b = 0; b < sampler_banks; ++b) {
        const juce::String bank = juce::String::charToString(juce::juce_wchar('A' + b));
        const int from = sheet.into[b];
        juce::String sub = from >= 0 ? "from " + juce::String::charToString(juce::juce_wchar('A' + from)) + juce::String::fromUTF8(" \xc2\xb7 ") + pads_text(sheet.pads[unsigned(from)])
                         : have[b] ? pads_text(have[b]) : juce::String("empty");
        m.items[2 + b] = card_item(Icon::Pads, "BANK " + bank, sub, from >= 0, from >= 0 && have[b] > 0);
    }
    m.count = kit_cards; m.current = s.ui.sheet_row % kit_cards;
    card_sheet(p, m);
}

void draw(Paint& p, const Snapshot& s) {
    draw_list(p, s);
    if (s.ui.sheet_open) draw_kit_sheet(p, s);
}

void wheel(Context& c, int delta) {
    if (c.ui.sheet_open) { c.ui.sheet_row = uint8_t(std::clamp<int>(c.ui.sheet_row + delta, 0, int(kit_cards) - 1)); return; }
    if (c.ui.expansions && c.ui.expansion_home) { c.ui.expansion_row = uint8_t(std::clamp(int(c.ui.expansion_row) + delta, 0, 3)); return; }
    if(c.ui.import_tempo.slot>=0){auto& pending=c.ui.import_tempo;pending.choice=uint8_t(std::clamp<int>(int(pending.choice)+delta,0,2));return;}
    int was = c.ui.files_cursor;
    c.ui.files_cursor = uint16_t(std::clamp<int>(c.ui.files_cursor + delta, 0, std::max(int(c.ui.files_count) - 1, 0)));
    if (c.ui.files_cursor != was) c.preview_cursor();
}

void nudge(Context& c, int delta) {
    const auto& pending = c.ui.import_tempo;
    if (pending.slot < 0) { wheel(c, delta); return; }
    if (!pending.detected) return;
    const int current = int(c.project.tempo_of(c.ui.lead_pattern));
    const int next = std::clamp(current + delta, 300, 3000);
    if (next != current) c.set_tempo(next);
}

void push(Context& c) {
    if(c.ui.sample_key_pick){
        if(!c.library||c.library->browse_pending())return;
        const auto selected=c.library->file_at(c.ui.files_cursor);
        if(!selected){c.notice("Choose an audio sample");return;}
        if(selected->folder){c.ui.browser_queries[2].clear();c.library->browse(Drive(c.ui.files_drive),selected->path);return;}
        if(selected->instrument||selected->midi||selected->bank||selected->kit||selected->expansion){c.notice("Choose an audio sample");return;}
        c.library->analyse_key(selected->path);c.notice("Key analysis; B3 applies the estimate");return;
    }
    if (c.ui.sheet_open) { // LOAD KIT's cards
        unsigned card = c.ui.sheet_row % kit_cards;
        if (card == 0) c.load_kit_banks();
        else if (card == 1) {
            c.ui.sheet_open = false;
            if (auto from = std::move(c.kit_held)) c.load_kit(*from, c.ui.kit_sheet.name);
        } else c.kit_sheet_step(card - 2);
        return;
    }
    if (c.ui.expansions && c.ui.expansion_home) { c.browse_expansion(c.ui.expansion_row); return; }
    if (c.ui.import_tempo.slot >= 0) {
        apply_import_choice(c);
        return;
    }
    if (!c.library || c.library->browse_pending()) return;
    std::optional<SampleFile> selected;
    if (c.ui.midi_import || c.ui.sample_folder_pick) {
        const auto files = c.library->files(c.ui.midi_import, c.ui.sample_folder_pick);
        if (c.ui.files_cursor < files.size()) selected = files[c.ui.files_cursor];
    } else {
        selected = c.library->file_at(c.ui.files_cursor);
    }
    if (!selected) { c.notice("No file selected"); return; }
    const auto& file = *selected;
    if (c.ui.expansions) {
        if (file.list) c.library->browse(Drive::Card, file.path);
    else if (file.folder) { c.ui.browser_queries[2].clear(); c.library->browse(Drive::Card, file.path); }
        else if (file.expansion) { c.ui.expansion_action = 0; c.library->install(file.path); }
        else if (file.bank || file.kit) c.load_expansion_file(file.path, file.name, file.bank);
        else if (file.instrument || file.midi) c.notice("Choose a drum kit, bank or sample");
        else if (c.ui.expansion_action == 2) c.notice("Choose a bank file or drum kit");
        else c.load_file(file.path, file.name, false);
        return;
    }
    if(c.ui.drums.open&&c.ui.drums.browsing&&!file.folder&&(file.instrument||file.midi||file.bank||file.kit||file.expansion)){c.notice("Choose an audio sample for the drum role");return;}
    if (c.ui.midi_import && file.midi) {
        if (!c.pattern().events.empty()) { c.notice("B3 replaces this pattern's MIDI"); return; }
        if (!c.frozen_edit(c.ui.track) && import_midi(c, file.path)) c.back();
        return;
    }
    if (file.list) c.library->browse(Drive::Card, file.path);
    else if (file.folder) { c.ui.browser_queries[2].clear(); c.library->browse(Drive(c.ui.files_drive), file.path); }
    else if (file.bank) c.load_bank_file(file.path, file.name);
    else if (file.kit) c.load_kit_file(file.path, file.name, c.track().engine == EngineKind::Sampler); // a kit: LOAD KIT asks which banks
    else if (file.expansion) c.library->install(file.path);
    else if (file.midi) import_midi(c, file.path); // roadmap P4: its notes onto the track's pattern
    else c.load_file(file.path, file.name, false);
    if (!file.folder && !file.expansion && !file.midi) c.library->used(file.path); // first on RECENT
}

void soft(Context& c, int key) {
    auto& ui = c.ui;
    if (ui.sheet_open) { if (key == 0) { c.kit_held.reset(); c.back(); } return; } // LOAD KIT's cards: B1 BACK
    if (!c.library) return;
    if(ui.sample_key_pick){
        if(key==0){
            if(ui.shift){c.open_name(NameTarget::SampleSearch,c.library->query(),31,true);return;}
            if(!browser_query(ui).empty()){ui.browser_queries[2].clear();c.library->filter_folder("");return;}
            if(c.library->top()){const auto back=ui.sample_key_back;ui.sample_key_pick=false;c.open(back);}else c.library->browse_up();return;
        }
        if(key==2){
            const auto selected=c.library->file_at(ui.files_cursor);const auto detected=c.library->sample_key();
            if(!selected||selected->folder||detected.path!=selected->path||!detected.ready){if(selected&&!selected->folder)c.library->analyse_key(selected->path);c.notice("Waiting for sample key analysis");return;}
            if(detected.tonic<0&&detected.root<0){c.notice("Key unclear; choose a tonal sample");return;}
            c.apply({Binding::Kind::Root,0,0,11,c.project.root},detected.root>=0?detected.root%12:detected.tonic);
            if(detected.root<0)c.apply({Binding::Kind::Scale,0,0,12,c.project.scale},detected.minor?1:0);
            const auto back=ui.sample_key_back;ui.sample_key_pick=false;c.open(back);
            c.notice(detected.root>=0?"Project root applied; scale kept":"Estimated project key and scale applied");return;
        }
    }
    if (key == 0 && !browser_query(ui).empty()) { ui.browser_queries[2].clear(); c.library->filter_folder(""); ui.files_cursor = 0; return; }
    if (ui.import_tempo.slot >= 0) {
        if (key == 0) c.finish_import_tempo(false, ui.shift || !ui.import_tempo.detected);
        if(ui.import_tempo.detected&&key==1)apply_import_choice(c);
        if (ui.import_tempo.detected && key == 2) c.preview_import_tempo(c.ui.shift);
        return;
    }
    if (ui.expansions) {
        if (ui.expansion_home) {
            if (key == 0) {
                if (ui.new_kit == int8_t(ui.track) && ui.new_for == int8_t(ui.track)) {
                    ui.expansions = ui.expansion_home = false;
                    samples_cancel(c);
                } else c.back();
            }
            else if (key == 2) c.browse_expansion(ui.expansion_row);
            return;
        }
        if (key == 1) { ui.browser_queries[2].clear(); c.library->filter_folder(""); c.stop_preview(); c.library->preview(""); ui.expansion_home = true; return; }
        if (c.library->browse_pending()) return;
        if (key == 0) {
            if (c.library->top()) ui.expansion_home = true;
            else c.library->browse_up();
        }
        if (key == 2) {
            if (ui.shift) { ui.settings.preview = !ui.settings.preview; c.preview_cursor(); }
            else push(c);
        }
        return;
    }
    if (ui.sample_folder_pick) {
        if (c.library->browse_pending()) return;
        if (key == 0) { if (c.library->top()) c.back(); else c.library->browse_up(); }
        if (key == 1) c.save_sample_folder(true);
        if (key == 2) c.save_sample_folder(false);
        return;
    }
    if (ui.midi_import && key == 2) {
        const auto files = c.library->files(true);
        if (ui.shift) {
            ui.midi_autoplay = !ui.midi_autoplay;
            c.preview_cursor();
            return;
        }
        if (ui.files_cursor < files.size() && files[ui.files_cursor].midi &&
            !c.frozen_edit(ui.track) && import_midi(c, files[ui.files_cursor].path)) c.back();
        return;
    }
    if (ui.midi_import && (key == 0 || key == 1)) c.stop_preview();
    if (key == 0 && !ui.midi_import && !ui.sample_folder_pick) {
        if (ui.shift) { c.open_name(NameTarget::SampleSearch, c.library->query(), 31, true); return; }
        if (!c.library->query().empty()) { c.search_samples(""); return; }
    }
    if (key == 0) { if (c.library->top()) samples_cancel(c); else c.library->browse_up(); } // at the top: CANCEL
    if (key == 1) { // the other drive, where it was left
        unsigned from = ui.files_drive % unsigned(Drive::Count), to = (from + 1) % unsigned(Drive::Count);
        c.files_places[from] = {c.library->folder(), ui.files_cursor, true};
        const auto& back = c.files_places[to];
        c.files_row_wanted = back.set ? back.row : -1;
        ui.files_drive = uint8_t(to);
        c.library->browse(Drive(to), back.set ? back.folder : std::string());
    }
    if (key == 2 && ui.shift) { // Shift + B3: the file under the cursor starred, or no longer
        if (!c.library->query().empty()) { c.stop_preview(); c.library->preview(""); c.library->refresh_search(); ui.files_cursor = 0; return; }
        auto files = c.library->files();
        if (ui.files_cursor < files.size() && starrable(files[ui.files_cursor])) c.library->star(files[ui.files_cursor].path);
        else c.notice("Star: a file");
        return;
    }
    if (key == 2) { // its LED says which; kept with the settings, as it was left
        ui.settings.preview = !ui.settings.preview;
        if (c.store) c.store->save_settings(ui.settings.text());
        c.preview_cursor();
    }
}
} // namespace

// Shift + pad on the browser: the file under the cursor onto the next empty pad.
bool samples_shift_pad(Context& c) {
    if (c.ui.page != PageId::Samples || !c.library || c.track().engine != EngineKind::Sampler) return false;
    if (c.ui.import_tempo.slot >= 0 || c.ui.midi_import || c.ui.sample_key_pick || c.ui.sample_folder_pick || c.ui.expansions || c.library->browse_pending()) return true;
    const auto selected = c.library->file_at(c.ui.files_cursor);
    if (!selected || selected->folder) { c.notice("Shift + pad: a file first"); return true; }
    const auto& file = *selected;
    if (file.bank) { c.notice("Push loads a bank"); return true; }
    if (file.kit) { c.notice("Push loads a kit"); return true; }
    if (file.expansion) { c.notice("Push installs it"); return true; }
    if (file.midi) { c.notice("Push lays it on the pattern"); return true; }
    c.load_file(file.path, file.name, true);
    c.library->used(file.path); // first on RECENT
    return true;
}

const PageSpec samples_page {draw, play_pad, wheel, push, soft, nudge};

} // namespace melodee
