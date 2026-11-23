// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/midi_harmony_source.h"
#include "melodee/model/project.h"
namespace melodee::harmony {
bool midi_source(const Pattern& pattern,bool pitches,unsigned pad,std::vector<harmony::Note>& notes,std::string& error){
    notes.clear();error.clear();if(pitches&&pad>=sampler_pads){error="Invalid MIDI source pad";return false;}
    std::vector<harmony::Note> read;read.reserve(std::min(pattern.events.size(),size_t(max_melody)));
    for(const auto& e:pattern.events){if((e.flags&step_mute)||!e.velocity||!e.length||e.tick>=pattern.length)continue;if(pitches&&e.key!=sampler_first_note+pad)continue;
        int key=pitches?60+int(e.pitch):int(e.key);if(key<0||key>127){error="Choose MIDI keys or the correct pitched pad";return false;}
        if(read.size()>=max_melody){error="MIDI source exceeds 2048 notes; choose a shorter pattern";return false;}
        read.push_back({e.tick,std::min<uint32_t>(e.length,pattern.length-e.tick),uint8_t(key),e.velocity});
    }
    if(read.empty()){error="No notes in the selected MIDI source";return false;}notes=std::move(read);return true;
}
bool pitched_pad_import(const std::vector<SmfTrack>& midi,unsigned pad,unsigned bar,Pattern& out,std::string& error){
    error.clear();if(pad>=sampler_pads){error="Invalid MIDI import pad";return false;}
    Track keyed;keyed.engine=EngineKind::Midi;Pattern made;if(!smf_pattern(midi,keyed,bar,made)){error="MIDI file has no usable notes";return false;}
    for(auto& e:made.events){if(e.key<12||e.key>108){error="Pad MIDI range is 12-108; use a keyed/MIDI track";return false;}e.pitch=int8_t(int(e.key)-60);e.key=uint8_t(sampler_first_note+pad);}
    out=std::move(made);return true;
}
}
