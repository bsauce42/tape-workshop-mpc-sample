// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/phrase_line.h"
#include "melodee/model/smf.h"
#include <cassert>
#include <map>
#include <set>
#include <sstream>
#include <iostream>
using namespace melodee;
int main(){
 std::set<std::string> signatures;unsigned cases=0;
 for(unsigned meter:{384u,288u})for(unsigned genre=1;genre<=music::genre_count;++genre){
 harmony::Settings settings;settings.root=2;settings.scale_mask=0x5ad;settings.genre=genre;auto chords=harmony::progression(4,meter,settings);assert(chords.count==4);auto before=harmony::midi(chords,930,meter==384?4:3,4);
 for(unsigned output:{0u,2u,3u,4u}){
  harmony::OutputSettings s;s.output=output;s.genre=genre;std::vector<harmony::Note> notes;std::string error;assert(harmony::output_notes(chords,s,96,notes,error,meter));assert(!notes.empty()&&notes.size()<=4096);
  std::ostringstream sig;for(auto n:notes){assert(n.key<128&&n.velocity&&n.tick+n.length<=4*meter);bool tone=false;for(unsigned i=0;i<chords.count;++i)if(n.tick>=chords.chords[i].tick&&n.tick<chords.chords[i].tick+chords.chords[i].length)for(unsigned k=0;k<chords.chords[i].count;++k)tone|=n.key%12==chords.chords[i].notes[k]%12;bool scale=chords.scale_mask&(1u<<((n.key+12-chords.tonic)%12));assert(tone||((genre==1||genre==2)&&output!=0&&scale));sig<<n.tick<<','<<n.length<<','<<unsigned(n.key)<<','<<unsigned(n.velocity)<<';';}
  if(meter==384&&output==3)signatures.insert(sig.str());
  auto bytes=harmony::midi_notes(notes,930,meter==384?4:3,4);std::vector<SmfTrack> tracks;unsigned tempo=0;assert(read_smf(bytes,tracks,error,&tempo)&&tempo==930);size_t count=0;for(auto& track:tracks)for(auto& n:track.notes){assert(n.tick==notes[count].tick&&n.length==notes[count].length&&n.key==notes[count].key);++count;}assert(count==notes.size());
  harmony::set_output_rhythm(s,3);assert(harmony::effective_rhythm(s)==3);std::vector<harmony::Note> explicitNotes,original;s.genre=0;assert(harmony::output_notes(chords,s,96,original,error,meter));s.genre=genre;assert(harmony::output_notes(chords,s,96,explicitNotes,error,meter));std::map<unsigned,unsigned>a,b;for(auto n:original)++a[n.tick];for(auto n:explicitNotes)++b[n.tick];assert(a==b);
  assert(before==harmony::midi(chords,930,meter==384?4:3,4));++cases;
 }
 }
 assert(signatures.size()>=20);
 harmony::OutputSettings invalid;invalid.genre=255;std::vector<harmony::Note> notes;std::string error;harmony::Settings key;auto c=harmony::progression(4,384,key);assert(!harmony::output_notes(c,invalid,96,notes,error));
 std::cout<<cases<<" genre/output/meter cases passed; "<<signatures.size()<<" distinct melody profiles, deterministic scale-aware output, explicit rhythm override, MIDI reimport, source preservation and bounds.\n";
}
