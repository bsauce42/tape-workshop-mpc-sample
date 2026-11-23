// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/audio_harmony.h"
#include <algorithm>
#include <cmath>
namespace melodee::harmony {
AudioFeatures::AudioFeatures(double seconds,unsigned tempo,unsigned beat){
    seconds=std::isfinite(seconds)?std::clamp(seconds,0.,86400.):0.;
    ticks_per_sample=std::clamp(tempo,100u,9990u)*96./3600000.;
    // Audio frame counts rarely land exactly on MIDI ticks. Ceiling a tiny
    // frame-rounding remainder can create a whole extra empty Roll bar.
    // Quantize to the nearest representable tick, retaining short nonempty files.
    total_ticks=seconds>0?std::max(1u,uint32_t(std::llround(seconds*6000*ticks_per_sample))):0;
    beat=std::max(1u,beat);bin_ticks=beat*std::max(1u,unsigned(std::ceil(double(total_ticks)/beat/max_chords)));
    for(uint32_t t=0;t<total_ticks&&bins.size()<max_chords;t+=bin_ticks){Chroma b;b.tick=t;b.length=std::min(bin_ticks,total_ticks-t);bins.push_back(b);}
    constexpr double pi=3.141592653589793;
    for(unsigned i=0;i<window;++i){hann[i]=float(.5-.5*std::cos(2*pi*i/(window-1)));roots[i]=std::polar(1.f,float(-2*pi*i/window));unsigned n=i,r=0;for(unsigned j=0;j<12;++j){r=(r<<1)|(n&1);n>>=1;}reverse[i]=r;}
}
void AudioFeatures::push(float sample){ring[samples%window]=std::isfinite(sample)?sample:0;++samples;if(samples>=window&&(samples-window)%hop==0)frame();}
void AudioFeatures::frame(bool tail){
    if(bins.empty())return;
    const uint64_t used=std::min<uint64_t>(samples,window),start=samples-used;
    float energy=0;
    for(unsigned i=0;i<window;++i){float x=i<used?ring[(start+i)%window]*hann[i]:0;energy+=x*x;spectrum[reverse[i]]={x,0};}
    if(energy/window<1e-8f)return;
    for(unsigned size=2;size<=window;size*=2)for(unsigned i=0;i<window;i+=size)for(unsigned j=0;j<size/2;++j){auto v=spectrum[i+j+size/2]*roots[j*window/size],u=spectrum[i+j];spectrum[i+j]=u+v;spectrum[i+j+size/2]=u-v;}
    const double center=tail?double(samples)-1:double(start)+used*.5;
    const auto tick=uint32_t(std::max(0.,center*ticks_per_sample));auto& bin=bins[std::min<size_t>(bins.size()-1,tick/bin_ticks)];
    float maximum=0;for(unsigned k=28;k<1366;++k)maximum=std::max(maximum,std::abs(spectrum[k]));
    std::array<float,12> tones{},bass{};
    for(unsigned k=28;k<1366;++k){float mag=std::abs(spectrum[k]);if(mag<maximum*.018f||mag<std::abs(spectrum[k-1])||mag<std::abs(spectrum[k+1]))continue;
        float a=std::abs(spectrum[k-1]),b=mag,c=std::abs(spectrum[k+1]);float offset=std::clamp(.5f*(a-c)/(a-2*b+c+1e-12f),-.5f,.5f);
        double hz=(k+offset)*6000./window,midi=69+12*std::log2(hz/440.);int note=int(std::lround(midi));if(note<28||note>95||std::abs(midi-note)>.4)continue;
        float prominence=std::max(0.f,mag-.5f*(std::abs(spectrum[k-3])+std::abs(spectrum[k+3])));
        float value=std::sqrt(prominence)*float(std::pow(220./hz,.45));tones[note%12]+=value;if(hz<330)bass[note%12]+=value;
    }
    float sum=0,low=0;for(unsigned n=0;n<12;++n){sum+=tones[n];low+=bass[n];}if(sum<=0)return;
    for(unsigned n=0;n<12;++n){bin.tones[n]+=tones[n]/sum;bin.bass[n]+=low>0?bass[n]/low:0;}++bin.frames;
}
std::vector<Chroma> AudioFeatures::finish(){if(samples)frame(true);for(auto& b:bins)if(b.frames)for(unsigned n=0;n<12;++n){b.tones[n]/=b.frames;b.bass[n]/=b.frames;}return std::move(bins);}
}
