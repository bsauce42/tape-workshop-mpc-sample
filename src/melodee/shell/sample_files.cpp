// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
// Sample files (parity phase 5), on the MIDI thread: what the MPC makes
// from a pad's audio and what its browser loads.
//
// - Trim Sample (Shift + Pad 13): the last pad's region becomes a file of
//   its own ("NAME-Trimmed"); the pad then plays all of it. One undo, which
//   puts the pad back on the old file and region.
// - Extract (Shift + B1 on Chop): the slice in hand as a file ("NAME-Slice
//   n"), onto the next empty pad. One undo.
// - Save Sample (Shift + Sample Select on a kit): the region as
//   Samples/<name>.wav, named on the Name sheet; the pad is left as it is.
// - Loading a file from the browser onto the last pad keeps the pad's
//   settings (volume, tune, envelopes, filter ...; its region, loop point,
//   Normalize and # Beats start again, as they belong to the old file);
//   onto an empty pad (Shift + pad: the next one) it starts at the defaults.
//   One undo.
//
// The service thread writes the files (SampleLibrary::write()) into a slot
// held for them; files_landed() puts them on their pads once they are in.
// Melodee writes only to the card: Trim and Extract go to Samples/Edits;
// without a card they stay in memory for the session.
#include "melodee/engine/filename_tempo.h"
#include "melodee/shell/browser_search.h"
#include "melodee/shell/typed_sample_search.h"
#include "melodee/shell/context.h"
#include "melodee/shell/new_track.h"
#include "melodee/shell/sample_library.h"
#include "melodee/shell/project_store.h"
#include "melodee/shell/stage_map.h"
#include <cstdio>
#include <filesystem>
#include <cstring>
#include <algorithm>
#include <utility>

