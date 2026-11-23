// SPDX-License-Identifier: AGPL-3.0-only
// Musical pattern/scheduler checks without rendering audio or touching a device.
#include "melodee/plugin/melody_pattern.h"
#include <cassert>
#include <set>
#include <vector>
#include <cstdio>
using namespace melodee::melody;
int main(){
 Settings s;s.enabled=true;s.density=1;
 std::set<std::vector<int>> patterns;
 for(unsigned seed=0;seed<100;++seed){s.variation=seed;std::vector<int> notes;for(unsigned n=0;n<32;++n){int key=note(s,n);assert(key>=0&&key<128);assert(key==note(s,n));notes.push_back(key);}patterns.insert(notes);}
 assert(patterns.size()>70);
 for(unsigned scale=0;scale<13;++scale)for(unsigned key=0;key<12;++key){s.scale=scale;s.key=key;
  Settings transposed=s;transposed.key=0;for(unsigned n=0;n<32;++n)assert(note(s,n)-note(transposed,n)==int(key));}
 s=Settings{};s.enabled=true;s.density=1;s.swing=.4f;s.gate=.7f;s.steps=16;
 for(unsigned n=0;n<32;++n){assert(onset(s,n+1)>onset(s,n));assert(onset(s,n)+duration(s,n)<onset(s,n+1));}
 Clock clock;int starts=0,stops=0,held=0;
 auto on=[&](int key,unsigned velocity){assert(!held);assert(key>=0&&key<128&&velocity>0&&velocity<=127);++starts;++held;};
 auto off=[&]{assert(held==1);++stops;--held;};
 double increment=120./60/44100;
 for(unsigned n=0;n<44100*5;++n)clock.tick(s,true,n*increment,increment,on,off);
 assert(starts>=18&&starts<=21);clock.tick(s,false,10,increment,on,off);assert(held==0&&stops==starts);
 // A seek, disable and restart must release the generator's held note exactly once.
 clock.tick(s,true,0,increment,on,off);clock.tick(s,true,8,increment,on,off);
 s.enabled=false;clock.tick(s,true,8+increment,increment,on,off);assert(!held);
 s.enabled=true;s.density=0;clock.tick(s,true,16,increment,on,off);assert(!held);
 clock.tick(s,true,-1,increment,on,off);assert(!held);
 // Same musical pattern whether time arrives as 64- or 128-sample host blocks.
 auto run=[&](unsigned block){Settings t;t.enabled=true;t.density=.8f;Clock c;std::vector<int> notes;for(unsigned base=0;base<44100*4;base+=block)for(unsigned i=0;i<block&&base+i<44100*4;++i)c.tick(t,true,(base+i)*increment,increment,[&](int n,unsigned){notes.push_back(n);},[]{});return notes;};
 assert(run(64)==run(128));std::puts("Variation, thirteen scales, all keys, swing/gate, rest density, stop/seek safety and block-size consistency passed; no audio rendered.");
}
