// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/midi_harmony_source.h"
#include "melodee/model/phrase_line.h"
#include <cassert>
#include <iostream>
using namespace melodee;
int main(){
    SmfTrack source;source.notes={{0,96,60,101},{96,96,64,92},{192,192,67,110}};std::vector<SmfTrack> file{source};Pattern imported;std::string error;
    assert(harmony::pitched_pad_import(file,19,384,imported,error));for(auto e:imported.events)assert(e.key==sampler_first_note+19);
    auto before=imported;std::vector<harmony::Note> notes;assert(harmony::midi_source(imported,true,19,notes,error)&&notes.size()==3);
    for(unsigned i=0;i<3;++i){assert(notes[i].key==source.notes[i].key&&notes[i].tick==source.notes[i].tick&&notes[i].length==source.notes[i].length&&notes[i].velocity==source.notes[i].velocity);}
    assert(imported.events==before.events);assert(!harmony::midi_source(imported,true,20,notes,error));
    Track instrument;instrument.engine=EngineKind::Midi;Pattern raw;assert(smf_pattern(file,instrument,384,raw));assert(harmony::midi_source(raw,false,0,notes,error)&&notes.size()==3);
    harmony::Settings settings;auto result=harmony::generate(notes,raw.length,96,384,settings);assert(result.count);
    for(unsigned output=0;output<5;++output){harmony::OutputSettings p;p.output=output;std::vector<harmony::Note> made;assert(harmony::output_notes(result,p,96,made,error)&&!made.empty());}
    auto low=file;low[0].notes[0].key=0;Pattern destination=before;assert(!harmony::pitched_pad_import(low,19,384,destination,error));assert(destination.events==before.events);
    raw.events[0].flags|=step_mute;assert(harmony::midi_source(raw,false,0,notes,error)&&notes.size()==2);
    auto invalid=raw;invalid.events[1].key=200;assert(!harmony::midi_source(invalid,false,0,notes,error)&&notes.empty());
    Pattern dense;for(unsigned n=0;n<harmony::max_melody+1;++n){Event e;e.key=60;e.tick=0;dense.events.push_back(e);}assert(!harmony::midi_source(dense,false,0,notes,error));
    std::cout<<"Imported MIDI pitch/timing, pitched-pad filtering, raw keyed Rolls, five outputs, immutable source, mute and range/density refusal passed.\n";
}
