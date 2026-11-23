// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/phrase_line.h"
#include "melodee/model/rap_phrase.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace melodee::harmony {
namespace {
unsigned random(unsigned x){x^=x>>16;x*=0x7feb352d;x^=x>>15;x*=0x846ca68b;return x^(x>>16);}
bool valid(const Result& r){if(!r.count||r.count>max_chords)return false;uint32_t end=0;for(unsigned n=0;n<r.count;++n){const auto& c=r.chords[n];if(c.count<3||c.count>5||c.root>11||c.tick<end||!c.length||c.length>0x0fffffff||c.tick>0x0fffffff-c.length)return false;for(unsigned k=0;k<c.count;++k)if(c.notes[k]>127)return false;end=c.tick+c.length;}return true;}
unsigned stride(const PhraseSettings& s,unsigned beat){beat=std::max(1u,beat);const unsigned divisors[]={1,2,4,3};return std::max(1u,beat/divisors[s.rate]);}
}
const char* output_name(unsigned output){const char* names[]={"CHORDS","BASS LINE","PLUCK","MELODY","HYBRID"};return names[std::min(output,4u)];}
const char* phrase_rate(unsigned rate){const char* names[]={"1/4","1/8","1/16","1/8T"};return names[std::min(rate,3u)];}
const char* phrase_pattern(unsigned pattern,bool melody,unsigned genre){if(rap::enabled(genre)){const char* names[]={"Hook","Rolling",genre==2?"Dark":"Soulful","Space"};return names[std::min(pattern,3u)];}const char* arp[]={"Up","Down","Bounce","Shuffle"};const char* lead[]={"Rise","Fall","Wave","Explore"};return (melody?lead:arp)[std::min(pattern,3u)];}
OutputSettings normalize_output(OutputSettings s){if(s.output==4){s.melody.octave=std::max<uint8_t>(s.melody.octave,1);}return s;}
bool phrase_line(const Result& r,const PhraseSettings& s,unsigned beat,bool melody,std::vector<Note>& notes,std::string& error,unsigned bar){
    notes.clear();error.clear();if(!valid(r)||s.rate>3||s.octave>3||s.gate<10||s.gate>100||s.pattern>3||s.variation>999||s.rhythm>=bass_rhythm_count||s.genre>music::genre_count){error="No harmony / invalid phrase settings";return false;}
    if(rap::enabled(s.genre))return rap::compose(r,s,melody,notes,error,bar);
    const auto style=music::profile(s.genre);
    if(s.rhythm){
        BassSettings timing;timing.rhythm=s.rhythm;timing.gate=s.gate;
        std::vector<Note> hits;if(!bass_line(r,timing,ppq,hits,error,bar))return false;
        const unsigned base=48+12*s.octave;int previous=int(base+r.chords[0].root);unsigned chordIndex=0,step=0;
        for(auto& hit:hits){
            while(chordIndex+1<r.count&&hit.tick>=r.chords[chordIndex+1].tick){++chordIndex;step=0;}
            const auto& c=r.chords[chordIndex];std::array<bool,12> tones{};for(unsigned n=0;n<c.count;++n)tones[c.notes[n]%12]=true;
            std::array<unsigned,12> arp{};unsigned size=0;for(unsigned n=0;n<12;++n)if(tones[(c.root+n)%12])arp[size++]=base+c.root+n;
            const unsigned seed=random(s.variation*7919+chordIndex*104729+step*3571+29);unsigned pitch=arp[0];
            if(!melody){unsigned at=step+(s.variation+style.rotation)%size;switch(s.pattern){case 0:at%=size;break;case 1:at=size-1-at%size;break;case 2:{unsigned period=2*size-2;if(!period)at=0;else{at%=period;at=at<size?at:period-at;}break;}default:at=seed%size;break;}pitch=arp[at];}
            else{constexpr int shape[]={0,3,5,2,7,5,2,0};int contour=shape[(step+s.variation%8)%8];int target=int(base+c.root)+contour;
                if(s.pattern==1)target=int(base+c.root)+12-contour;else if(s.pattern==2)target=previous+((step/4)%2?-int(style.hop):int(style.hop));else if(s.pattern==3)target=previous+int(seed%9)-4;
                if(!step)target=int(base+c.root);else if(s.genre&&s.pattern<2)target+=int(style.hop)-3;double best=std::numeric_limits<double>::max();for(unsigned n=base;n<base+24;++n)if(tones[n%12]){double score=std::abs(int(n)-target)*.65+std::abs(int(n)-previous)*.35+(random(seed+n*127)%1000)/1500.;if(!step&&n==base+c.root)score-=12;if(score<best){best=score;pitch=n;}}
            }
            hit.key=uint8_t(pitch);previous=int(pitch);++step;
        }
        notes=std::move(hits);return true;
    }
    (void)beat;const unsigned grid=stride(s,ppq),base=48+12*s.octave;size_t maximum=0;
    for(unsigned i=0;i<r.count;++i){maximum+=(uint64_t(r.chords[i].length)+grid-1)/grid;if(maximum>max_bass_notes){error="Phrase too dense; choose a slower rate";return false;}}
    std::vector<Note> made;made.reserve(maximum);int previous=int(base+r.chords[0].root);
    for(unsigned i=0;i<r.count;++i){const auto& c=r.chords[i];std::array<bool,12> tones{};for(unsigned k=0;k<c.count;++k)tones[c.notes[k]%12]=true;
        std::vector<unsigned> arp;for(unsigned n=0;n<12;++n)if(tones[(c.root+n)%12])arp.push_back(base+c.root+n);
        unsigned step=0;for(uint32_t offset=0;offset<c.length;){
            const unsigned seed=random(s.variation*7919+i*104729+step*3571+29);unsigned span=std::min(grid,c.length-offset);
            if(melody&&step&&seed%7==0){offset+=span;++step;continue;}
            unsigned pitch=arp[0];
            if(!melody){unsigned at=step+(s.variation+style.rotation)%arp.size(),size=unsigned(arp.size());switch(s.pattern){case 0:at%=size;break;case 1:at=size-1-at%size;break;case 2:{unsigned period=2*size-2;if(!period)at=0;else{at%=period;at=at<size?at:period-at;}break;}default:at=seed%size;break;}pitch=arp[at];}
            else {
                constexpr int shape[]={0,3,5,2,7,5,2,0};int contour=shape[(step+s.variation%8)%8];
                int target=int(base+c.root)+contour;if(s.pattern==1)target=int(base+c.root)+12-contour;else if(s.pattern==2)target=previous+((step/4)%2?-int(style.hop):int(style.hop));else if(s.pattern==3)target=previous+int(seed%9)-4;
                if(!step)target=int(base+c.root);else if(s.genre&&s.pattern<2)target+=int(style.hop)-3;double best=std::numeric_limits<double>::max();
                for(unsigned n=base;n<base+24;++n)if(tones[n%12]){double score=std::abs(int(n)-target)*.65+std::abs(int(n)-previous)*.35+(random(seed+n*127)%1000)/1500.;if(!step&&n==base+c.root)score-=12;if(score<best){best=score;pitch=n;}}
                if(step&&seed%4==0)span=std::min<uint32_t>(grid*2,c.length-offset);
            }
            unsigned gate=std::max(1u,unsigned(uint64_t(span)*s.gate/100));made.push_back({c.tick+offset,gate,uint8_t(pitch),uint8_t(std::clamp(int(c.velocity)+(step?int(seed%11)-5:4),1,127))});previous=int(pitch);offset+=span;++step;
        }
    }
    notes=std::move(made);return !notes.empty();
}
bool output_notes(const Result& chords,const OutputSettings& requested,unsigned beat,std::vector<Note>& notes,std::string& error,unsigned bar){
    notes.clear();error.clear();if(requested.output>4||requested.genre>music::genre_count||!valid(chords)){error="No valid harmony / output";return false;}const auto s=normalize_output(requested);std::vector<Note> made;
    const unsigned rhythm=effective_rhythm(s);if(rhythm>=bass_rhythm_count){error="Invalid rhythm";return false;}
    if(s.output==0||s.output==4){
        std::vector<Note> hits;
        if(rhythm){BassSettings timing;timing.rhythm=rhythm;timing.gate=100;if(!bass_line(chords,timing,ppq,hits,error,bar))return false;}
        else for(unsigned i=0;i<chords.count;++i){const auto& c=chords.chords[i];hits.push_back({c.tick,c.length,0,c.velocity});}
        unsigned chordIndex=0;
        for(auto hit:hits){while(chordIndex+1<chords.count&&hit.tick>=chords.chords[chordIndex+1].tick)++chordIndex;const auto& c=chords.chords[chordIndex];
            if(made.size()+c.count>max_bass_notes){error="Chord rhythm too dense; choose a slower rhythm";return false;}
            for(unsigned n=0;n<c.count;++n)made.push_back({hit.tick,hit.length,uint8_t(s.output==4?48+c.notes[n]%12:c.notes[n]),hit.velocity});
        }
    }
    if(s.output==1){if(!bass_line(chords,s.bass,ppq,made,error,bar))return false;}
    if(s.output>=2){std::vector<Note> part;auto phrase=s.output==2?s.pluck:s.melody;if(s.output==4)phrase.rhythm=s.hybrid_rhythm;
        if(s.genre){auto profile=music::profile(s.genre);if(!phrase.rhythm&&!rap::enabled(s.genre))phrase.rhythm=s.output==2?profile.pluck:profile.melody;phrase.genre=s.genre;}
        if(!phrase_line(chords,phrase,beat,s.output!=2,part,error,bar))return false;
        if(made.size()+part.size()>max_bass_notes){error="Combined output too dense; choose a slower rhythm";return false;}made.insert(made.end(),part.begin(),part.end());
    }
    if(made.empty()||made.size()>max_bass_notes){error="Combined output too dense; lower phrase rate";return false;}
    std::stable_sort(made.begin(),made.end(),[](const Note&a,const Note&b){return a.tick!=b.tick?a.tick<b.tick:a.key<b.key;});
    if(s.genre){int offset=music::profile(s.genre).velocity;const unsigned swing=!output_rhythm(s)&&s.output!=1?(music::swing(s.genre)-50)*48/100:0;for(auto& note:made){note.velocity=uint8_t(std::clamp(int(note.velocity)+offset,1,127));if(swing&&note.tick%48==24){unsigned move=std::min(swing,note.length-1);note.tick+=move;note.length-=move;}}}
    notes=std::move(made);return true;
}
unsigned output_preview(const Chord& chord,const OutputSettings& requested,std::array<uint8_t,8>& notes){
    auto s=normalize_output(requested);unsigned count=0;
    if(s.output==0||s.output==4)for(unsigned k=0;k<std::min(5u,unsigned(chord.count));++k)notes[count++]=uint8_t(s.output==4?48+chord.notes[k]%12:chord.notes[k]);
    if(s.output==1)notes[count++]=uint8_t(bass_pitch(chord,s.bass));
    if(s.output==2)notes[count++]=uint8_t(48+12*s.pluck.octave+chord.root%12);
    if(s.output==3||s.output==4)notes[count++]=uint8_t(48+12*s.melody.octave+chord.root%12);
    return count;
}
}
