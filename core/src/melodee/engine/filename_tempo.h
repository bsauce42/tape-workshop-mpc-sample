// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop: explicitly labelled filename tempo; never infer tempo from arbitrary numbers.
#pragma once
#include <cctype>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace melodee {
inline uint16_t filename_tempo(std::string_view path) {
    const auto slash = path.find_last_of("/\\");
    if (slash != std::string_view::npos) path.remove_prefix(slash + 1);
    auto lower = [](char c) { return char(std::tolower(static_cast<unsigned char>(c))); };
    auto separator = [](char c) { return c == ' ' || c == '_' || c == '-' || c == ':'; };
    uint16_t result = 0;
    for (size_t i = 0; i < path.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(path[i]))) continue;
        const size_t begin = i;
        if (begin && (std::isalnum(static_cast<unsigned char>(path[begin - 1])) || path[begin - 1] == '.')) continue;
        double value = 0;
        while (i < path.size() && std::isdigit(static_cast<unsigned char>(path[i]))) value = value * 10 + (path[i++] - '0');
        if (i + 1 < path.size() && path[i] == '.' && std::isdigit(static_cast<unsigned char>(path[i + 1]))) {
            ++i; double scale = .1;
            while (i < path.size() && std::isdigit(static_cast<unsigned char>(path[i]))) { value += (path[i++] - '0') * scale; scale *= .1; }
        }
        size_t after = i;
        while (after < path.size() && separator(path[after])) ++after;
        bool labelled = after + 3 <= path.size() && lower(path[after]) == 'b' && lower(path[after + 1]) == 'p' && lower(path[after + 2]) == 'm' &&
            (after + 3 == path.size() || !std::isalnum(static_cast<unsigned char>(path[after + 3])));
        size_t before = begin;
        while (before && separator(path[before - 1])) --before;
        if (before >= 3 && lower(path[before - 3]) == 'b' && lower(path[before - 2]) == 'p' && lower(path[before - 1]) == 'm' &&
            (before == 3 || !std::isalnum(static_cast<unsigned char>(path[before - 4])))) labelled = true;
        if (labelled && value >= 30 && value <= 300) {
            auto tempo = uint16_t(std::lround(value * 10));
            if (result && result != tempo) return 0; // conflicting labels: use audio analysis
            result = tempo;
        }
        if (i) --i;
    }
    return result;
}
}
