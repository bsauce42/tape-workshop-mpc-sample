// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop; based on Melodee by Kerem Kiliç (Ellic Studio).
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace melodee::harmony {
constexpr unsigned ppq=96, max_chords=256, max_melody=2048;
enum class Follow : uint8_t { Note, Beat, Phrase, Bar, Auto };
struct Note { uint32_t tick=0,length=1; uint8_t key=60,velocity=100; };
struct Settings { Follow follow=Follow::Phrase; uint8_t strength=85,complexity=7,genre=0; bool leading=true; uint16_t variation=0; int8_t root=-1; uint16_t scale_mask=0; bool strict_scale=false; };
struct Chord { uint32_t tick=0,length=1; std::array<uint8_t,5> notes{}; uint8_t count=0,velocity=90,root=0,quality=0,inversion=0; };
struct Result { std::array<Chord,max_chords> chords{}; uint16_t count=0; uint8_t tonic=0; bool minor=false; uint16_t scale_mask=0; };
const char* follow_name(Follow mode);
std::string chord_name(const Chord& chord);
Result generate(const std::vector<Note>& melody,uint32_t length,unsigned beat,unsigned bar,const Settings& settings);
Result progression(unsigned bars,unsigned bar,const Settings& settings);
void invert(Chord& chord,int delta);
std::vector<uint8_t> midi(const Result& chords,unsigned tempo_tenths,unsigned beats,unsigned unit);
// Non-real-time storage only. Refuses empty/invalid card roots; never overwrites a recording.
bool save(const std::string& card,const std::string& filename,const std::vector<uint8_t>& bytes,std::string& path,std::string& error);
}
