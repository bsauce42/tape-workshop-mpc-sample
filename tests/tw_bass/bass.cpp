// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/bass_line.h"
#include "melodee/model/chord_roll.h"
#include "melodee/model/smf.h"
#include <cassert>
#include <iostream>
using namespace melodee;
int main(){
    harmony::Settings key;key.root=2;key.scale_mask=0x5ad;auto chords=harmony::progression(4,384,key);auto original=chords;
    for(unsigned style=0;style<4;++style)for(unsigned octave=0;octave<4;++octave){
        harmony::BassSettings settings;settings.style=style;settings.octave=octave;std::vector<harmony::Note> notes;std::string error;
        assert(harmony::bass_line(chords,settings,96,notes,error));assert(notes.size()==(style==0?4:style==3?32:16));
        uint32_t end=0;for(auto n:notes){assert(n.tick>=end&&n.length>0&&n.tick+n.length<=1536);end=n.tick+n.length;assert(n.key>=12+12*octave&&n.key<=35+12*octave);}
        for(unsigned i=0;i<4;++i){bool root=false;for(auto n:notes)if(n.tick==chords.chords[i].tick){assert(n.key%12==chords.chords[i].root);root=true;}assert(root);}
        auto bytes=harmony::midi_notes(notes,1200,4,4);std::vector<SmfTrack> parsed;assert(read_smf(bytes,parsed,error)&&parsed.size()==1&&parsed[0].notes.size()==notes.size());
        for(unsigned i=0;i<notes.size();++i){auto a=notes[i];auto b=parsed[0].notes[i];assert(a.key==b.key&&a.tick==b.tick&&a.length==b.length&&a.velocity==b.velocity);}
        Pattern existing;Event old;old.key=90;old.length=12;existing.events.push_back(old);Pattern keep,replace;
        assert(harmony::roll_notes_edit(existing,notes,1536,49152,true,keep,error));assert(harmony::roll_notes_edit(existing,notes,1536,49152,false,replace,error));assert(keep.events.size()==replace.events.size()+1&&existing.events.size()==1);
    }
    assert(harmony::midi(chords,1200,4,4)==harmony::midi(original,1200,4,4));
    harmony::BassSettings s;std::vector<harmony::Note> notes;std::string error;s.gate=100;assert(harmony::bass_line(chords,s,96,notes,error)&&notes[0].length==384);
    s.style=3;auto longChord=chords;longChord.count=1;longChord.chords[0].length=500000;assert(!harmony::bass_line(longChord,s,96,notes,error)&&notes.empty());
    harmony::Result empty;assert(!harmony::bass_line(empty,s,96,notes,error));auto bad=chords;bad.chords[1].tick=0;assert(!harmony::bass_line(bad,s,96,notes,error));
    harmony::Chord dim;dim.root=0;dim.count=3;dim.notes={48,51,54,0,0};s.style=2;s.octave=2;assert(harmony::bass_pitch(dim,s,1)==42);dim.notes={48,52,56,0,0};assert(harmony::bass_pitch(dim,s,1)==44);
    s.style=1;s.gate=100;assert(harmony::bass_line(chords,s,96,notes,error));std::vector<SmfTrack> exact;assert(read_smf(harmony::midi_notes(notes,1200,4,4),exact,error)&&exact[0].notes.size()==16);for(auto n:exact[0].notes)assert(n.length==96);
    std::cout<<"Four bass styles, three octaves, root following, monophonic timing, gate, exact MIDI reimport, keep/replace, source preservation and density bounds passed.\n";
}