namespace melodee {
namespace {
constexpr const char* writing = "(writing)"; // a slot's path while its file is made

const char* edits_folder = "Edits";

// A free sample slot held for a file being made, -1 when every slot is in use.
int hold_slot(Project& project, const std::string& name) {
    for (unsigned n = 0; n < sample_slots; ++n) {
        if (project.samples[n].used()) continue;
        auto& ref = project.samples.edit(n);
        std::snprintf(ref.path, sizeof(ref.path), "%s", writing);
        std::snprintf(ref.name, sizeof(ref.name), "%s", name.c_str());
        ref.frames = 0;
        if (project.chops[n].used()) project.chops.edit(n) = SampleChop {}; // roadmap P3: no chop of what the slot held before
        return int(n);
    }
    return -1;
}

SampleJob job(SampleJob::Kind kind, unsigned source, int target, uint32_t from, uint32_t to, const std::string& name, const char* folder) {
    SampleJob j;
    j.kind = kind; j.source = uint16_t(source); j.target = uint16_t(std::max(target, 0));
    j.from = from; j.to = to;
    std::snprintf(j.name, sizeof(j.name), "%s", name.c_str());
    std::snprintf(j.folder, sizeof(j.folder), "%s", folder);
    return j;
}

// The pad's name with a word after it, fitting a slot's name.
std::string derived(const SampleRef& ref, const std::string& tail) {
    std::string base = ref.name[0] ? ref.name : "SAMPLE";
    size_t room = sizeof(ref.name) - 1 > tail.size() ? sizeof(ref.name) - 1 - tail.size() : 0;
    return base.substr(0, room) + tail;
}
} // namespace

int Context::hold_file(const std::string& name, unsigned t, int pad, uint8_t land) {
    unsigned free = 0;
    while (free < pending_files.size() && pending_files[free].slot >= 0) ++free;
    int slot = free < pending_files.size() ? hold_slot(project, name) : -1;
    if (slot < 0) return -1;
    pending_files[free] = {int16_t(slot), uint8_t(t % track_count), int16_t(pad), false, PadSound {}, land};
    return slot;
}

void Context::drop_file(int slot) {
    if (slot < 0 || slot >= int(sample_slots)) return;
    project.samples.edit(size_t(slot)) = SampleRef {};
    for (auto& p : pending_files) if (p.slot == slot) p = PendingFile {};
}

int Context::next_empty_pad(unsigned after) const {
    const auto& t = project.tracks[ui.track % track_count];
    for (unsigned k = 1; k <= sampler_pads; ++k) {
        unsigned n = (after + k) % sampler_pads;
        if (!t.pads[n].used()) return int(n);
    }
    return -1;
}

void Context::trim_sample() {
    auto& t = track();
    unsigned kit = t.last_pad % sampler_pads;
    const auto& pad = t.pads[kit];
    if (t.engine != EngineKind::Sampler || !pad.used()) { notice("Trim Sample: no sample"); return; }
    if (refuse_instrument("Trim Sample")) return;
    if (!library) return;
    const auto& ref = project.samples[pad.sample % sample_slots];
    uint32_t frames = ref.frames, end = pad.end && pad.end < frames ? pad.end : frames;
    if (!frames) { notice("Trim Sample: still loading"); return; }
    if (pad.start == 0 && end >= frames) { notice("Trim Sample: nothing to cut"); return; }
    unsigned free = 0;
    while (free < pending_files.size() && pending_files[free].slot >= 0) ++free;
    std::string name = derived(ref, "-Trimmed");
    int slot = free < pending_files.size() ? hold_slot(project, name) : -1;
    if (slot < 0) { notice(free < pending_files.size() ? "Trim Sample: samples full" : "Trim Sample: busy"); return; }
    if (!library->write(job(SampleJob::Kind::Derive, pad.sample, slot, pad.start, end, name, edits_folder))) {
        project.samples.edit(size_t(slot)) = SampleRef {};
        notice("Trim Sample: busy");
        return;
    }
    pending_files[free] = {int16_t(slot), ui.track, int16_t(kit), true, pad};
    notice(pad_text("Trimming", kit));
}

void Context::extract_slice() {
    auto& t = track();
    const auto& sl = ui.slices;
    uint32_t frames = ui.chop_sample < sample_slots ? project.samples[ui.chop_sample].frames : 0;
    if (t.engine != EngineKind::Sampler || !frames || ui.chop_slice >= sl.count) { notice("Extract: no slice"); return; }
    if (!library) return;
    unsigned free = 0;
    while (free < pending_files.size() && pending_files[free].slot >= 0) ++free;
    const auto& ref = project.samples[ui.chop_sample];
    std::string name = derived(ref, "-Slice " + std::to_string(ui.chop_slice + 1));
    int slot = free < pending_files.size() ? hold_slot(project, name) : -1;
    if (slot < 0) { notice(free < pending_files.size() ? "Extract: samples full" : "Extract: busy"); return; }
    SampleJob slice = job(SampleJob::Kind::Derive, ui.chop_sample, slot, sl.start[ui.chop_slice], sl.end(ui.chop_slice, frames), name, edits_folder);
    slice.edges = true; // faded where it was cut, as the pads play it
    if (!library->write(slice)) {
        project.samples.edit(size_t(slot)) = SampleRef {};
        notice("Extract: busy");
        return;
    }
    pending_files[free] = {int16_t(slot), ui.track, int16_t(t.last_pad % sampler_pads), false, PadSound {}};
    notice("Extracting slice " + std::to_string(ui.chop_slice + 1));
}

void Context::save_sample_as(const std::string& name) {
    const auto& t = track();
    const auto& pad = t.pads[t.last_pad % sampler_pads];
    if (t.engine != EngineKind::Sampler || !pad.used() || !library) { notice("Save Sample: no sample"); return; }
    if (library->root().empty()) { notice("Save Sample: no card"); return; }
    if (!library->write(job(SampleJob::Kind::Save, pad.sample, -1, pad.start, pad.end, name, ""))) { notice("Save Sample: busy"); return; }
    notice("Saving " + name);
}

// Hold the destination while audio is read. A failed search result must
// leave the sound already on the pad playable.
void Context::load_file(const std::string& path, const std::string& name, bool next_empty) {
    const auto& t = track();
    if (!library || !t.sampled() || frozen_edit(ui.track)) return;
    const bool keyed = t.engine == EngineKind::Keygroup;
    if (keyed && ui.layer_load > 0) { ui.layer_load = -1; notice("Keygroup has one sound"); return; }
    if (browser_load.slot >= 0) { notice("Sample still loading"); return; }
    if (path.size() >= sizeof(SampleRef::path)) { notice("Sample path too long"); return; }
    const bool drum_pick=ui.drums.open&&ui.drums.browsing&&ui.drums.owner==ui.track;
    if(drum_pick)next_empty=false;
    const int kit = drum_pick?int(ui.drums.sound_pad):keyed ? 0 : next_empty ? next_empty_pad(t.last_pad) : int(t.last_pad % sampler_pads);
    if (kit < 0) { notice("No empty pad"); return; }
    const unsigned layer = ui.layer_load > 0 && !next_empty ? unsigned(ui.layer_load) % pad_layers : 0;
    if (layer) {
        SampleRef asked;
        std::memcpy(asked.path, path.c_str(), path.size() + 1);
        if (asked.instrument()) { notice("Layers take samples"); return; }
        if (!t.pads[kit].used()) { notice("Layer 1 first"); return; }
    }
    const auto existing = std::find_if(project.samples.begin(), project.samples.end(),
                                       [&](const SampleRef& ref) { return path == ref.path; });
    const bool allocated = existing == project.samples.end();
    int slot = sample_slot(path, name);
    if (slot < 0 && free_unused_samples()) slot = sample_slot(path, name);
    if (slot < 0) { notice("Samples full"); return; }
    if (!allocated) {
        const auto previous = library->slot(unsigned(slot));
        if (previous.path == path && !previous.error.empty() && !library->load(unsigned(slot), path)) {
            notice("Sample loader busy");
            return;
        }
    }
    stop_preview();
    library->preview("");
    if(drum_pick)ui.drums.sound_load_requested=true;
    browser_load = {slot, ui.track, uint8_t(kit), uint8_t(layer), t.pads[kit].sample, next_empty, allocated};
    notice(pad_text("Loading", unsigned(kit)));
    browser_load_landed();
}

void Context::browser_load_landed() {
    if (!library || browser_load.slot < 0) return;
    const int slot = browser_load.slot;
    const auto loaded = library->slot(unsigned(slot));
    if (loaded.path != project.samples[size_t(slot)].path || (!loaded.ready && loaded.error.empty())) return;
    const auto pending = std::exchange(browser_load, BrowserLoad {});
    if (!loaded.ready) {
        if (pending.allocated) drop_file(slot);
        ui.drums.sound_load_requested=false;
        notice("Load failed: " + loaded.error);
        return;
    }
    auto& t = project.tracks[pending.track];
    const unsigned kit = pending.pad;
    const bool keyed = t.engine == EngineKind::Keygroup;
    if (!t.sampled() || t.pads[kit].sample != pending.previous_sample) {
        notice("Pad changed during load");
        return;
    }
    if (frozen_edit(pending.track)) return;
    const bool still_browsing = ui.page == PageId::Samples && ui.track == pending.track &&
        !ui.name.open() && !ui.midi_import && !ui.sample_folder_pick;
    Edit edit;
    edit.track = pending.track; edit.pad = int16_t(kit);
    edit.pad_before = pad_state(t, kit);
    edit.pad_after = edit.pad_before;
    if (pending.layer) {
        auto& layer = edit.pad_after.sound.layers[pending.layer - 1];
        if (!layer.used()) layer = PadLayer {};
        layer.sample = uint16_t(slot); layer.start = layer.end = 0;
        edit.what = pad_text("Load", kit) + " L" + std::to_string(pending.layer + 1);
        restore_pad(pending.track, kit, edit.pad_after);
        edits.push(std::move(edit));
        notice(pad_text("Loaded", kit) + " layer " + std::to_string(pending.layer + 1));
        if (still_browsing && ui.layer_load == pending.layer) back();
        return;
    }
    auto& sound = edit.pad_after.sound;
    bool keep = sound.used() && !pending.next_empty;
    if (keep) { // the pad's settings stay; what belonged to the old file starts again
        sound.start = sound.end = sound.loop_start = 0;
        sound.loop_lock = true;
        sound.gain = 0; sound.own_tempo = 0; sound.beats = 0;
    } else sound = PadSound {};
    if (sound.sample != uint16_t(slot)) edit.pad_after.trims.clear(); // round 3: another file's zones are not these
    sound.sample = uint16_t(slot);
    if (loaded.instrument) { // played on keys: held while held, no region of its own
        sound.play = PadPlay::Gate; sound.decay = 100; sound.sustain = 100;
        if (!keep) sound.mono = false; // a fresh instrument pad is Poly (chords on Keys); a kept pad keeps its Mono
        sound.loop = false; sound.warp = warp_off; sound.beats = 0;
        const PadSound fresh; // its zones are its layers: the pad's own layers go
        sound.layers = fresh.layers; sound.low_velocity = fresh.low_velocity; sound.high_velocity = fresh.high_velocity;
        sound.layer_play = fresh.layer_play;
        landing_pads.push_back({pending.track, uint8_t(kit), uint16_t(slot)});
    }
    if (keyed && !keep) { sound.mono = false; sound.play = PadPlay::Gate; }
    edit.what = pad_text("Load", unsigned(kit));
    restore_pad(pending.track, kit, edit.pad_after);
    if (still_browsing && pending.next_empty) t.last_pad = pending.pad;
    edits.push(std::move(edit));
    dirty = true;
    notice(keep ? pad_text("Loaded", unsigned(kit)) + ", settings kept" : pad_text("Loaded", unsigned(kit)));
    if(drum_sound_matches(ui.drums,pending.track,kit)){drum_sound_landed(*this,pending.track,kit);return;}
    if (still_browsing && !keyed && ui.settings.auto_import_bpm && !loaded.instrument) {
        ui.import_tempo = {slot, pending.track, pending.pad};
    } else if (still_browsing) {
        new_track_landed(*this);
    }
}

void Context::finish_import_tempo(bool match, bool stay_in_browser) {
    const auto pending = ui.import_tempo;
    if (pending.slot >= 0) stop_preview();
    ui.import_tempo = {};
    if (pending.slot < 0 || pending.track != ui.track || track().engine != EngineKind::Sampler) return;
    auto& pad = track().pads[pending.pad];
    if (pad.sample != uint16_t(pending.slot)) return;

    // The audio is already on the pad. Warp is a separate undoable edit.
    if (pending.detected && (match || pad.warped())) {
        Edit edit;
        edit.track = ui.track;
        edit.pad = int16_t(pending.pad);
        edit.pad_before = pad_state(track(), pending.pad);
        edit.pad_after = edit.pad_before;
        auto& sound = edit.pad_after.sound;
        sound.warp = match ? warp_seq_stretch : warp_off;
        sound.own_tempo = match ? pending.detected : 0;
        sound.beats = 0;
        edit.what = pad_text(match ? "Match tempo" : "Keep original tempo", pending.pad);
        restore_pad(ui.track, pending.pad, edit.pad_after);
        edits.push(std::move(edit));
        dirty = true;
    }
    if (!stay_in_browser) new_track_landed(*this);
}

void Context::use_detected_import_bpm() {
    const auto pending=ui.import_tempo;
    if(pending.slot<0||pending.slot>=int(sample_slots)||pending.track!=ui.track||track().engine!=EngineKind::Sampler||pending.pad>=sampler_pads||track().pads[pending.pad].sample!=uint16_t(pending.slot)) {notice("Sample changed; choose its BPM again");return;}
    if(pending.detected<300||pending.detected>3000){notice("No detected BPM in the 30-300 range");return;}
    set_tempo(pending.detected);dirty=true;
    finish_import_tempo(false);
    char text[64];std::snprintf(text,sizeof(text),"Sequence set to WAV BPM: %.1f",pending.detected/10.);notice(text);
}

void Context::remember_sample_select() {
    if (!library || ui.page != PageId::Samples || ui.sample_folder_pick || ui.midi_import || ui.expansions) return;
    sample_select_drive = uint8_t(library->drive());
    sample_select_place = {library->folder(), ui.files_cursor, true};
}

void Context::open_sample_select() {
    if (ui.sample_folder_pick) return;
    const bool already_open = ui.page == PageId::Samples && !ui.expansions;
    ui.expansions = ui.expansion_home = false;
    ui.browser_queries[2].clear();
    if (library) library->filter_folder("");
    open(PageId::Samples);
    if (!library) return;

    const bool use_default = !sample_select_started && !ui.settings.sample_folder.empty() && !sample_select_place.set;
    sample_select_started = true;
    if (already_open) { library->rescan(); return; }
    if (use_default) {
        stop_preview();
        library->preview("");
        ui.files_drive = uint8_t(Drive::Card);
        ui.files_cursor = 0;
        files_row_wanted = -1;
        library->browse_card_folder(ui.settings.sample_folder);
        return;
    }
    if (sample_select_place.set) {
        const auto drive = Drive(sample_select_drive);
        ui.files_drive = sample_select_drive;
        ui.files_cursor = sample_select_place.row;
        if (library->drive() != drive || library->folder() != sample_select_place.folder) {
            stop_preview();
            library->preview("");
            files_row_wanted = sample_select_place.row;
            library->browse(drive, sample_select_place.folder);
            return;
        }
    }
    library->rescan();
}

void Context::pick_sample_folder() {
    ui.browser_queries[2].clear();
    if (!library || library->root().empty()) { notice("No SD card"); return; }
    open(PageId::Samples);
    ui.sample_folder_pick = true;
    ui.files_cursor = 0;
    ui.files_drive = uint8_t(Drive::Card);
    stop_preview();
    library->preview("");
    library->browse_card_folder(ui.settings.sample_folder);
}

void Context::save_sample_folder(bool reset) {
    if (!library) return;
    const auto folder = reset ? std::string() : library->current_card_folder();
    if ((!reset && folder.empty()) || !Settings::valid_sample_folder(folder)) { notice("Choose an SD folder"); return; }
    ui.settings.sample_folder = folder;
    sample_select_place = {};
    sample_select_started = false;
    if (store) store->save_settings(ui.settings.text());
    if (reset) library->browse_card_folder("");
    back();
    notice(reset ? "Default: Samples" : "Sample folder saved");
}

void Context::set_auto_import_bpm(bool enabled) {
    if (ui.settings.auto_import_bpm == enabled) return;
    ui.settings.auto_import_bpm = enabled;
    if (store) store->save_settings(ui.settings.text());
    // Revisit the already loaded file when switching Auto BPM on; a different
    // highlighted file still uses the normal load-and-detect workflow.
    if(enabled&&ui.page==PageId::Samples&&ui.import_tempo.slot<0&&track().engine==EngineKind::Sampler&&library&&!ui.expansions&&!ui.midi_import&&!ui.sample_folder_pick&&!ui.sample_key_pick){
        const auto pad=track().last_pad%sampler_pads;const auto slot=track().pads[pad].sample;const auto selected=library->file_at(ui.files_cursor);
        if(slot<sample_slots&&selected&&!selected->folder&&selected->path==project.samples[slot].path){
            auto loaded=library->slot(slot);
            if(loaded.path==selected->path){
                if(loaded.ready&&!filename_tempo(loaded.path)&&!loaded.beats.found()){notice("No steady sample BPM detected");return;}
                ui.import_tempo={int(slot),ui.track,uint8_t(pad),uint16_t(loaded.ready?(filename_tempo(loaded.path)?filename_tempo(loaded.path):(loaded.beats.found()?loaded.beats.tempo:0)):0)};
            }
        }
    }
}

void Context::preview_import_tempo(bool warped) {
    auto& pending = ui.import_tempo;
    if (!pending.detected || pending.slot < 0 || pending.track != ui.track ||
        track().engine != EngineKind::Sampler || track().pads[pending.pad].sample != uint16_t(pending.slot)) return;
    Command audition {Command::Type::Audition, ui.track, 0, 0, int16_t(ui.audition_velocity)};
    audition.pad.sample = uint16_t(pending.slot);
    audition.pad.own_tempo = warped ? pending.detected : 0;
    pending.warped_preview = warped;
    send(audition);
    previewing = ui.track;
}

// An instrument read: the envelope most of its regions have goes on its pad,
// with its velocity tracking and an open filter, once, as part of the Load.
void Context::land_instruments() {
    if (!library || landing_pads.empty()) return;
    for (size_t n = 0; n < landing_pads.size();) {
        const auto l = landing_pads[n];
        auto& tr = project.tracks[l.track % track_count];
        auto& pad = tr.pads[l.pad % sampler_pads];
        const auto& ref = project.samples[l.slot % sample_slots];
        if (pad.sample != l.slot || !ref.instrument()) { landing_pads.erase(landing_pads.begin() + long(n)); continue; }
        const auto loaded = library->slot(l.slot);
        if (loaded.path != ref.path || (!loaded.ready && loaded.error.empty())) { ++n; continue; } // not read yet
        landing_pads.erase(landing_pads.begin() + long(n));
        if (!loaded.ready) { notice(loaded.error.c_str()); continue; }
        auto apply = [&](PadSound& sound) {
            if (loaded.stages[0] >= 0) sound.attack = uint8_t(loaded.stages[0]);
            if (loaded.stages[1] >= 0) sound.decay = uint8_t(loaded.stages[1]);
            if (loaded.stages[2] >= 0) sound.sustain = uint8_t(loaded.stages[2]);
            if (loaded.stages[3] >= 0) sound.release = uint8_t(loaded.stages[3]);
            sound.velocity_sense = uint8_t(loaded.velocity);
            sound.filter = PadFilter::Off; sound.cutoff = 100;
        };
        apply(pad);
        send_pad(l.track, l.pad);
        if (auto* edit = edits.latest())
            if (edit->track == l.track && edit->pad == int16_t(l.pad) && edit->pad_after.sound.sample == l.slot) apply(edit->pad_after.sound);
    }
}

// Made files that have arrived go onto their pads; what the library has to
// say about a file (saved, failed) shows over the header.
void Context::files_landed() {
    if (!library) return;
    land_instruments();
    browser_load_landed();
    auto& pending = ui.import_tempo;
    if (pending.slot >= 0 && !pending.detected) {
        if (ui.page != PageId::Samples || pending.track != ui.track ||
            track().engine != EngineKind::Sampler || track().pads[pending.pad].sample != uint16_t(pending.slot)) {
            stop_preview();
            pending = {};
        } else {
            const auto loaded = library->slot(unsigned(pending.slot));
            const auto& ref = project.samples[size_t(pending.slot)];
            if (loaded.path == ref.path) {
                if (loaded.ready && (filename_tempo(loaded.path) || loaded.beats.found())) {
                    pending.detected = filename_tempo(loaded.path) ? filename_tempo(loaded.path) : loaded.beats.tempo;
                } else if (loaded.ready || !loaded.error.empty()) {
                    finish_import_tempo(false);
                }
            }
        }
    }
    SampleLibrary::Done done;
    while (library->done(done)) {
        notice(done.text);
        for (auto& p : pending_files) // a file that could not be made: its slot is free again
            if (done.failed >= 0 && p.slot == done.failed) {
                if (p.land == 2) { ui.rec_slot = ui.rec_pad = -1; ui.rec_chops = 0; } // the take is gone
                project.samples.edit(size_t(p.slot)) = SampleRef {}; p = PendingFile {};
            }
    }
    SampleLibrary::BankBytes bank_bytes;
    while (library->take_bank(bank_bytes)) { // Load Bank: the file asked for last, onto the bank in view
        if (bank_bytes.path == expansion_load.path) {
            BankFile bank;
            if (!bank_bytes.error.empty() || !melodee::load_bank(bank_bytes.bytes, bank)) {
                expansion_load = {}; notice("Expansion bank unreadable");
            } else expansion_bank_landed(bank, std::filesystem::path(bank_bytes.path).stem().string());
            continue;
        }
        if (bank_bytes.path != bank_from) continue;
        bank_from.clear();
        auto name = std::filesystem::path(bank_bytes.path).stem().string();
        BankFile bank;
        if (!bank_bytes.error.empty()) notice("Load Bank: " + bank_bytes.error);
        else if (!melodee::load_bank(bank_bytes.bytes, bank)) notice("Load Bank: not a bank file");
        else load_bank(bank, name);
    }
    stems_landed(); // a sample's four stems together (shell/stems_jobs.cpp)
    for (auto& p : pending_files) {
        if (p.slot < 0 || p.land >= 3) continue; // 3 and up: stems, landed above
        auto loaded = library->slot(unsigned(p.slot));
        if (!loaded.ready) continue; // still being made
        if (p.land == 2 && loaded.streamed && !loaded.shaped) continue; // a long take's cuts land on its hits, once found
        auto& ref = project.samples.edit(size_t(p.slot));
        PendingFile was = p;
        p = PendingFile {};
        std::snprintf(ref.path, sizeof(ref.path), "%s", loaded.path.empty() ? ("(memory) " + loaded.name).c_str() : loaded.path.c_str());
        std::snprintf(ref.name, sizeof(ref.name), "%s", loaded.name.c_str());
        ref.frames = loaded.frames;
        ref.root = loaded.root;
        dirty = true;
        auto& t = project.tracks[was.track % track_count];
        if (was.land == 2) { take_landed(was.track, unsigned(was.pad) % sampler_pads, unsigned(was.slot), loaded); continue; }
        Edit edit;
        edit.track = was.track;
        if (was.land == 1) { // Audio Recall, Resample: onto the pad chosen, at the defaults
            unsigned kit = unsigned(was.pad) % sampler_pads;
            edit.pad = int16_t(kit);
            edit.pad_before = pad_state(t, kit);
            edit.pad_after = edit.pad_before;
            edit.pad_after.sound = PadSound {};
            edit.pad_after.sound.sample = uint16_t(was.slot);
            edit.what = pad_text(loaded.name.rfind("RESAMPLE", 0) == 0 ? "Resample to" : loaded.name.find(" Loop") != std::string::npos ? "Commit to"
                                 : "Audio Recall to", kit);
            t.last_pad = uint8_t(kit);
        } else if (was.trim) { // the pad, if it still plays what was trimmed, onto the whole new file
            unsigned kit = unsigned(was.pad) % sampler_pads;
            const auto& now_sound = t.pads[kit];
            if (now_sound.sample != was.sound.sample || now_sound.start != was.sound.start || now_sound.end != was.sound.end) {
                notice(pad_text("Trimmed file for", kit));
                continue;
            }
            edit.pad = int16_t(kit);
            edit.pad_before = pad_state(t, kit);
            edit.pad_after = edit.pad_before;
            auto& s = edit.pad_after.sound;
            uint32_t cut = s.start;
            s.sample = uint16_t(was.slot);
            s.loop_start = s.loop_start > cut ? std::min(s.loop_start - cut, loaded.frames - 1) : 0;
            s.start = 0; s.end = 0;
            edit.what = pad_text("Trim Sample", kit);
        } else { // Extract: onto the next empty pad after the one chopped
            int kit = next_empty_pad(unsigned(was.pad));
            if (was.track != ui.track || kit < 0) { notice("Extract: no empty pad"); continue; }
            edit.pad = int16_t(kit);
            edit.pad_before = pad_state(t, unsigned(kit));
            edit.pad_after = edit.pad_before;
            edit.pad_after.sound = PadSound {};
            edit.pad_after.sound.sample = uint16_t(was.slot);
            edit.what = pad_text("Extract to", unsigned(kit));
        }
        restore_pad(was.track, unsigned(edit.pad), edit.pad_after);
        notice(was.trim ? pad_text("Trimmed", unsigned(edit.pad)) : edit.what);
        if (was.land == 1) send_pad(was.track, unsigned(edit.pad));
        edits.push(std::move(edit));
    }
}

// Sample Record's take: onto its pad, and the pads cut during it each from
// their cut (pulled onto the strongest hit up to 50 ms before the tap, then
// with Zero Snap onto a zero crossing) to the next. One undo, which puts every pad it filled back as it was.
void Context::take_landed(unsigned t, unsigned kit, unsigned slot, const LoadedSample& loaded) {
    close_gesture(); // a pot's turns came before the take
    auto& track = project.tracks[t % track_count];
    std::array<uint32_t, UiState::rec_chop_capacity> at {};
    unsigned cuts = ui.rec_track == t ? ui.rec_chops : 0;
    for (unsigned n = 0; n < cuts; ++n) {
        at[n] = std::min(snap_to_hit(ui.rec_chop_at[n], loaded.hits, loaded.hit_count), loaded.frames ? loaded.frames - 1 : 0);
        if (ui.settings.zero_snap && library) at[n] = library->zero_crossing(slot, at[n]); // Chop's Zero Snap
    }
    Edit edit;
    edit.track = uint8_t(t % track_count);
    edit.what = pad_text("Take on", kit);
    auto place = [&](unsigned pad, uint32_t start, uint32_t end) {
        pad %= sampler_pads;
        edit.pads_before.emplace_back(uint8_t(pad), pad_state(track, pad));
        auto& sound = track.pads[pad];
        sound = PadSound {};
        sound.sample = uint16_t(slot); sound.start = start; sound.end = end;
        send_pad(t, pad);
        edit.pads_after.emplace_back(uint8_t(pad), pad_state(track, pad));
    };
    // Cuts in the order they fall; a cut pulled onto an earlier one's place is dropped.
    std::array<unsigned, UiState::rec_chop_capacity> order {};
    for (unsigned n = 0; n < cuts; ++n) order[n] = n;
    std::sort(order.begin(), order.begin() + cuts, [&](unsigned a, unsigned b) { return at[a] < at[b]; });
    uint32_t first_cut = cuts ? at[order[0]] : 0;
    place(kit, 0, first_cut > 0 ? first_cut : 0);
    unsigned placed = 0;
    for (unsigned k = 0; k < cuts; ++k) {
        unsigned n = order[k];
        uint32_t end = k + 1 < cuts ? at[order[k + 1]] : 0;
        if (at[n] == 0 || (end && end <= at[n])) continue;
        place(ui.rec_chop_pad[n], at[n], end);
        ++placed;
    }
    track.last_pad = uint8_t(kit);
    if (ui.rec_slot == int(slot)) { ui.rec_slot = ui.rec_pad = -1; ui.rec_chops = 0; }
    edits.push(std::move(edit));
    dirty = true;
    notice(placed ? pad_text("Take on", kit) + ", " + std::to_string(placed) + (placed == 1 ? " cut" : " cuts") : pad_text("Take on", kit));
}

void Context::preview_cursor() {
    if (!library || ui.sample_folder_pick || ui.sample_key_pick || library->browse_pending()) return;
    if (ui.midi_import) {
        const auto files = library->files(true);
        const auto* file = ui.files_cursor < files.size() ? &files[ui.files_cursor] : nullptr;
        const std::string path = ui.midi_autoplay && file && file->midi ? file->path : std::string();
        if (path == midi_preview_path) return;
        stop_preview();
        midi_preview_path = path;
        midi_preview_selected_at = now;
        return;
    }
    const auto file = library->file_at(ui.files_cursor);
    std::string path = !ui.midi_import && ui.settings.preview && file && !file->folder && !file->bank && !file->midi ? file->path : std::string(); // a MIDI file has no sound to preview
    stop_preview(); // what played stops as the cursor moves on
    library->preview(path);
}

void Context::stop_preview() {
    midi_preview_path.clear();
    if (ui.midi_preview) {
        ui.midi_preview = false;
        send({Command::Type::BrowserPiano});
        if (library) library->preview("");
    }
    if (previewing < 0) return;
    Command stop {Command::Type::Audition, uint8_t(previewing)};
    stop.pad.sample = no_sample;
    send(stop);
    previewing = -1;
}

void Context::search_samples(const std::string& query, bool typed) {
    if (!library || !library->search(query, typed)) return;
    stop_preview();
    library->preview("");
    ui.files_cursor = 0;
}

void Context::browser_tick() {
    if (ui.page == PageId::Project && store && ui.project_list) {
        ProjectStore::View view; store->view(view);
        filter_browser_files(view.entries, browser_query(ui));
        ui.project_rows = uint16_t(project_rows(view.entries.size()));
    }
    if (!library) return;
    if (ui.page == PageId::Samples) {
        const auto query = browser_query(ui);
        if (ui.name.target == NameTarget::BrowserSearch && query != ui.browser_queries[2]) { ui.files_cursor = 0; ui.browser_queries[2] = query; }
        library->filter_folder(query);
    }
    if (ui.page == PageId::Samples && ui.name.target == NameTarget::SampleSearch)
        search_samples(std::string(ui.name.text, ui.name.length), true);
    bool here = ui.page == PageId::Samples && track().sampled(); // a kit's browser or a keygroup's
    auto folder = library->folder();
    if (folder != files_folder_seen) {
        if (ui.midi_import) stop_preview(); // a new folder: the cursor on the one it came out of, else the top
        files_folder_seen = folder;
        ui.files_cursor = uint16_t(files_row_wanted >= 0 ? files_row_wanted : ui.midi_import ? 0 : std::max(library->came_from(), 0)); // B2's drive: its row
        files_row_wanted = -1;
        ui.files_drive = uint8_t(library->drive());
        if (ui.page == PageId::Samples && ui.midi_import) preview_cursor();
        else if (here && ui.settings.preview && ui.import_tempo.slot < 0) preview_cursor();
    }
    if (!here && previewing >= 0) { // away from the browser: what it played stops (B3 stays as it was)
        stop_preview();
        library->preview("");
    }
    if (ui.page == PageId::Samples && ui.midi_import && ui.midi_autoplay && !ui.midi_preview &&
        !midi_preview_path.empty() && now - midi_preview_selected_at >= 180) {
        ui.midi_preview = true;
        library->preview(midi_preview_path);
    }
    const auto order = library->search_order();
    if (here && order.query != browser_query_seen) {
        browser_query_seen = order.query;
        ui.files_cursor = 0;
        stop_preview();
        library->preview("");
    } else if (here) {
        ui.files_cursor = uint16_t(typed_sample_cursor(ui.files_cursor, browser_primary_seen, order.primary));
    }
    if (here) browser_primary_seen = order.primary;
    const auto hovered = here && !ui.midi_import && !ui.sample_folder_pick && ui.import_tempo.slot < 0
        ? library->file_at(ui.files_cursor) : std::nullopt;
    library->request_expansion_cover(here && ui.expansions && !ui.expansion_home && !library->browse_pending()
        ? (hovered && hovered->folder ? hovered->path : folder) : std::string());
    const bool audio = hovered && !hovered->folder && !hovered->bank && !hovered->instrument
        && !hovered->kit && !hovered->expansion && !hovered->midi;
    std::string path = audio ? hovered->path : std::string();
    const auto& track = this->track();
    const bool import_open = here && ui.import_tempo.slot >= 0 && unsigned(ui.import_tempo.slot) < sample_slots;
    if (import_open)
        path = project.samples[unsigned(ui.import_tempo.slot)].path;
    if (ui.page == PageId::Stage && track.engine == EngineKind::Sampler
        && stage_page_now(ui, track, stage_instrument(project, track)).kind == StageKind::Trim) {
        const auto& pad = track.pads[track.last_pad % sampler_pads];
        if (pad.used() && pad.sample < sample_slots) {
            const auto& sample = project.samples[pad.sample];
            if (sample.used() && !sample.instrument()) path = sample.path;
        }
    }
    const std::string row = hovered ? hovered->path : path;
    if (row != browser_selected_row) {
        browser_selected_row = row;
        browser_selected_at = now;
    }
    if (path != browser_selected_path) {
        browser_selected_path = path;
        browser_selected_at = now;
        library->analyse_key("");
    }
    if (!path.empty() && (import_open || now - browser_selected_at >= 500)) library->analyse_key(path);
    unsigned arrived = library->previews();
    if (arrived == previews_seen) return;
    previews_seen = arrived;
    if (ui.page == PageId::Samples && ui.midi_import && ui.midi_preview) {
        const auto files = library->files(true);
        if (ui.files_cursor < files.size() && files[ui.files_cursor].midi &&
            files[ui.files_cursor].path == library->previewed())
            send({Command::Type::BrowserPiano, 0, 0, 0, 1});
        return;
    }
    if (!hovered || !ui.settings.preview || hovered->path != library->previewed()) return;
    Command a {Command::Type::Audition, ui.track, 0, 0, int16_t(ui.audition_velocity)};
    a.pad.sample = uint16_t(preview_slot); a.pad.start = 0; a.pad.end = 0;
    send(a);
    previewing = ui.track;
}

} // namespace melodee
