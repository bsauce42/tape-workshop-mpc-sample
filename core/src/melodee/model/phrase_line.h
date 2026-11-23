// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop; host based on Melodee by Kerem Kiliç (Ellic Studio).
#pragma once
#include "melodee/model/bass_line.h"
#include "melodee/model/music_genres.h"
namespace melodee::harmony {
struct PhraseSettings { uint8_t rate=1,octave=1,gate=50,pattern=0;uint16_t variation=0;uint8_t rhythm=0,genre=0; };
struct OutputSettings { uint8_t output=0,chord_rhythm=0,hybrid_rhythm=0;BassSettings bass {0,1,100,7};PhraseSettings pluck;PhraseSettings melody {0,2,80,2,0};uint8_t genre=0; };
const char* output_name(unsigned output);
inline unsigned output_rhythm(const OutputSettings& s){switch(s.output){case 0:return s.chord_rhythm;case 1:return s.bass.rhythm;case 2:return s.pluck.rhythm;case 3:return s.melody.rhythm;default:return s.hybrid_rhythm;}}
inline void set_output_rhythm(OutputSettings& s,unsigned rhythm){rhythm=std::min(rhythm,bass_rhythm_count-1);switch(s.output){case 0:s.chord_rhythm=rhythm;break;case 1:s.bass.rhythm=rhythm;break;case 2:s.pluck.rhythm=rhythm;break;case 3:s.melody.rhythm=rhythm;break;default:s.hybrid_rhythm=rhythm;break;}}
inline unsigned effective_rhythm(const OutputSettings& s){auto rhythm=output_rhythm(s);if(rhythm||!s.genre||s.output==1)return rhythm;auto p=music::profile(s.genre);return s.output==0||s.output==4?p.chords:s.output==2?p.pluck:p.melody;}
const char* phrase_rate(unsigned rate);
const char* phrase_pattern(unsigned pattern,bool melody,unsigned genre=0);
// Keep Hybrid parts in distinct pitch registers on a single editable Roll.
OutputSettings normalize_output(OutputSettings settings);
bool phrase_line(const Result& chords,const PhraseSettings& settings,unsigned beat,bool melody,std::vector<Note>& notes,std::string& error,unsigned bar_ticks=ppq*4);
bool output_notes(const Result& chords,const OutputSettings& settings,unsigned beat,std::vector<Note>& notes,std::string& error,unsigned bar_ticks=ppq*4);
unsigned output_preview(const Chord& chord,const OutputSettings& settings,std::array<uint8_t,8>& notes);
}
