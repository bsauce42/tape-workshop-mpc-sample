// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/model/project.h"
#include <set>
#include <stdexcept>
#include <string>

namespace melodee {
inline void studio_hash(uint64_t& hash, uint64_t value) { hash ^= value; hash *= 1099511628211ull; }
inline std::string studio_revision(const Pattern& pattern) {
    uint64_t h = 1469598103934665603ull;
    studio_hash(h, pattern.length);
    for (const auto& e : pattern.events) {
        for (int64_t v : {int64_t(e.tick), int64_t(e.length), int64_t(e.key), int64_t(e.velocity), int64_t(e.chance), int64_t(e.flags), int64_t(e.pitch), int64_t(e.ratchet), int64_t(e.ramp), int64_t(e.pitch_ramp), int64_t(e.condition), int64_t(e.cutoff)}) studio_hash(h, uint64_t(v));
    }
    return std::to_string(h);
}
inline std::string studio_revision(const Song& song) {
    uint64_t h = 1469598103934665603ull;
    studio_hash(h, song.count); studio_hash(h, song.loop_start); studio_hash(h, song.loop_end);
    for (unsigned i = 0; i < song.count; ++i) {
        const auto& c = song.clips[i];
        for (unsigned v : {unsigned(c.track), unsigned(c.pattern), unsigned(c.bar), unsigned(c.bars), unsigned(c.muted)}) studio_hash(h, v);
    }
    return std::to_string(h);
}
struct StudioNoteChange {
    enum class Kind { Add, Update, Delete } kind = Kind::Add;
    unsigned index = 0, tick = 0, length = 24, key = 60, velocity = 100;
    bool carry_play = false;
    Event play;
};
inline Pattern studio_notes(const Pattern& original, const std::string& revision, const std::vector<StudioNoteChange>& changes, unsigned max_key) {
    if (revision != studio_revision(original)) throw std::runtime_error("Notes changed on the MPC. Refreshed; try again.");
    if (changes.empty() || changes.size() > 128) throw std::runtime_error("Select 1–128 notes per edit");
    Pattern next = original;
    std::set<unsigned> touched, removed;
    std::vector<Event> added;
    for (const auto& change : changes) {
        if (change.kind != StudioNoteChange::Kind::Add) {
            if (change.index >= original.events.size() || !touched.insert(change.index).second) throw std::runtime_error("Invalid or repeated note index");
        }
        if (change.kind == StudioNoteChange::Kind::Delete) { removed.insert(change.index); continue; }
        if (change.tick >= original.length || change.length < 1 || change.length > 65535 || change.length > original.length - change.tick || change.key > max_key || (max_key > 127 && change.key < sampler_first_note) || change.velocity < 1 || change.velocity > 127)
            throw std::runtime_error("Note is outside this pattern");
        Event event = change.kind == StudioNoteChange::Kind::Add ? Event{} : original.events[change.index];
        if (change.carry_play) {
            const auto& play = change.play;
            if (play.chance > 100 || play.flags > 15 || play.pitch < -48 || play.pitch > 48 || play.ratchet < 1 || play.ratchet > 8 || play.ramp < -100 || play.ramp > 100 || play.pitch_ramp < -24 || play.pitch_ramp > 24 || play.condition >= 44 || play.cutoff > 101) throw std::runtime_error("Invalid note playback settings");
            event.chance = play.chance; event.flags = play.flags; event.pitch = play.pitch; event.ratchet = play.ratchet;
            event.ramp = play.ramp; event.pitch_ramp = play.pitch_ramp; event.condition = play.condition; event.cutoff = play.cutoff;
        }
        event.tick = change.tick; event.length = uint16_t(change.length); event.key = uint8_t(change.key); event.velocity = uint8_t(change.velocity);
        if (change.kind == StudioNoteChange::Kind::Add) added.push_back(event); else next.events[change.index] = event;
    }
    for (auto i = removed.rbegin(); i != removed.rend(); ++i) next.events.erase(next.events.begin() + *i);
    if (next.events.size() + added.size() > max_pattern_events) throw std::runtime_error("Pattern note limit reached");
    next.events.insert(next.events.end(), added.begin(), added.end()); next.sort();
    return next;
}
// Song::place can cut before detecting capacity. Stage on a copy, reject any
// overlap or capacity failure, and never mutate the live arrangement on error.
inline Song studio_clip(const Song& original, const std::string& revision, int index, const Clip* replacement) {
    if (revision != studio_revision(original)) throw std::runtime_error("Arrangement changed on the MPC. Refreshed; try again.");
    Song next = original;
    if (index >= 0) {
        if (unsigned(index) >= original.count) throw std::runtime_error("Clip no longer exists");
        next.erase(unsigned(index));
    }
    if (replacement) {
        const auto& c = *replacement;
        if (c.track >= track_count || c.pattern >= pattern_count || !c.bars || c.bar >= song_bars || unsigned(c.bar) + c.bars > song_bars) throw std::runtime_error("Clip outside song range");
        for (unsigned i = 0; i < next.count; ++i) { const auto& other = next.clips[i]; if (other.track == c.track && other.bar < c.end() && c.bar < other.end()) throw std::runtime_error("Clip overlaps another clip. Move it to an empty space."); }
        if (next.count == song_clips) throw std::runtime_error("Song clip limit reached");
        next.clips[next.count++] = c;
    }
    return next;
}
}
