// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/phrase_line.h"
#include "melodee/model/smf.h"
#include <cassert>
#include <iostream>
#include <set>
using namespace melodee;
int main(){
 harmony::Settings key;key.root=0;auto chords=harmony::progression(4,384,key);auto original=harmony::midi(chords,1200,4,4);
 for(unsigned rhythm=0;rhythm<harmony::bass_rhythm_count;++rhythm)for(unsigned style=0;style<4;++style){
  harmony::BassSettings s;s.rhythm=rhythm;s.style=style;std::vector<harmony::Note> notes;std::string error;assert(harmony::bass_line(chords,s,96,notes,error));uint32_t end=0;
  for(auto n:notes){assert(n.tick>=end&&n.length&&n.velocity&&n.velocity<=127);end=n.tick+n.length;bool belongs=false;for(unsigned i=0;i<chords.count;++i){auto c=chords.chords[i];if(n.tick>=c.tick&&n.tick<c.tick+c.length){assert(end<=c.tick+c.length);belongs=true;}}assert(belongs);}
  for(unsigned i=0;i<chords.count;++i){bool found=false;for(auto n:notes)if(n.tick==chords.chords[i].tick){assert(n.key%12==chords.chords[i].root);found=true;}assert(found);}
  std::vector<SmfTrack> parsed;assert(read_smf(harmony::midi_notes(notes,1200,4,4),parsed,error));assert(parsed[0].notes.size()==notes.size());for(unsigned i=0;i<notes.size();++i){auto a=notes[i];auto b=parsed[0].notes[i];assert(a.tick==b.tick&&a.length==b.length&&a.key==b.key&&a.velocity==b.velocity);}
 }
 harmony::Result one=chords;one.count=1;one.chords[0].length=768;harmony::BassSettings s;s.gate=100;std::vector<harmony::Note> notes;std::string error;
 auto ticks=[&](unsigned rhythm){s.rhythm=rhythm;assert(harmony::bass_line(one,s,96,notes,error));std::vector<unsigned> result;for(auto n:notes)result.push_back(n.tick);return result;};
 assert(ticks(3)==std::vector<unsigned>({0,96,192,288,384,480,576,672}));assert(ticks(4).size()==16);assert(ticks(5).size()==32);assert(ticks(6).size()==24);
 assert(ticks(7)==std::vector<unsigned>({0,168,192,336,384,552,576,720}));
 assert(ticks(14)==std::vector<unsigned>({0,192,576}));assert(notes[0].length==192&&notes[1].length==384&&notes[2].length==192);
 assert(ticks(17)==std::vector<unsigned>({0,336,384,720}));assert(notes[0].length==336&&notes[1].length==48);
 assert(ticks(19)==std::vector<unsigned>({0,96,192,384,480,576}));assert(notes[0].length==96&&notes[1].length==96&&notes[2].length==192);
 // Fixed grid survives an off-grid chord change; partial tails are clipped.
 one.chords[0].tick=17;one.chords[0].length=200;s.rhythm=3;assert(harmony::bass_line(one,s,96,notes,error));assert(notes.size()==3&&notes[0].tick==17&&notes[1].tick==96&&notes[2].tick==192&&notes.back().tick+notes.back().length==217);
 one.chords[0].tick=0;one.chords[0].length=864;s.rhythm=22;assert(harmony::bass_line(one,s,96,notes,error,288));assert(notes.size()==3&&notes[1].tick==288&&notes[2].tick==576);
 harmony::OutputSettings out;out.output=1;out.bass=s;assert(harmony::output_notes(one,out,96,notes,error,288)&&notes[1].tick==288);
 one.chords[0].length=500000;s.rhythm=5;assert(!harmony::bass_line(one,s,96,notes,error)&&notes.empty());s.rhythm=255;assert(!harmony::bass_line(one,s,96,notes,error));
 assert(harmony::midi(chords,1200,4,4)==original);
 std::cout<<"24 rhythms x 4 pitch styles: exact MIDI, grid, long/short durations, chord roots, monophony, tails, 3/4 meter, bounds and immutable source passed.\n";
}
