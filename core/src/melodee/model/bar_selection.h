// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstdint>
namespace melodee {
// Equal-width knob zones, including the final partial bar. No exact-endpoint requirement.
inline unsigned bar_knob_index(int value, uint32_t length, unsigned bar_ticks) {
    if (!length || !bar_ticks) return 0;
    const unsigned count = 1 + (length - 1) / bar_ticks;
    return std::min(count - 1, unsigned(std::clamp(value, 0, 127)) * count / 128);
}
}
