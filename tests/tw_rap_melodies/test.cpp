// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/phrase_line.h"
#include "melodee/model/smf.h"
#include <cassert>
#include <set>
#include <tuple>
#include <sstream>
#include <iostream>
using namespace melodee;using namespace melodee::harmony;
std::string signature(const std::vector<harmony::Note>& notes){std::ostringstream o;for(auto n:notes)o<<n.tick<<','<<n.length<<','<<int(n.key)<<','<<int(n.velocity)<<';';return o.str();}
int main(){
 std::set<std::string> varied;unsigned passing=0;
 for(unsigned genre:{1u,2u})for(unsigned meter:{384u,288u,480u})for(unsigned root=0;root<12;++root)for(unsigned mask:{0xab5u,0x5adu,0x295u,0x4a9u}){
  Settings key;key.root=root;key.scale_mask=mask;key.strict_scale=true;key.genre=genre;auto chords=progression(8,meter,key);assert(chords.count==8&&chords.scale_mask==mask);auto before=midi(chords,1400,meter/96,4);
  for(unsigned output:{2u,3u,4u})for(unsigned rate=0;rate<4;++rate){OutputSettings s;s.output=output;s.genre=genre;s.melody.rate=s.pluck.rate=rate;s.melody.variation=s.pluck.variation=17;std::vector<harmony::Note> notes,again;std::string error;assert(output_notes(chords,s,96,notes,error,meter));assert(output_notes(chords,s,96,again,error,meter));assert(signature(notes)==signature(again));assert(!notes.empty()&&notes.size()<=4096);
   std::set<unsigned> bars;unsigned lastEnd=0;for(auto n:notes){assert(n.key<128&&n.velocity&&n.length&&n.tick+n.length<=8*meter);bars.insert(n.tick/meter);bool found=false;for(unsigned c=0;c<chords.count;++c){auto chord=chords.chords[c];if(n.tick<chord.tick||n.tick>=chord.tick+chord.length)continue;assert(n.tick+n.length<=chord.tick+chord.length);found=true;bool tone=false;for(unsigned k=0;k<chord.count;++k)tone|=n.key%12==chord.notes[k]%12;if(!tone){assert(mask&(1u<<((n.key+12-root)%12)));assert(n.length<=48);++passing;}}assert(found);if(output!=4){assert(n.tick>=lastEnd);lastEnd=n.tick+n.length;}}
   assert(bars.size()==8);assert(midi(chords,1400,meter/96,4)==before);
   std::vector<SmfTrack> imported;unsigned bpm=0;assert(read_smf(midi_notes(notes,1400,meter/96,4),imported,error,&bpm)&&bpm==1400);size_t count=0;for(auto&t:imported)for(auto n:t.notes){assert(n.tick==notes[count].tick&&n.length==notes[count].length&&n.key==notes[count].key);++count;}assert(count==notes.size());
  }
 }
 assert(passing>0);
 Settings key;key.root=0;key.scale_mask=0x5ad;key.strict_scale=true;auto chords=progression(8,384,key);OutputSettings s;s.output=3;s.genre=2;s.melody.rate=1;std::vector<harmony::Note> notes;std::string error;
 for(unsigned variation=0;variation<32;++variation){s.melody.variation=variation;assert(output_notes(chords,s,96,notes,error));varied.insert(signature(notes));}assert(varied.size()>=24);
 // Repeating one chord isolates the intended two-bar motif and answer structure.
 Result held;held.tonic=0;held.minor=true;held.scale_mask=0x5ad;held.count=1;held.chords[0]={0,384*8,{48,51,55},3,90,0,1,0};s.melody.variation=0;assert(output_notes(held,s,96,notes,error));
 std::set<unsigned>a,b;for(auto n:notes){if(n.tick<768)a.insert(n.tick);if(n.tick>=768&&n.tick<1536)b.insert(n.tick-768);}assert(a==b&&a.size()>=5&&a.size()<=10);
 unsigned minimum=127,maximum=0;for(auto n:notes){minimum=std::min(minimum,unsigned(n.key));maximum=std::max(maximum,unsigned(n.key));}assert(maximum-minimum<=19);
 std::set<std::string> styles;unsigned rollingCount=0,spaceCount=0;for(unsigned pattern=0;pattern<4;++pattern){s.melody.pattern=pattern;assert(output_notes(held,s,96,notes,error));styles.insert(signature(notes));if(pattern==1)rollingCount=notes.size();if(pattern==3)spaceCount=notes.size();}assert(styles.size()==4&&rollingCount>spaceCount);s.melody.pattern=2;
 // A 25th bar and a final partial bar remain addressable; no dropped tail bar.
 held.chords[0].length=25*384+37;assert(output_notes(held,s,96,notes,error));bool tail=false;for(auto n:notes){assert(n.tick+n.length<=held.chords[0].length);tail|=n.tick>=25*384;}assert(tail);
 for(unsigned bar:{12u,48u,96u,144u})for(unsigned rate=0;rate<4;++rate){held.chords[0].length=bar*5+3;s.melody.rate=rate;assert(output_notes(held,s,96,notes,error,bar));unsigned end=0;for(auto n:notes){assert(n.tick>=end);end=n.tick+n.length;assert(end<=held.chords[0].length);}}
 // Explicit rhythm is still exact and distinct from genre-auto rhythm.
 for(unsigned rhythm=1;rhythm<bass_rhythm_count;++rhythm){s.melody.rhythm=rhythm;assert(output_notes(chords,s,96,notes,error));BassSettings timing;timing.rhythm=rhythm;timing.gate=s.melody.gate;std::vector<harmony::Note> hits;assert(bass_line(chords,timing,96,hits,error));assert(notes.size()==hits.size());for(unsigned n=0;n<hits.size();++n)assert(notes[n].tick==hits[n].tick);}
 std::cout<<"Hip-hop/trap: keys, four scales, 3/4/5-beat meters, all melodic outputs/rates, motifs, passing notes, variation, full WAV tail, explicit rhythm and MIDI roundtrip passed.\n";
}
