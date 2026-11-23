// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop; host based on Melodee by Kerem Kiliç (Ellic Studio).
#pragma once
#include "melodee/model/harmonizer.h"
namespace melodee::harmony {
struct BassSettings { uint8_t style=0,octave=2,gate=85,rhythm=0; };
inline constexpr unsigned max_bass_notes=4096;
inline constexpr unsigned bass_rhythm_count=24;
const char* bass_rhythm(unsigned rhythm);
const char* bass_style(unsigned style);
unsigned bass_pitch(const Chord& chord,const BassSettings& settings,unsigned step=0);
// Deterministic accompaniment following detected chord roots/changes; not
// extraction of an existing bass performance from the audio. Non-real-time only.
bool bass_line(const Result& chords,const BassSettings& settings,unsigned beat,std::vector<Note>& notes,std::string& error,unsigned bar_ticks=ppq*4);
std::vector<uint8_t> midi_notes(const std::vector<Note>& notes,unsigned tempo_tenths,unsigned beats,unsigned unit);
}
