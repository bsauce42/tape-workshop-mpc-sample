// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop; based on Melodee by Kerem Kiliç (Ellic Studio).
#pragma once
#include "melodee/model/pattern.h"
#include "melodee/model/music_genres.h"
#include <array>
#include <string>
#include <vector>
namespace melodee::drums {
constexpr unsigned roles=8,genres=music::genre_count,limit=4096;
const char* genre_name(unsigned genre);
const char* role_name(unsigned role);
struct Settings { uint8_t genre=1,bars=4,density=65,swing=50,fills=50;uint32_t seed=1;std::array<bool,roles> locked{}; };
struct Hit {uint32_t tick=0;uint8_t role=0,velocity=100;bool operator==(const Hit& b)const{return tick==b.tick&&role==b.role&&velocity==b.velocity;}};
// Control-thread generation, deterministic by seed. Locked lanes survive regeneration.
bool generate(const Settings& settings,const std::vector<Hit>& previous,std::vector<Hit>& result,std::string& error);
bool notes(const std::vector<Hit>& hits,const std::array<uint8_t,roles>& pads,unsigned length,std::vector<Event>& result,std::string& error);
bool edit(const Pattern& source,const std::vector<Event>& notes,unsigned length,bool keep,Pattern& result,std::string& error);
}
