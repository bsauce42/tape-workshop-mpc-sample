// SPDX-License-Identifier: AGPL-3.0-only
// Original motif composer; research notes in docs/development/RAP_MELODIES.md.
#pragma once
#include "melodee/model/phrase_line.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace melodee::harmony::rap {
inline unsigned hash(unsigned x){x^=x>>16;x*=0x7feb352d;x^=x>>15;x*=0x846ca68b;return x^(x>>16);}
inline bool enabled(unsigned genre){return genre==1||genre==2;}
// Original onset designs in sixteenths across two bars. 255 ends each motif.
// Every bar has a downbeat; the remaining gaps leave room for drums/vocals.
inline constexpr uint8_t trap[][11]={
 {0,3,6,12,16,19,22,28,255},{0,6,10,16,22,26,30,255},
 {0,2,7,12,16,18,23,28,255},{0,7,10,14,16,23,26,30,255},
 {0,4,11,16,20,27,30,255},{0,3,10,16,19,24,28,255},
 {0,6,12,14,16,22,28,30,255},{0,2,8,14,16,18,24,30,255}};
inline constexpr uint8_t hiphop[][11]={
 {0,3,6,10,16,19,22,27,255},{0,6,8,14,16,22,24,30,255},
 {0,4,7,12,16,20,23,28,255},{0,3,8,11,16,19,24,29,255},
 {0,6,11,16,22,27,30,255},{0,4,10,14,16,20,26,255},
 {0,2,7,10,16,18,23,28,255},{0,7,12,16,23,26,30,255}};
inline constexpr uint8_t rolling[][11]={
 {0,2,6,8,12,16,18,22,24,28,255},{0,3,6,10,14,16,19,22,26,30,255},
 {0,4,6,10,12,16,20,22,26,28,255},{0,2,4,10,14,16,18,20,26,30,255}};
inline constexpr uint8_t space[][11]={
 {0,12,16,28,255},{0,10,16,26,255},{0,8,16,24,30,255},{0,14,16,26,30,255}};
