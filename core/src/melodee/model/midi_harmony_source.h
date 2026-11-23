// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop; host based on Melodee by Kerem Kiliç (Ellic Studio).
#pragma once
#include "melodee/model/harmonizer.h"
#include "melodee/model/pattern.h"
#include "melodee/model/smf.h"
namespace melodee::harmony {
// Raw MIDI-key Rolls, or one sampler pad's encoded pitch lane. Read-only.
bool midi_source(const Pattern& pattern,bool pad_pitches,unsigned pad,std::vector<harmony::Note>& notes,std::string& error);
// MIDI imported into a pitched-pad Roll keeps its notes on that pad. Refuses
// pitches outside the existing +/-48-semitone pad range, without a partial edit.
bool pitched_pad_import(const std::vector<SmfTrack>& midi,unsigned pad,unsigned bar_ticks,Pattern& out,std::string& error);
}
