// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/model/harmonizer.h"
#include "melodee/model/pattern.h"
namespace melodee {
std::vector<harmony::Chord> bar_chords(unsigned root,unsigned mask);
bool bar_replace(const Pattern& source,unsigned from,unsigned to,int lane,const std::vector<Event>& notes,Pattern& out,std::string& error);
bool bar_octave(const Pattern& source,const std::vector<size_t>& selected,int octaves,int lane,Pattern& out,std::string& error);
uint64_t bar_fingerprint(const Pattern& pattern);
}
