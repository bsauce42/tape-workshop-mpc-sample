// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/phrase_line.h"
#include "melodee/model/smf.h"
#include <cassert>
#include <set>
#include <tuple>
#include <iostream>
using namespace melodee;
int main(){
 harmony::Settings key;key.root=0;key.scale_mask=0xab5;
 for(unsigned bar:{384u,288u}){auto source=harmony::progression(4,bar,key);auto before=harmony::midi(source,1200,bar==384?4:3,4);
 for(unsigned output=0;output<5;++output)for(unsigned rhythm=0;rhythm<harmony::bass_rhythm_count;++rhythm){harmony::OutputSettings s;s.output=output;harmony::set_output_rhythm(s,rhythm);assert(harmony::output_rhythm(s)==rhythm);std::vector<harmony::Note> notes;std::string error;assert(harmony::output_notes(source,s,96,notes,error,bar));assert(!notes.empty());
  for(auto n:notes){assert(n.key<128&&n.velocity&&n.length);bool region=false;for(unsigned i=0;i<source.count;++i){auto c=source.chords[i];if(n.tick>=c.tick&&n.tick<c.tick+c.length){assert(n.tick+n.length<=c.tick+c.length);bool tone=false;for(unsigned k=0;k<c.count;++k)tone|=n.key%12==c.notes[k]%12;assert(tone);region=true;}}assert(region);}
  if(output==2||output==3){unsigned end=0;for(auto n:notes){assert(n.tick>=end);end=n.tick+n.length;}}
  if(rhythm){harmony::BassSettings timing;timing.rhythm=rhythm;timing.gate=100;std::vector<harmony::Note> hits;assert(harmony::bass_line(source,timing,96,hits,error,bar));std::set<unsigned> wanted,got;for(auto n:hits)wanted.insert(n.tick);for(auto n:notes)got.insert(n.tick);assert(wanted==got);
   for(auto hit:hits){unsigned count=0,low=0,high=0,chordIndex=0;for(unsigned i=0;i<source.count;++i)if(hit.tick>=source.chords[i].tick)chordIndex=i;for(auto n:notes)if(n.tick==hit.tick){++count;low+=n.key<60;high+=n.key>=60;}
    assert(count==(output==0?source.chords[chordIndex].count:output==4?source.chords[chordIndex].count+1:1));if(output==4)assert(low==source.chords[chordIndex].count&&high==1);
   }
  }
  std::vector<SmfTrack> parsed;assert(read_smf(harmony::midi_notes(notes,1200,bar==384?4:3,4),parsed,error));using N=std::tuple<unsigned,unsigned,unsigned,unsigned>;std::vector<N>a,b;for(auto n:notes)a.emplace_back(n.tick,n.length,n.key,n.velocity);for(auto n:parsed[0].notes)b.emplace_back(n.tick,n.length,n.key,n.velocity);std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());assert(a==b);
 }
 assert(harmony::midi(source,1200,bar==384?4:3,4)==before);
 }
 harmony::OutputSettings independent;for(unsigned out=0;out<5;++out){independent.output=out;harmony::set_output_rhythm(independent,out+1);}for(unsigned out=0;out<5;++out){independent.output=out;assert(harmony::output_rhythm(independent)==out+1);}
 auto dense=harmony::progression(4,384,key);dense.count=1;dense.chords[0].length=49152;std::vector<harmony::Note> notes;std::string error;for(unsigned output:{0u,4u}){harmony::OutputSettings s;s.output=output;harmony::set_output_rhythm(s,5);assert(!harmony::output_notes(dense,s,96,notes,error)&&notes.empty());}
 std::cout<<"Five outputs x 24 rhythms x two meters: rhythm onsets, chord tones, Hybrid without bass, exact MIDI timing, bounds and independent settings passed.\n";
}
