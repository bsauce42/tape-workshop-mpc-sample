// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/chord_roll.h"
#include "melodee/model/limits.h"
#include <unordered_set>
namespace melodee::harmony {
bool roll_edit(const Pattern& original,const Result& chords,uint32_t length,uint32_t maximum,bool keep,Pattern& output,std::string& error){
    error.clear();if((keep&&(original.length>maximum||original.events.size()>max_pattern_events))||!chords.count||chords.count>max_chords||!length||length>maximum){error="No valid chords / Roll length exceeded";return false;}
    std::vector<Note> notes;notes.reserve(chords.count*5);
    for(unsigned i=0;i<chords.count;++i){const auto& ch=chords.chords[i];if(ch.count<3||ch.count>5){error="Invalid chord notes";return false;}for(unsigned k=0;k<ch.count;++k)notes.push_back({ch.tick,ch.length,ch.notes[k],ch.velocity});}
    return roll_notes_edit(original,notes,length,maximum,keep,output,error);
}
bool roll_notes_edit(const Pattern& original,const std::vector<Note>& notes,uint32_t length,uint32_t maximum,bool keep,Pattern& output,std::string& error){
    error.clear();if(notes.empty()||notes.size()>max_pattern_events||!length||length>maximum||(keep&&(original.length>maximum||original.events.size()>max_pattern_events))){error="No valid notes / Roll limit exceeded";return false;}
    Pattern made=original;if(!keep)made.events.clear();made.length=keep?std::max(original.length,length):length;
    auto signature=[](const Event& e){return (uint64_t(e.tick)<<23)|(uint64_t(e.length)<<7)|e.key;};
    std::unordered_set<uint64_t> seen;seen.reserve(std::min<size_t>(max_pattern_events,made.events.size()+notes.size()));for(const auto& e:made.events)seen.insert(signature(e));
    for(const auto& note:notes){if(note.key>127||!note.length||note.tick>=length||note.length>length-note.tick){error="Invalid note timing/pitch";return false;}
        uint32_t left=note.length,at=note.tick;while(left){Event e;e.tick=at;e.length=uint16_t(std::min(left,65535u));e.key=note.key;e.velocity=std::clamp<unsigned>(note.velocity,1,127);
            bool duplicate=!seen.insert(signature(e)).second;
            if(!duplicate){if(made.events.size()>=max_pattern_events){error="Roll note limit exceeded";return false;}made.events.push_back(e);}at+=e.length;left-=e.length;
        }
    }
    made.sort();made.fit_lanes();output=std::move(made);return true;
}
}
