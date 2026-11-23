// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/bar_edit.h"
#include <set>
#include <tuple>
namespace melodee {
std::vector<harmony::Chord> bar_chords(unsigned root,unsigned mask){
    static constexpr int intervals[11][5]={{0,4,7},{0,3,7},{0,3,6},{0,4,8},{0,4,7,11},{0,3,7,10},{0,4,7,10},{0,3,6,10},{0,4,7,11,14},{0,3,7,10,14},{0,4,7,10,14}};
    std::vector<harmony::Chord> out;
    for(unsigned degree=0;degree<12;++degree)for(unsigned quality=0;quality<11;++quality){
        harmony::Chord c;c.root=(root+degree)%12;c.quality=quality;c.count=quality<4?3:quality<8?4:5;bool fits=true;
        for(unsigned n=0;n<c.count;++n){unsigned d=(degree+intervals[quality][n])%12;fits&=(mask&(1u<<d))!=0;c.notes[n]=uint8_t(48+c.root+intervals[quality][n]);}
        if(fits)out.push_back(c);
    }
    return out;
}
uint64_t bar_fingerprint(const Pattern& p){uint64_t h=1469598103934665603ull;auto add=[&](uint64_t v){h^=v;h*=1099511628211ull;};add(p.length);add(p.events.size());for(auto e:p.events){add(e.tick);add(e.length);add(e.key);add(e.pitch);add(e.velocity);add(e.chance);add(e.flags);add(e.ratchet);add(e.ramp);add(e.pitch_ramp);add(e.condition);add(e.cutoff);}return h;}
bool bar_replace(const Pattern& src,unsigned from,unsigned to,int lane,const std::vector<Event>& notes,Pattern& out,std::string& error){
    error.clear();if(from>=to||to>src.length){error="Invalid bar range";return false;}Pattern made=src;made.events.clear();
    for(auto e:src.events){unsigned end=e.tick+e.length;if((lane>=0&&e.key!=lane)||end<=from||e.tick>=to){made.events.push_back(e);continue;}
        if(e.tick<from){auto left=e;left.length=uint16_t(from-e.tick);made.events.push_back(left);}
        if(end>to){e.tick=to;e.length=uint16_t(end-to);made.events.push_back(e);}
    }
    for(auto e:notes){if(e.tick<from||!e.length||e.tick+e.length>to||(lane>=0&&e.key!=lane)){error="New notes leave selected bar";return false;}made.events.push_back(e);}
    if(made.events.size()>max_pattern_events){error="Pattern full; no notes changed";return false;}
    std::stable_sort(made.events.begin(),made.events.end(),[](auto a,auto b){return std::tie(a.tick,a.key,a.pitch)<std::tie(b.tick,b.key,b.pitch);});
    std::set<std::tuple<unsigned,unsigned,int>> seen;for(auto e:made.events)if(!seen.emplace(e.tick,e.key,e.pitch).second){error="Conflicting notes at bar edge";return false;}
    out=std::move(made);return true;
}
bool bar_octave(const Pattern& src,const std::vector<size_t>& selected,int octaves,int lane,Pattern& out,std::string& error){
    error.clear();if(selected.empty()||!octaves||octaves< -8||octaves>8){error="Choose notes and a nonzero octave shift";return false;}Pattern made=src;
    for(auto n:selected){if(n>=made.events.size()){error="Selection changed";return false;}auto& e=made.events[n];int key=lane>=0?int(e.pitch):int(e.key);int value=key+12*octaves;if(value<(lane>=0?-48:0)||value>(lane>=0?48:127)||(lane>=0&&e.key!=lane)){error="Octave shift exceeds note range";return false;}if(lane>=0)e.pitch=int8_t(value);else e.key=uint8_t(value);}
    std::set<std::tuple<unsigned,unsigned,int>> seen;for(auto e:made.events)if(!seen.emplace(e.tick,e.key,e.pitch).second){error="Octave shift collides with existing notes";return false;}
    std::stable_sort(made.events.begin(),made.events.end(),[](auto a,auto b){return std::tie(a.tick,a.key,a.pitch)<std::tie(b.tick,b.key,b.pitch);});out=std::move(made);return true;
}
}
