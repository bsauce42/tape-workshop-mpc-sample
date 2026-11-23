// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include "melodee/plugin/plugin_panel.h"
#include "melodee/ui/paint.h"
#include <algorithm>

namespace melodee {

inline juce::Colour prism_band_colour(unsigned band) {
    constexpr unsigned colours[] = {0xff70dfce, 0xffffb366, 0xffc1a0ff, 0xff72adff, 0xffff82ac};
    return juce::Colour(colours[std::min(band, 4u)]);
}

inline unsigned prism_selected_band(const PluginPanel& panel) {
    return panel.rows == 5 ? std::min<unsigned>(panel.row, 4)
                           : std::min<unsigned>(panel.row / 4, 3);
}

} // namespace melodee
