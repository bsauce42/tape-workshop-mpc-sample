// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/drum_generator.h"
#include <cassert>
#include <iostream>
#include <set>
using namespace melodee;
int main(){
 unsigned cases=0;std::set<std::vector<unsigned>> backbones;
 for(unsigned genre=0;genre<drums::genres;++genre)for(unsigned bars:{1u,2u,4u,8u,16u})for(unsigned density:{0u,65u,100u})for(unsigned swing:{50u,75u}){
 drums::Settings s;s.genre=genre;s.bars=bars;s.density=density;s.swing=swing;s.seed=84;std::vector<drums::Hit>a,b;std::string error;assert(drums::generate(s,{},a,error));assert(!a.empty()&&a.size()<=drums::limit);assert(drums::generate(s,{},b,error)&&a==b);
 std::set<uint64_t> seen;for(auto h:a){assert(h.tick<bars*384&&h.velocity&&h.velocity<=127&&h.role<8);assert(seen.insert((uint64_t(h.tick)<<8)|h.role).second);}
 s.locked[0]=true;s.locked[1]=true;++s.seed;assert(drums::generate(s,a,b,error));for(unsigned role:{0u,1u}){std::vector<drums::Hit>x,y;for(auto h:a)if(h.role==role)x.push_back(h);for(auto h:b)if(h.role==role)y.push_back(h);assert(x==y);}
 std::array<uint8_t,8> pads{1,2,3,4,5,6,127,128};std::vector<Event> events;assert(drums::notes(a,pads,bars*384,events,error));for(auto e:events)assert(e.key>=36&&e.key<=163&&e.tick+e.length<=bars*384);
 Pattern source;Event original;original.key=36;original.tick=9000;source.length=10000;source.events.push_back(original);Pattern made;assert(drums::edit(source,events,bars*384,true,made,error));assert(made.length==10000&&source.events.size()==1&&made.events.back()==original);assert(drums::edit(source,events,bars*384,false,made,error));assert(made.length==bars*384&&made.events.size()==events.size());
 ++cases;
 }
 drums::Settings s;std::vector<drums::Hit>a,b;std::string error;assert(drums::generate(s,{},a,error));for(auto& lock:s.locked)lock=true;++s.seed;assert(drums::generate(s,a,b,error)&&a==b);
 std::array<uint8_t,8> duplicate{1,1,0,0,0,0,0,0};std::vector<Event> notes;assert(!drums::notes(a,duplicate,1536,notes,error));duplicate.fill(0);assert(!drums::notes(a,duplicate,1536,notes,error));s.bars=0;assert(!drums::generate(s,{},b,error));
 for(unsigned genre=0;genre<drums::genres;++genre){s=drums::Settings{};s.genre=genre;assert(drums::generate(s,{},a,error));std::vector<unsigned> signature;for(auto h:a)signature.push_back(h.tick*8+h.role);backbones.insert(signature);}
 assert(backbones.size()>=24);
 std::cout<<cases<<" genre/length/density/swing cases passed; deterministic variations, locked lanes, A-H mapping, Keep/Replace, bounds and distinct styles passed.\n";
}