inline const uint8_t* motif(const PhraseSettings& s,unsigned seed){return s.pattern==1?rolling[seed%4]:s.pattern==3?space[seed%4]:s.genre==2?trap[seed%8]:hiphop[seed%8];}
inline bool compose(const Result& r,const PhraseSettings& s,bool melody,std::vector<Note>& notes,std::string& error,unsigned bar){
    notes.clear();bar=std::max(1u,bar);const unsigned seed=hash(s.variation*7919+s.genre*173+s.pattern*31+11);
    const unsigned mask=r.scale_mask?r.scale_mask:(r.minor?0x5ad:0xab5),tonic=r.tonic%12;
    const unsigned base=48+12*s.octave,end=r.chords[r.count-1].tick+r.chords[r.count-1].length;
    std::vector<Note> hits;
    if(s.rhythm){BassSettings timing;timing.rhythm=s.rhythm;timing.gate=s.gate;if(!bass_line(r,timing,ppq,hits,error,bar))return false;}
    else{
        const auto* pattern=motif(s,seed);
        for(uint64_t start=0;start<end;start+=uint64_t(bar)*2){
            unsigned phrase=unsigned(start/(uint64_t(bar)*2));
            for(unsigned n=0;n<11&&pattern[n]!=255;++n){
                if(s.rate==0&&pattern[n]%16!=0&&((s.pattern==0&&(n&1))||(s.pattern==2&&n%3==2)))continue;
                uint64_t tick=start+uint64_t(pattern[n])*bar/16;
                if(s.rate==3){unsigned grid=std::max(1u,ppq/3);tick=start+((tick-start+grid/2)/grid)*grid;}
                if(tick>=end)continue;
                unsigned chord=0;while(chord+1<r.count&&tick>=r.chords[chord+1].tick)++chord;const auto& c=r.chords[chord];
                if(tick<c.tick||tick>=uint64_t(c.tick)+c.length)continue;
                uint64_t next=start+uint64_t(n+1<11&&pattern[n+1]!=255?pattern[n+1]:32)*bar/16;
                if(s.rate==3){unsigned grid=std::max(1u,ppq/3);next=start+((next-start+grid/2)/grid)*grid;}
                unsigned room=unsigned(std::min<uint64_t>(c.tick+c.length,std::max(next,tick+1))-tick);
                // Higher rate permits a restrained short pickup near a phrase ending.
                unsigned span=room;if(s.rate==2&&n>=5)span=std::min(span,std::max(1u,bar/8));
                const int accent=(pattern[n]%16==0?6:pattern[n]%4==0?0:-7)+int(hash(seed+n*71)%7)-3;
                hits.push_back({uint32_t(tick),std::max(1u,span*s.gate/100),0,uint8_t(std::clamp(int(c.velocity)+accent,1,127))});
                if(s.rate==2&&n>=5&&room>=bar/8+2&&((phrase+n)&1)==0){unsigned offset=std::max(1u,bar/16);hits.back().length=std::min(hits.back().length,offset);hits.push_back({uint32_t(tick+offset),std::max(1u,std::min(offset,room-offset)*s.gate/100),0,uint8_t(std::max(1,int(c.velocity)-12))});}
                if(hits.size()>max_bass_notes){notes.clear();error="Phrase too long; choose a shorter region";return false;}
            }
        }
    }
    if(!s.rhythm){std::stable_sort(hits.begin(),hits.end(),[](const Note& a,const Note& b){return a.tick<b.tick;});hits.erase(std::unique(hits.begin(),hits.end(),[](const Note& a,const Note& b){return a.tick==b.tick;}),hits.end());}
    if(hits.empty()){error="No melodic region available";return false;}
    std::array<unsigned,12> scale{};unsigned size=0;for(unsigned n=0;n<12;++n)if(mask&(1u<<n))scale[size++]=n;
    if(!size){error="Empty melody scale";return false;}
    // Generate one short contour, then repeat it with a small answer/cadence.
    int contour[12]{},degree=int(seed%3);for(int n=0;n<12;++n){unsigned roll=hash(seed+n*127);if(n&&n%3!=1)degree+=int(roll%3)-1;degree=std::clamp(degree,-2,4);contour[n]=s.pattern==0?std::clamp(degree,-1,1):s.pattern==1?(n%6<3?n%3:2-n%3)-1:s.pattern==2?degree+(n%6>=3?-1:1):(n%4==2?2:0);}
    int previous=int(base+tonic+scale[std::min(2u,size-1)]);unsigned chord=0;
    for(unsigned i=0;i<hits.size();++i){auto& note=hits[i];while(chord+1<r.count&&note.tick>=r.chords[chord+1].tick)++chord;const auto& c=r.chords[chord];
        const unsigned phase=unsigned(uint64_t(note.tick)*16/bar)%32,phrase=unsigned(uint64_t(note.tick)/(uint64_t(bar)*2));
        int motifIndex=0;const auto* pattern=motif(s,seed);for(unsigned n=0;n<11&&pattern[n]!=255;++n)if(pattern[n]<=phase)motifIndex=int(n);
        int step=contour[motifIndex%12];if((phrase&1)&&phase>=24)step-=1;int degreeIndex=int(std::min(2u,size-1))+step;
        while(degreeIndex<0)degreeIndex+=int(size);
        int target=int(base+tonic)+int(scale[unsigned(degreeIndex)%size])+12*(degreeIndex/int(size));target=std::clamp(target,int(base),int(base+19));
        const bool cadence=i+1==hits.size()||((phrase&1)&&phase>=28),strong=cadence||note.tick%ppq==0;
        if(cadence)target=int(base+c.root%12);
        bool tones[12]{};for(unsigned n=0;n<c.count;++n)tones[c.notes[n]%12]=true;
        double best=std::numeric_limits<double>::max();unsigned pitch=base+c.root%12;
        for(unsigned key=base;key<std::min(128u,base+20);++key){bool tone=tones[key%12];unsigned relative=(key+12-tonic)%12;bool inScale=mask&(1u<<relative);
            // Strong notes and explicit rhythm selections stay chord-based. Brief
            // weak-beat melody notes may connect through the chosen scale.
            if(!tone&&(!melody||strong||s.rhythm||!inScale))continue;
            if(!inScale&&!tone)continue;
            const bool pentatonic=relative==0||relative==(r.minor?3u:4u)||relative==(r.minor?5u:2u)||relative==7||relative==(r.minor?10u:9u);
            double score=std::abs(int(key)-target)+.18*std::abs(int(key)-previous)+(tone?0:.8)+(pentatonic?-.25:0);
            if(std::abs(int(key)-previous)>7)score+=3;
            if(!i&&tone)score-=.5;
            if(score<best){best=score;pitch=key;}
        }
        note.key=uint8_t(pitch);previous=int(pitch);
        // Weak connecting tones are short; anchors may breathe across the gap.
        if(!tones[pitch%12])note.length=std::min(note.length,std::max(1u,ppq/2));
        if(i+1<hits.size())note.length=std::min(note.length,std::max(1u,hits[i+1].tick-note.tick));
    }
    notes=std::move(hits);return true;
}
}
