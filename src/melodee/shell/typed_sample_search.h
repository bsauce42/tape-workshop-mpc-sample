// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Tape Workshop contributors
#pragma once
#include <algorithm>
#include <iterator>
#include <string>
#include <cctype>
#include <string_view>
#include <vector>

namespace melodee {
// Uppercase filename stem and query. Each phrase fragment must occur somewhere
// in the name; the existing compact-name/genre aliases are checked separately.
// Split once per query batch, never once per file or on the audio thread.
inline std::vector<std::string_view> typed_sample_fragments(std::string_view query) {
    std::vector<std::string_view> words;
    constexpr std::string_view separators = " \t\r\n_.-";
    for (size_t at = 0; at < query.size();) {
        at = query.find_first_not_of(separators, at);
        if (at == std::string_view::npos) break;
        auto end = query.find_first_of(separators, at);
        if (end == std::string_view::npos) end = query.size();
        words.push_back(query.substr(at, end - at));
        at = end;
    }
    return words;
}
inline bool typed_sample_matches(std::string_view stem_key,
                                 const std::vector<std::string_view>& words) {
    if (words.empty()) return false;
    for (auto word : words) if (stem_key.find(word) == std::string_view::npos) return false;
    return true;
}
// Prefer the beginning of the filename over fragments later in the name.
inline bool typed_sample_starts_with(std::string_view stem_key,
                                     const std::vector<std::string_view>& words) {
    return !words.empty() && stem_key.substr(0, words.front().size()) == words.front();
}

// New prefix hits can arrive after secondary matches during background indexing.
// Keep two stable groups without sorting the whole library on every batch.
template<class T>
void append_typed_sample_matches(std::vector<T>& results, size_t& primary_count,
                                 std::vector<T>& primary, std::vector<T>& secondary,
                                 size_t limit = 0xffff) {
    const auto keep = std::min(primary.size(), limit - primary_count);
    results.insert(results.begin() + primary_count, std::make_move_iterator(primary.begin()),
                   std::make_move_iterator(primary.begin() + keep));
    primary_count += keep;
    if (results.size() > limit) results.resize(limit);
    const auto rest = std::min(secondary.size(), limit - results.size());
    results.insert(results.end(), std::make_move_iterator(secondary.begin()),
                   std::make_move_iterator(secondary.begin() + rest));
}
inline unsigned typed_sample_cursor(unsigned cursor, size_t old_primary, size_t new_primary) {
    // Row zero follows the best result. A selected secondary row follows its
    // file when newly indexed prefix hits are inserted above it.
    if (cursor && cursor >= old_primary && new_primary > old_primary)
        return unsigned(std::min<size_t>(0xfffe, cursor + new_primary - old_primary));
    return cursor;
}
template<class T> void filter_browser_files(std::vector<T>& files, std::string query) {
    if (query.empty()) return;
    for (auto& c : query) c = char(std::toupper(static_cast<unsigned char>(c)));
    const auto words = typed_sample_fragments(query);
    if (words.empty()) return;
    std::vector<T> first, later;
    for (auto& file : files) {
        std::string name = file.name;
        for (auto& c : name) c = char(std::toupper(static_cast<unsigned char>(c)));
        if (!typed_sample_matches(name, words)) continue;
        (typed_sample_starts_with(name, words) ? first : later).push_back(std::move(file));
    }
    first.insert(first.end(), std::make_move_iterator(later.begin()), std::make_move_iterator(later.end()));
    files = std::move(first);
}
} // namespace melodee
