// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/shell/typed_sample_search.h"
#include "melodee/shell/name_entry.h"
#include <cassert>
#include <iostream>
#include <string>
int main() {
    using namespace melodee;
    struct File { std::string name; int identity; };
    std::vector<File> files {{"Dog kicks", 1}, {"Kick 01", 2}, {"KICK soft", 3}, {"Snare", 4}};
    filter_browser_files(files, "ki");
    assert(files.size() == 3 && files[0].identity == 2 && files[1].identity == 3 && files[2].identity == 1);
    filter_browser_files(files, "kick sof");
    assert(files.size() == 1 && files[0].identity == 3); // selection retains the actual file identity
    filter_browser_files(files, "missing"); assert(files.empty());
    const auto match = [](std::string_view stem, std::string_view query) {
        return typed_sample_matches(stem, typed_sample_fragments(query));
    };
    assert(match("WARMGRANDPIANO", "W"));
    assert(match("WARMGRANDPIANO", "WAR"));
    assert(match("WARMGRANDPIANO", "WARM PIA"));
    assert(match("WARMGRANDPIANO", "PIA WARM"));
    assert(match("WARMGRANDPIANO", "WARM_PIA"));
    assert(match("WARMGRANDPIANO", "  WARM  PIA  "));
    assert(match("PIANO", "IAN")); // fragments can start inside a word
    assert(!match("KICK", "W")); // .wav does not supply a W
    assert(!match("KICK", "A")); // .wav does not supply an A
    assert(!match("WARMGRANDPIANO", "WARM SNAR"));
    assert(!match("WARMGRANDPIANO", ""));
    assert(!match("WARMGRANDPIANO", "   "));
    const auto words = typed_sample_fragments("KICK");
    assert(typed_sample_starts_with("KICK01", words));
    assert(typed_sample_starts_with("KICKS", words));
    assert(!typed_sample_starts_with("KINGSDOGKICKS", words));
    assert(!typed_sample_starts_with("DEEPKICK", words));
    assert(typed_sample_starts_with("KINGSDOGKICKS", typed_sample_fragments("K")));
    assert(typed_sample_starts_with("WARMGRANDPIANO", typed_sample_fragments("WARM PIA")));
    std::vector<std::string> results, primary, secondary {"Kings Dog Kicks", "Deep Kick"};
    size_t primary_count = 0;
    append_typed_sample_matches(results, primary_count, primary, secondary);
    primary = {"Kick 01", "Kick Deep"}; secondary = {"Dog Kick"};
    append_typed_sample_matches(results, primary_count, primary, secondary);
    assert((results == std::vector<std::string>{"Kick 01", "Kick Deep", "Kings Dog Kicks", "Deep Kick", "Dog Kick"}));
    assert(typed_sample_cursor(0, 0, 2) == 0);
    assert(typed_sample_cursor(1, 0, 2) == 3); // same secondary file after insertion
    assert(typed_sample_cursor(1, 2, 3) == 1); // existing primary stays put
    results = {"secondary 1", "secondary 2", "secondary 3"}; primary_count = 0;
    primary = {"Kick A", "Kick B"}; secondary.clear();
    append_typed_sample_matches(results, primary_count, primary, secondary, 3);
    assert((results == std::vector<std::string>{"Kick A", "Kick B", "secondary 1"}));
    primary = {"Kick C", "Kick D"};
    append_typed_sample_matches(results, primary_count, primary, secondary, 3);
    assert((results == std::vector<std::string>{"Kick A", "Kick B", "Kick C"}));
    assert(primary_count == 3); // late prefix matches displace low-priority hits at the cap
    NameEntry e; e.start(NameTarget::SampleSearch, "", 31, true);
    e.pad(0, 1000); assert(e.length == 1 && match("AMBIENTPAD", e.text));
    e.pad(0, 1100); assert(e.text[0] == 'B' && match("BASS", e.text));
    e.erase(); assert(e.length == 0);
    std::cout << "Live matching, prefix-first ordering, late arrivals, result cap and selection preservation passed\n";
}
