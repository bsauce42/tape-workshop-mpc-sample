// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/phrase_line.h"
#include "melodee/model/audio_harmony.h"
#include "melodee/model/scales.h"
#include <cassert>
#include <iostream>
using namespace melodee;
void check(const harmony::Result&r,unsigned root,unsigned mask){assert(r.count);for(unsigned n=0;n<r.count;++n){auto c=r.chords[n];assert(c.count>=3);for(unsigned k=0;k<c.count;++k)assert(mask&(1u<<((c.notes[k]+12-root)%12)));}}
int main(){unsigned cases=0;
 for(unsigned root=0;root<12;++root)for(auto scale:scales()){
  harmony::Settings s;s.root=root;s.scale_mask=scale.mask;s.strict_scale=true;s.genre=2;
  auto progression=harmony::progression(4,384,s);check(progression,root,scale.mask);
  std::vector<harmony::Note> melody{{0,192,uint8_t(60+root),100},{192,192,uint8_t(61+root),80},{384,384,uint8_t(67+root),110}};
  auto followed=harmony::generate(melody,768,96,384,s);check(followed,root,scale.mask);
  std::vector<harmony::Chroma> bins(2);for(unsigned i=0;i<2;++i){bins[i].tick=i*384;bins[i].length=384;bins[i].frames=10;bins[i].tones[root]=.5;bins[i].tones[(root+4)%12]=.3;bins[i].tones[(root+7)%12]=.2;}
  auto audio=harmony::recognize(bins,s);check(audio,root,scale.mask);
  for(unsigned output:{0u,2u,3u,4u}){harmony::OutputSettings part;part.output=output;part.genre=2;std::vector<harmony::Note> notes;std::string error;assert(harmony::output_notes(progression,part,96,notes,error));for(auto n:notes)assert(scale.mask&(1u<<((n.key+12-root)%12)));++cases;}
 }
 harmony::Settings impossible;impossible.root=0;impossible.scale_mask=1;impossible.strict_scale=true;assert(!harmony::progression(4,384,impossible).count);assert(!harmony::generate({{0,384,60,100}},384,96,384,impossible).count);
 std::cout<<cases<<" scale/root/output checks passed; all 13 scales, project-key progression, MIDI and audio recognition stay in scale; impossible combinations refuse.\n";
}
