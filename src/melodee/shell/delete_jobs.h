// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include <cstdint>
#include <string>

// Erase in a browser: the file or the folder under the cursor deleted from
// the card, once B3 says so (shell/ask.h; nothing brings it back).
//
// Samples: a sample, a kit bank, a MIDI file, an instrument (an MPC 3 track
// or program goes with the data folder that is its own, so an auto sampled
// keygroup goes whole), or a folder with all in it. The plug-in picker: a
// plug-in, or a folder of them.
//
// Not deleted: what the open project plays (a sample or an instrument in
// one of its slots, a plug-in on a track or in a chain), anything off the
// card (the internal drive), the folders at the card's top, Melodee's own
// folder from the sample browser, a SoundFont's presets, and the Plugins
// folder itself.
namespace melodee {

class Context;

// What the question is about (UiState::ask_target).
enum class DeleteWhat : uint8_t { File, Folder, Program, Plugin, PluginFolder };

// Erase tapped on Samples or on the plug-in picker: the question asked (false: nothing under the cursor).
bool samples_erase(Context& c);
bool plugins_erase(Context& c);
// B3 on the question: deleted on the library's thread; its notice says how it went.
void delete_asked(Context& c);
// Every pass: the plug-in picker listed again after a deletion.
void delete_tick(Context& c);

} // namespace melodee
