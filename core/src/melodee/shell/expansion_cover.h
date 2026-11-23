// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop expansion artwork, decoded on the library service thread.
#pragma once
#include <memory>
namespace juce { class Image; }
#include <string>
namespace melodee {
struct ExpansionCover {
    std::shared_ptr<const juce::Image> image;
    std::string title, author, folder;
};
// Searches the selected folder/program and up to four parents, within root.
// Only bounded JPEG/PNG files inside the pack folder are decoded.
ExpansionCover read_expansion_cover(const std::string& selected, const std::string& root);
}
