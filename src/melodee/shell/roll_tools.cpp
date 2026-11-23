// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/shell/roll_tools.h"
#include "melodee/shell/roll.h"
#include "melodee/shell/undo_scope.h"

namespace melodee {
void roll_select_all(Context& c) {
    auto& ui = c.ui;
    roll_unmark(ui);
    if (c.pattern().events.empty()) { c.notice("No notes in this pattern"); return; }
    ui.roll_mark_track = uint8_t(ui.track);
    ui.roll_mark_pattern = uint16_t(c.track().pattern);
    ui.roll_all = true;
    c.notice(std::to_string(c.pattern().events.size()) + " notes selected");
}
bool roll_start_split(Context& c) {
    const int index = roll_focus(c.pattern(), c.ui, roll_lane(c.track()));
    if (index < 0) { c.notice("Select a note first"); return false; }
    const auto& note = c.pattern().events[unsigned(index)];
    const uint32_t end = std::min(c.pattern().length, uint32_t(note.tick + note.length));
    if (end <= note.tick + 1) { c.notice("Note is too short to split"); return false; }
    auto& fine = c.ui.roll_tools;
    fine.note = note;
    fine.owner = c.ui.track * pattern_count + c.track().pattern;
    const uint32_t cursor = roll_cursor(c.pattern(), c.ui) * c.pattern().q();
    fine.cursor = cursor > note.tick && cursor < end ? cursor : note.tick + (end - note.tick) / 2;
    fine.increment = 0;
    while (fine.increment < 11 && roll_split_increment(fine.increment) < c.pattern().q()) ++fine.increment;
    fine.knob_motion = {};
    fine.fine = true;
    c.ui.sheet_open = false;
    return true;
}
void roll_split_step(Context& c, int delta) {
    auto& fine = c.ui.roll_tools;
    int index = roll_split_note(c.pattern(), fine.note);
    if (index < 0) {
        for (unsigned i = 0; i < c.pattern().events.size(); ++i) {
            const auto& note = c.pattern().events[i];
            if (note.key == fine.note.key && note.pitch == fine.note.pitch &&
                fine.cursor >= note.tick && fine.cursor <= uint32_t(note.tick + note.length)) {
                fine.note = note;
                index = int(i);
                break;
            }
        }
    }
    if (fine.owner != c.ui.track * pattern_count + c.track().pattern || index < 0) {
        c.notice("Choose Done, then select a note");
        return;
    }
    const auto& note = c.pattern().events[unsigned(index)];
    const uint32_t end = std::min(c.pattern().length, uint32_t(note.tick + note.length));
    fine.cursor = uint32_t(std::clamp<int64_t>(int64_t(fine.cursor) +
        int64_t(delta) * roll_split_increment(fine.increment), note.tick, end));
}
void roll_finish_split(Context& c) {
    roll_split_step(c, 0);
    auto& fine = c.ui.roll_tools;
    if (!fine.fine || fine.owner != c.ui.track * pattern_count + c.track().pattern) return;
    if (c.frozen_edit(pattern_scope(c.project, c.ui))) return;
    c.close_gesture();
    bool changed = false;
    c.edit_pattern(c.ui.track, c.track().pattern, "Split note", [&] {
        changed = roll_split_apply(c.pattern(), fine);
        if (changed) c.sync_pattern(c.ui.track, c.track().pattern);
    });
    if (!changed) { c.notice("No room to split here"); return; }
    c.ui.step_cursor = int16_t(fine.note.tick / c.pattern().q());
    c.ui.roll_key = uint8_t(roll_row(fine.note, roll_lane(c.track())));
    const auto in = step_events(c.pattern(), unsigned(c.ui.step_cursor));
    const int right = roll_split_note(c.pattern(), fine.note);
    c.ui.step_event = right >= 0 ? uint8_t(unsigned(right) - in.first) : 0;
    roll_unmark(c.ui);
    c.notice("Note split");
}
}
