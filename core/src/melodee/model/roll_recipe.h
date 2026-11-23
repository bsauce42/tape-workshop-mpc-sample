// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/model/pattern.h"
#include <sstream>
#include <string>
namespace melodee {
constexpr unsigned advanced_user_presets = 8;
struct RollRecipe {
    int recipe = 0, grid = 5, hits = 6, velocity = 100, intensity = 30, accent = 0;
    int swing = 0, offset = 0, repeats = 1, placement = 3, duration = 96, key = 42;
    int slot = 0, start = 0;
    bool replace = false;
    void clamp() {
        recipe = std::clamp(recipe, 0, 15); grid = std::clamp(grid, 0, 8); hits = std::clamp(hits, 1, 256);
        velocity = std::clamp(velocity, 1, 127); intensity = std::clamp(intensity, 0, 100);
        accent = std::clamp(accent, 0, 5); swing = std::clamp(swing, -49, 49);
        offset = std::clamp(offset, -384, 384); repeats = std::clamp(repeats, 1, 64);
        placement = std::clamp(placement, 0, 6); duration = std::clamp(duration, 1, 6144);
        start = std::clamp(start, 0, 1048575);
        key = std::clamp(key, 0, 163); slot = std::clamp(slot, 0, 7);
    }
    std::string text() const {
        std::ostringstream out;
        out << recipe << ',' << grid << ',' << hits << ',' << velocity << ',' << intensity << ',' << accent << ','
            << swing << ',' << offset << ',' << repeats << ',' << placement << ',' << duration << ',' << key << ',' << int(replace) << ',' << start;
        return out.str();
    }
    static RollRecipe read(std::string text) {
        std::replace(text.begin(), text.end(), ',', ' ');
        RollRecipe out;
        int replace = 0;
        std::istringstream in(text);
        in >> out.recipe >> out.grid >> out.hits >> out.velocity >> out.intensity >> out.accent >>
            out.swing >> out.offset >> out.repeats >> out.placement >> out.duration >> out.key >> replace >> out.start;
        out.replace = replace != 0; out.clamp();
        return out;
    }
};
}
