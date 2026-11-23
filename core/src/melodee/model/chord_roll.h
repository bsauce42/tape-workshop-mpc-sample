// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/model/harmonizer.h"
#include "melodee/model/pattern.h"
namespace melodee::harmony {
bool roll_notes_edit(const Pattern& original,const std::vector<Note>& notes,uint32_t length,uint32_t max_length,bool keep,Pattern& output,std::string& error);
// Builds a complete proposed edit before the host opens its undo transaction.
bool roll_edit(const Pattern& original,const Result& chords,uint32_t length,uint32_t max_length,bool keep,Pattern& output,std::string& error);
}
