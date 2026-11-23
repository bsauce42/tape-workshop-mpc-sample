// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/model/pattern.h"
#include <algorithm>
#include <array>
namespace melodee {
struct RollToolsState {
    bool fine = false;
    uint32_t cursor = 0;
    int zoom = 0;
    unsigned increment = 7;
    std::array<int, 3> knob_motion {};

    Event note;
    unsigned owner = ~0u;
};
inline unsigned roll_split_increment(unsigned index) {
    constexpr unsigned ticks[] {1, 2, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96};
    return ticks[std::min(index, 11u)];
}
inline const char* roll_split_increment_name(unsigned index) {
    constexpr const char* names[] {"1 tick", "2 ticks", "4 ticks", "6 ticks", "8 ticks", "1/32", "1/16T", "1/16", "1/8T", "1/8", "1/4T", "1/4"};
    return names[std::min(index, 11u)];
}
inline unsigned roll_view_grid(unsigned grid, int zoom) {
    return zoom >= 0 ? std::max(1u, grid >> unsigned(std::min(zoom, 4))) :
                       grid << unsigned(std::min(-zoom, 3));
}
inline int roll_split_note(const Pattern& pattern, const Event& target) {
    for (unsigned i = 0; i < pattern.events.size(); ++i) {
        const auto& e = pattern.events[i];
        if (e.tick > target.tick) break;
        if (e.tick == target.tick && e.key == target.key && e.pitch == target.pitch) return int(i);
    }
    return -1;
}
inline bool roll_split_at(Pattern& pattern, const Event& target, uint32_t cursor) {
    const int index = roll_split_note(pattern, target);
    if (index < 0 || pattern.events.size() >= max_pattern_events) return false;
    const auto original = pattern.events[unsigned(index)];
    const uint32_t end = std::min(pattern.length, uint32_t(original.tick + original.length));
    if (cursor <= original.tick || cursor >= end) return false;
    auto right = original;
    right.tick = cursor;
    right.length = uint16_t(end - cursor);
    // Keep a single trigger for this pitch at the split point.
    if (std::any_of(pattern.events.begin(), pattern.events.end(), [&](const Event& e) {
        return e.tick == cursor && e.key == right.key && e.pitch == right.pitch;
    })) return false;
    pattern.events[unsigned(index)].length = uint16_t(cursor - original.tick);
    pattern.events.push_back(right);
    pattern.sort();
    return true;
}
inline bool roll_split_apply(Pattern& pattern, RollToolsState& state) {
    if (!state.fine || !roll_split_at(pattern, state.note, state.cursor)) return false;
    auto target = state.note;
    target.tick = state.cursor;
    const int right = roll_split_note(pattern, target);
    if (right >= 0) state.note = pattern.events[unsigned(right)];
    return true;
}

}
