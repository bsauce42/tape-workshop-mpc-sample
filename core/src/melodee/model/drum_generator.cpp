// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/drum_generator.h"
#include <algorithm>
#include <cstdint>
#include <unordered_set>
namespace melodee::drums {
const char* genre_name(unsigned n){return music::genre_name(n);}
const char* role_name(unsigned n){static const char* names[]={"Kick","Snare","Closed Hat","Open Hat","Clap","Percussion","Tom","Cymbal"};return names[n%roles];}
namespace {
// Sixteenth-note anchors. Each genre has its own backbone; probability only
// decorates that backbone, rather than selecting unrelated random hits.
struct Groove {uint16_t kick,snare,open,perc;uint8_t hats,feel,half;};
constexpr Groove styles[]={
 {0x0441,0x1010,0x4000,0x0080,2,55,0},{0x0841,0x0100,0x1000,0x0000,1,50,1},
 {0x0401,0x1010,0x4000,0x0000,2,58,0},{0x2401,0x0100,0x4000,0x0040,1,54,1},
 {0x0401,0x1010,0x4000,0x0080,2,61,0},{0x0481,0x1010,0x4000,0x0400,2,55,0},
 {0x0241,0x1010,0x4000,0x0880,2,59,0},{0x1111,0x1010,0x4444,0x0088,2,50,0},
 {0x1111,0x1010,0x4444,0x0480,2,56,0},{0x1111,0x1000,0x4444,0x8420,1,50,0},
 {0x0241,0x1010,0x4000,0x0088,1,50,0},{0x0421,0x1210,0x4040,0x8800,1,55,0},
 {0x0441,0x0100,0x4000,0x0020,2,50,1},{0x0921,0x1010,0x4000,0x2492,2,53,0},
 {0x0909,0x1010,0x4000,0x1248,2,52,0},{0x1111,0x4848,0x4000,0x0240,2,50,0},
 {0x0901,0x1010,0x4000,0x4924,2,54,0},{0x0449,0x1010,0x4000,0x2288,1,57,0},
 {0x1111,0x1010,0x4444,0x8888,2,50,0},{0x0101,0x1010,0x4000,0x0800,2,50,0},
 {0x0101,0x1010,0x4000,0x0000,2,50,0},{0x0101,0x1010,0x0000,0x0040,3,60,0},
 {0x0001,0x1000,0x0000,0x0200,4,50,1},{0x0101,0x1010,0x0000,0x0000,2,50,0},
 {0x1111,0x1010,0x4444,0x0080,2,50,0},{0x0101,0x1010,0x4000,0x0440,2,55,0},
 {0x1111,0x1010,0x4444,0x8080,1,50,0},{0x0441,0x0100,0x4000,0x2080,2,50,1}};
static_assert(sizeof(styles)/sizeof(styles[0])==genres,"genre coverage");
uint32_t random(uint32_t& state){state^=state<<13;state^=state>>17;state^=state<<5;return state;}
}
bool generate(const Settings& s,const std::vector<Hit>& previous,std::vector<Hit>& output,std::string& error){
 error.clear();if(s.genre>=genres||!s.bars||s.bars>16||s.density>100||s.swing<50||s.swing>75||s.fills>100||previous.size()>limit){error="Invalid drum settings";return false;}
 const unsigned length=s.bars*384;std::vector<Hit> made;made.reserve(std::min<unsigned>(limit,s.bars*128));
 for(auto h:previous)if(h.role<roles&&s.locked[h.role]&&h.tick<length&&h.velocity)made.push_back(h);
 uint32_t rng=s.seed?s.seed:1;const auto g=styles[s.genre];
 auto add=[&](unsigned tick,unsigned role,unsigned vel){if(s.locked[role]||tick>=length)return;made.push_back({tick,uint8_t(role),uint8_t(std::clamp<unsigned>(vel,1,127))});};
 auto time=[&](unsigned bar,unsigned step){return bar*384+step*24+(step%2?(s.swing-50)*48/100:0);};
 auto chance=[&](unsigned percent){return random(rng)%100<percent;};
 for(unsigned bar=0;bar<s.bars;++bar){
  for(unsigned step=0;step<16;++step){const unsigned mask=1u<<step;unsigned at=time(bar,step);
   if(g.kick&mask)add(at,0,step%4?98+random(rng)%12:114);
   else if(step%2==0&&chance(s.density/8))add(at,0,85+random(rng)%14);
   if(g.snare&mask){add(at,1,105+random(rng)%12);if(s.genre==1||s.genre==3||s.genre==7||s.genre==15||s.genre==18||s.genre==27)add(at+2,4,74+random(rng)%12);}
   else if((step==7||step==15)&&chance(s.density/3))add(at,1,36+random(rng)%18);
   bool hat=g.hats==1||(g.hats==2&&step%2==0)||(g.hats==3&&(step==0||step==4||step==6||step==8||step==12||step==14))||(g.hats==4&&step%4==0);
   if(hat&&(step%4==0||chance(45+s.density/2))){
    const bool roll=(s.genre==1||s.genre==3||s.genre==10||s.genre==11||s.genre==27)&&step%4==3&&chance(s.density/2);
    if(roll){for(unsigned n=0;n<3;++n)add(at+n*8,2,52+n*8+random(rng)%9);}else add(at,2,step%4?49+random(rng)%25:80+random(rng)%9);
   }
   if((g.open&mask)&&chance(35+s.density/2))add(at,3,65+random(rng)%16);
   if((g.perc&mask)&&chance(s.density))add(at,5,55+random(rng)%28);
  }
  if(s.genre==21){for(unsigned beat=0;beat<4;++beat){add(bar*384+beat*96,7,70+random(rng)%12);if(beat%2)add(bar*384+beat*96+64,7,48+random(rng)%12);}}
  else if(bar%4==0&&chance(30+s.density/2))add(bar*384,7,70+random(rng)%15);
  if((bar%4==3||bar+1==s.bars)&&chance(s.fills))for(unsigned n=0;n<4;++n)add(bar*384+288+n*24,6,60+n*9);
 }
 std::stable_sort(made.begin(),made.end(),[](const Hit&a,const Hit&b){return a.tick<b.tick||(a.tick==b.tick&&a.role<b.role);});
 made.erase(std::unique(made.begin(),made.end(),[](const Hit&a,const Hit&b){return a.tick==b.tick&&a.role==b.role;}),made.end());
 if(made.size()>limit){error="Drum note limit exceeded";return false;}output=std::move(made);return true;
}
bool notes(const std::vector<Hit>& hits,const std::array<uint8_t,roles>& pads,unsigned length,std::vector<Event>& result,std::string& error){
 error.clear();if(!length||length>16*384||hits.size()>limit){error="Invalid drum length";return false;}std::array<bool,128> used{};for(unsigned pad:pads){if(pad>128){error="Invalid pad mapping";return false;}if(pad&&used[pad-1]){error="Assign each role a different pad";return false;}if(pad)used[pad-1]=true;}
 std::vector<Event> made;made.reserve(hits.size());for(auto h:hits){if(h.role>=roles||h.tick>=length||!h.velocity){error="Invalid drum hit";return false;}if(!pads[h.role])continue;Event e;e.tick=h.tick;e.length=uint16_t(std::min(6u,length-h.tick));e.key=uint8_t(35+pads[h.role]);e.velocity=h.velocity;made.push_back(e);}
 if(made.empty()){error="Assign at least one loaded drum pad";return false;}result=std::move(made);return true;
}
bool edit(const Pattern& source,const std::vector<Event>& notes,unsigned length,bool keep,Pattern& output,std::string& error){
 error.clear();if(notes.empty()||notes.size()>limit||!length||length>16*384||(keep&&(source.events.size()>max_pattern_events||source.length>max_bars*384))){error="No valid generated drums";return false;}Pattern made=source;if(!keep)made.events.clear();made.length=keep?std::max(source.length,length):length;made.free=false;
 std::unordered_set<uint64_t> seen;seen.reserve(std::min<size_t>(max_pattern_events,made.events.size()+notes.size()));for(auto e:made.events)seen.insert((uint64_t(e.tick)<<8)|e.key);
 for(auto n:notes){if(n.key<36||n.key>163||!n.length||n.tick>=length||n.length>length-n.tick){error="Invalid drum notes";return false;}bool duplicate=!seen.insert((uint64_t(n.tick)<<8)|n.key).second;if(!duplicate){if(made.events.size()>=max_pattern_events){error="Roll note limit exceeded";return false;}made.events.push_back(n);}}
 made.sort();made.fit_lanes();output=std::move(made);return true;
}
}
