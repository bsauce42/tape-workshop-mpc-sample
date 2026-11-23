// SPDX-License-Identifier: AGPL-3.0-only
// Shared, bounded melody generation for Tape Workshop plug-ins and their host display.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>
namespace melodee::melody {
inline constexpr unsigned parameters=13;
inline constexpr std::array<const char*,parameters> names {
    "Melody Play","New Melody","Key","Scale","Note Speed","Octave","Note Range","Pattern Steps",
    "Note Gate","Swing","Density","Melody Velocity","Follow Project"
};
inline constexpr std::array<const char*,13> scale_names {"Major","Minor","Dorian","Phrygian","Lydian","Mixolydian","Locrian","Harm Minor","Mel Minor","Major Pent","Minor Pent","Blues","Chromatic"};
inline constexpr std::array<const char*,12> keys {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
inline constexpr std::array<const char*,4> speeds {"1/16","1/8","1/4","1/2"};
inline int parameter_base(std::string_view name,unsigned count){
    if(name=="Tape Workshop Strings"&&count==43)return 30;
    if(name=="Tape Workshop Orchestra"&&count==48)return 35;
    return -1;
}
struct Settings {
    bool enabled=false;unsigned variation=0,key=0,scale=1,speed=1,octave=2,range=1,steps=16;
    float gate=.65f,swing=0,density=.9f,velocity=.8f;
};
inline unsigned choice(float value,unsigned maximum){return unsigned(std::lround(std::clamp(value,0.f,1.f)*maximum));}
inline Settings settings(const std::array<float,parameters>& p){
    Settings s;s.enabled=p[0]>.5f;s.variation=choice(p[1],999);s.key=choice(p[2],11);s.scale=choice(p[3],12);
    s.speed=choice(p[4],3);s.octave=choice(p[5],3);s.range=1+choice(p[6],2);s.steps=8u<<choice(p[7],2);
    s.gate=.1f+.85f*std::clamp(p[8],0.f,1.f);s.swing=.45f*std::clamp(p[9],0.f,1.f);
    s.density=std::clamp(p[10],0.f,1.f);s.velocity=std::clamp(p[11],0.f,1.f);return s;
}
inline uint32_t hash(uint32_t v){v^=v>>16;v*=0x7feb352du;v^=v>>15;v*=0x846ca68bu;return v^(v>>16);}
inline int note(const Settings& s,unsigned index){
    constexpr unsigned masks[]={0xab5,0x5ad,0x6ad,0x5ab,0xad5,0x6b5,0x56b,0x9ad,0xaad,0x295,0x4a9,0x4e9,0xfff};
    unsigned scale=std::min(s.scale,12u),size=0;std::array<int,12> degrees{};
    for(unsigned n=0;n<12;++n)if(masks[scale]&(1u<<n))degrees[size++]=int(n);
    index%=std::clamp(s.steps,8u,32u);int degree=0;
    for(unsigned n=1;n<=index;++n){uint32_t h=hash(s.variation*7919+n*104729);constexpr int moves[]={-2,-1,1,2,0,1,-1,3};degree=std::clamp(degree+moves[h&7],0,int(size*s.range)-1);}
    if(index+1==s.steps)degree=0;
    return std::clamp(36+12*int(std::min(s.octave,3u))+int(s.key%12)+12*(degree/int(size))+degrees[degree%int(size)],0,127);
}
inline bool sounding(const Settings& s,unsigned step){
    if(s.density<=0)return false;
    return step%s.steps==0 || (hash(s.variation*3571+(step%s.steps)*5381+17)&65535)<uint32_t(s.density*65536);
}
inline unsigned velocity(const Settings& s,unsigned step){return unsigned(std::clamp(int(27+s.velocity*90)+int(hash(s.variation+step*31)%17)-8,1,127));}
inline double beats(const Settings& s){constexpr double rates[]={.25,.5,1,2};return rates[std::min(s.speed,3u)];}
inline double onset(const Settings& s,int64_t step){return (double(step)+((step&1)?s.swing:0))*beats(s);}
inline double duration(const Settings& s,int64_t step){return std::max(.01,(onset(s,step+1)-onset(s,step))*s.gate);}
inline uint32_t signature(const Settings& s){uint32_t h=s.variation;for(unsigned v:{s.key,s.scale,s.speed,s.octave,s.range,s.steps})h=hash(h^v);return h;}
// No allocation or audio dependency. Call once per sample; callbacks only start/release owned melody voices.
struct Clock {
    int64_t last=-1;double offAt=-1,expected=0;uint32_t config=0;bool running=false,held=false;
    template<class Off>void stop(Off off){if(held)off();held=false;running=false;last=-1;offAt=-1;}
    template<class On,class Off>void tick(const Settings& s,bool playing,double ppq,double increment,On on,Off off){
        if(!s.enabled||!playing||!std::isfinite(ppq)||ppq<0||ppq>1e10){stop(off);return;}
        uint32_t now=signature(s);
        if(!running||config!=now||std::abs(ppq-expected)>.02){stop(off);running=true;config=now;}
        expected=ppq+increment;
        if(held&&ppq+1e-10>=offAt){off();held=false;}
        const int64_t step=int64_t(std::floor(ppq/beats(s)+1e-9));
        if(last!=step&&ppq+1e-10>=onset(s,step)){
            if(held){off();held=false;}last=step;
            const unsigned at=unsigned(step%std::clamp(s.steps,8u,32u));offAt=onset(s,step)+duration(s,step);
            if(ppq<offAt&&sounding(s,at)){on(note(s,at),velocity(s,at));held=true;}
        }
    }
};
}
