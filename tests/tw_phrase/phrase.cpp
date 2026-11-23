// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/phrase_line.h"
#include "melodee/model/chord_roll.h"
#include "melodee/model/smf.h"
#include <cassert>
#include <set>
#include <tuple>
#include <iostream>
using namespace melodee;
using namespace melodee::harmony;
int main(){
    Settings settings;settings.root=1;settings.scale_mask=0x5ad;auto source=progression(4,384,settings);auto original=midi(source,1100,4,4);
    for(unsigned output=0;output<5;++output){OutputSettings s;s.output=output;std::vector<harmony::Note> notes;std::string error;
        assert(output_notes(source,s,96,notes,error));assert(!notes.empty());
        std::vector<SmfTrack> parsed;assert(read_smf(midi_notes(notes,1100,4,4),parsed,error)&&parsed.size()==1);
        using N=std::tuple<unsigned,unsigned,unsigned,unsigned>;std::vector<N> wanted,got;
        for(auto n:notes){assert(n.key<128&&n.length&&n.tick+n.length<=1536);wanted.emplace_back(n.tick,n.length,n.key,n.velocity);}for(auto n:parsed[0].notes)got.emplace_back(n.tick,n.length,n.key,n.velocity);std::sort(wanted.begin(),wanted.end());std::sort(got.begin(),got.end());assert(wanted==got);
        if(output==2||output==3){uint32_t end=0;for(auto n:notes){assert(n.tick>=end);end=n.tick+n.length;bool matched=false;for(unsigned i=0;i<source.count;++i){auto ch=source.chords[i];if(n.tick>=ch.tick&&n.tick<ch.tick+ch.length){for(unsigned k=0;k<ch.count;++k)matched|=n.key%12==ch.notes[k]%12;assert(n.tick+n.length<=ch.tick+ch.length);}}assert(matched);}}
        if(output==4){unsigned chords=0,melody=0;std::array<uint32_t,128> ends{};
            for(auto n:notes){assert(n.tick>=ends[n.key]);ends[n.key]=n.tick+n.length;if(n.key<60)++chords;else ++melody;}
            assert(chords&&melody);for(auto n:notes)assert(n.key>=48&&(n.key<60||n.key>=72));std::vector<harmony::Note> lead;assert(phrase_line(source,s.melody,96,true,lead,error));unsigned chordNotes=0;for(unsigned i=0;i<source.count;++i)chordNotes+=source.chords[i].count;assert(notes.size()==chordNotes+lead.size());
            OutputSettings raised=s;raised.pluck.octave=3;raised.melody.octave=0;auto fixed=normalize_output(raised);assert(fixed.pluck.octave==3&&fixed.melody.octave==1);
        }
        Pattern pattern,kept,replaced;Event old;old.key=12;pattern.events.push_back(old);assert(roll_notes_edit(pattern,notes,1536,49152,true,kept,error));assert(roll_notes_edit(pattern,notes,1536,49152,false,replaced,error));assert(kept.events.size()==replaced.events.size()+1&&pattern.events.size()==1);
        std::array<uint8_t,8> preview{};unsigned count=output_preview(source.chords[0],s,preview);assert(count&&count<=7);if(output==4)assert(count==source.chords[0].count+1);for(unsigned n=0;n<count;++n)assert(preview[n]<128);
    }
    std::set<std::vector<unsigned>> variants;
    for(unsigned seed=0;seed<32;++seed){PhraseSettings p;p.variation=seed;std::vector<harmony::Note> notes;std::string error;assert(phrase_line(source,p,96,true,notes,error));std::vector<unsigned> pitches;for(auto n:notes)pitches.push_back(n.key);variants.insert(pitches);}assert(variants.size()>8);
    for(unsigned rate=0;rate<4;++rate)for(unsigned pattern=0;pattern<4;++pattern){PhraseSettings p;p.rate=rate;p.pattern=pattern;std::vector<harmony::Note> a,b;std::string error;assert(phrase_line(source,p,96,false,a,error)&&phrase_line(source,p,48,false,b,error));assert(a.size()==b.size());for(unsigned n=0;n<a.size();++n)assert(a[n].tick==b[n].tick&&a[n].key==b[n].key);}
    OutputSettings dense;dense.output=4;dense.pluck.rate=2;dense.melody.rate=2;auto longSource=source;longSource.count=1;longSource.chords[0].length=500000;std::vector<harmony::Note> notes;std::string error;assert(!output_notes(longSource,dense,96,notes,error)&&notes.empty());
    assert(midi(source,1100,4,4)==original);std::cout<<"Five outputs, Hybrid layers/registers, monophony, chord tones/timing, independent phrases, variations, preview bounds, MIDI reimport and Keep/Replace passed.\n";
}
