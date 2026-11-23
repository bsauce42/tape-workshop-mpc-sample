// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/bass_line.h"
#include <algorithm>
namespace melodee::harmony {
const char* bass_style(unsigned style){const char* names[]={"Held Roots","Quarter Pulse","Root + Fifth","Octave Pulse"};return names[std::min(style,3u)];}
unsigned bass_pitch(const Chord& ch,const BassSettings& s,unsigned step){
    unsigned interval=0;
    if(s.style==2&&(step&1)){interval=7;for(unsigned n=0;n<std::min(5u,unsigned(ch.count));++n){unsigned relative=(ch.notes[n]+12-ch.root%12)%12;if(relative>=6&&relative<=8){interval=relative;break;}}}
    if(s.style==3&&(step&1))interval=12;
    return std::min(127u,12+12*std::min(3u,unsigned(s.octave))+ch.root%12+interval);
}
namespace {
struct Hit { unsigned start, length; int accent=0; };
struct Rhythm { std::array<Hit,16> hits{}; unsigned count=0, bars=1; };
Rhythm pattern(unsigned which) {
    Rhythm r;
    auto add=[&](unsigned start,unsigned length,int accent=0){r.hits[r.count++]={start,length,accent};};
    switch(which) {
    case 7: add(0,12,5);add(14,2,-5);add(16,10,2);add(28,4,-2);break; // classic 808
    case 8: add(0,10,5);add(12,3,-2);add(22,2,2);add(26,2,-4);add(28,2,-2);add(30,2,1);break;
    case 9: add(0,4,5);add(6,4,-2);add(14,2,1);add(20,6,2);add(28,4,-2);break;
    case 10: for(unsigned n=4;n<32;n+=8)add(n,4,-2);break;
    case 11: add(0,8,5);add(12,4,-2);add(16,8,2);add(28,4,-2);break;
    case 12: add(0,10,5);add(12,10,1);add(24,6,2);break;
    case 13: add(0,10,5);add(12,10,1);add(24,6,-2);add(30,2,-4);break;
    case 14: r.bars=2;add(0,16,5);add(16,32,1);add(48,16,3);break;
    case 15: r.bars=2;add(0,32,5);add(32,16,2);add(48,16,-1);break;
    case 16: r.bars=2;add(0,16,5);add(16,32,1);add(48,8,3);add(56,8,-2);break;
    case 17: add(0,28,5);add(28,4,-2);break;
    case 18: add(0,24,5);add(24,8,-2);break;
    case 19: add(0,8,5);add(8,8,-2);add(16,16,2);break;
    case 20: add(0,8,5);add(8,24,1);break;
    case 21: for(unsigned n=0;n<32;n+=8){add(n,4,n?0:5);add(n+4,2,-3);add(n+6,2,-1);}break;
    case 22: add(0,32,5);break;
    case 23: r.bars=2;add(0,24,5);add(28,4,-3);add(32,12,3);add(46,2,-3);add(48,8,1);add(60,2,-4);add(62,2,-2);break;
    default: break;
    }
    return r;
}
}
const char* bass_rhythm(unsigned rhythm) {
    static constexpr const char* names[]={"Style Default","Whole Notes","Half Notes","1/4 Notes","1/8 Notes","1/16 Notes","1/8 Triplets","Classic 808","Trap 808","Syncopated","Offbeat","Charleston","Tresillo 3-3-2","Dotted + Pickup","Half Whole Half","Whole Half Half","Half Whole Classic","Last 1/8 Pickup","Long Short","Two Short Long","Short Long","Gallop","Every Measure","Two-Bar Motion"};
    return names[std::min(rhythm,bass_rhythm_count-1)];
}
bool bass_line(const Result& chords,const BassSettings& s,unsigned beat,std::vector<Note>& notes,std::string& error,unsigned bar) {
    notes.clear();error.clear();
    if(!chords.count||chords.count>max_chords||s.style>3||s.rhythm>=bass_rhythm_count||s.octave>3||s.gate<10||s.gate>100||!beat||beat>65535||bar<16||bar>65535){error="No chords / invalid bass settings";return false;}
    uint32_t previousEnd=0;
    for(unsigned i=0;i<chords.count;++i){const auto& ch=chords.chords[i];
        if(ch.count<3||ch.count>5||ch.root>11||!ch.length||ch.tick<previousEnd||ch.tick>0x0fffffff||ch.length>0x0fffffff-ch.tick){error="Invalid chord timing";return false;}
        previousEnd=ch.tick+ch.length;
    }
    std::vector<Note> made;
    made.reserve(std::min<size_t>(max_bass_notes,chords.count*16));
    for(unsigned i=0;i<chords.count;++i){
        const auto& ch=chords.chords[i];const uint32_t end=ch.tick+ch.length;unsigned step=0;size_t first=made.size();
        auto emit=[&](uint32_t tick,uint32_t length,int accent){
            if(made.size()>=max_bass_notes)return false;
            made.push_back({tick,std::min(length,end-tick),uint8_t(bass_pitch(ch,s,step++)),uint8_t(std::clamp(int(ch.velocity)+accent,1,127))});return true;
        };
        bool ok=true;
        if(s.rhythm<=6){
            unsigned span=ch.length;
            if(!s.rhythm)span=s.style==0?ch.length:s.style==3?std::max(1u,beat/2):beat;
            else {const unsigned spans[]={0,beat*4,beat*2,beat,std::max(1u,beat/2),std::max(1u,beat/4),std::max(1u,beat/3)};span=spans[s.rhythm];}
            if(!s.rhythm){
                for(uint32_t offset=0;offset<ch.length;offset+=span)if(!emit(ch.tick+offset,span,offset?0:4)){ok=false;break;}
            }else{
                ok=emit(ch.tick,span,4);
                for(uint64_t tick=(uint64_t(ch.tick)/span+1)*span;ok&&tick<end;tick+=span)
                    ok=emit(uint32_t(tick),span,0);
            }
        }else{
            const auto r=pattern(s.rhythm);const uint64_t cycle=uint64_t(bar)*r.bars;
            // A root at every harmonic change avoids carrying the previous chord
            // across a rest. Remaining hits stay locked to the measure grid.
            ok=emit(ch.tick,ch.length,5);
            for(uint64_t base=uint64_t(ch.tick)/cycle*cycle;ok&&base<end;base+=cycle){
                for(unsigned n=0;n<r.count;++n){const auto hit=r.hits[n];uint64_t tick=base+uint64_t(hit.start)*bar/32;
                    if(tick<ch.tick||tick>=end)continue;
                    unsigned length=unsigned(std::max<uint64_t>(1,uint64_t(hit.length)*bar/32));
                    if(tick==ch.tick){made[first].length=std::min(length,ch.length);made[first].velocity=uint8_t(std::clamp(int(ch.velocity)+hit.accent,1,127));}
                    else if(!emit(uint32_t(tick),length,hit.accent)){ok=false;break;}
                }
            }
        }
        if(!ok){error="Bass line too dense; choose a slower rhythm";return false;}
        for(size_t n=first;n<made.size();++n){auto& note=made[n];uint32_t stop=n+1<made.size()?made[n+1].tick:end;
            note.length=std::max(1u,unsigned(uint64_t(std::min(note.length,stop-note.tick))*s.gate/100));
        }
    }
    notes=std::move(made);return !notes.empty();
}
}
