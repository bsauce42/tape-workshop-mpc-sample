// SPDX-License-Identifier: AGPL-3.0-only
// Synthetic analysis inputs only; does not play or record audio.
#include "melodee/model/audio_harmony.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace melodee::harmony;
int main(){
    AudioFeatures f(8,1200,96);const int notes[][3]={{60,64,67},{65,69,72},{57,60,64},{55,59,62}};
    for(int n=0;n<8*6000;++n){double sample=0;int chord=n/12000;for(int key:notes[chord]){double hz=440*std::exp2((key-69)/12.);for(unsigned h=1;h<=3;++h)sample+=.12/h*std::sin(6.283185307179586*hz*h*n/6000);}f.push(float(sample));}
    auto features=f.finish();Settings settings;settings.complexity=1;auto result=recognize(features,settings);
    assert(f.ticks()==1536);assert(result.count>=4);unsigned found=0;const unsigned roots[]={0,5,9,7};
    for(unsigned i=0;i<4;++i){bool match=false;for(unsigned j=0;j<result.count;++j){auto c=result.chords[j];if(c.tick<=i*384+192&&c.tick+c.length>i*384+192){std::cout<<i<<": "<<chord_name(c)<<" at "<<c.tick<<" +"<<c.length<<"\n";match=c.root==roots[i];}}assert(match);++found;}
    assert(found==4);assert(result.chords[result.count-1].tick+result.chords[result.count-1].length==1536);
    AudioFeatures silence(2,1200,96);for(unsigned n=0;n<12000;++n)silence.push(0);assert(!recognize(silence.finish(),settings).count);
    AudioFeatures tail(1.123,1200,96);for(unsigned n=0;n<6738;++n)tail.push(float(.2*std::sin(6.283185307179586*440*n/6000)));auto bins=tail.finish();assert(bins.back().tick+bins.back().length==216&&bins.back().frames>0);auto tailResult=recognize(bins,settings);assert(tailResult.count&&tailResult.chords[tailResult.count-1].tick+tailResult.chords[tailResult.count-1].length==216);
    // A 25-bar WAV can end in a silent/faded bar. Keep its final harmony
    // through the file end, without filling intentional rests in the middle.
    std::vector<Chroma> ending(25);
    for(unsigned i=0;i<ending.size();++i){auto& b=ending[i];b.tick=i*384;b.length=384;if(i<24){b.frames=1;b.tones[0]=b.tones[4]=b.tones[7]=1.f/3;}}
    auto full=recognize(ending,settings);assert(full.count);
    assert(full.chords[full.count-1].tick+full.chords[full.count-1].length==25*384);
    ending.back().length=37;auto partial=recognize(ending,settings);assert(partial.count);
    assert(partial.chords[partial.count-1].tick+partial.chords[partial.count-1].length==24*384+37);
    ending[12].frames=0;auto rests=recognize(ending,settings);assert(rests.count);
    for(unsigned i=0;i<rests.count;++i)assert(!(rests.chords[i].tick<13*384&&rests.chords[i].tick+rests.chords[i].length>12*384));
    for(auto& b:ending)b.frames=0;assert(!recognize(ending,settings).count);
    AudioFeatures faded(50,1200,96);
    for(unsigned n=0;n<300000;++n)faded.push(n<288000?float(.2*std::sin(6.283185307179586*440*n/6000)):0.f);
    auto fadedResult=recognize(faded.finish(),settings);assert(fadedResult.count);
    assert(fadedResult.chords[fadedResult.count-1].tick+fadedResult.chords[fadedResult.count-1].length==25*384);
    // Actual Business 138 BPM WAV: frame quantization is only .00174 tick
    // beyond 24 bars and must not create an empty 25th bar.
    AudioFeatures business(1840696./44100,1380,96);assert(business.ticks()==24*384);
    AudioFeatures partialBar((24*384+24)*60./(138*96),1380,96);assert(partialBar.ticks()==24*384+24);
    AudioFeatures tiny(1./44100,1380,96);assert(tiny.ticks()==1);
    AudioFeatures empty(0,1380,96);assert(empty.ticks()==0);
    AudioFeatures longFile(200,1200,24);assert(longFile.finish().size()<=max_chords);
    std::cout<<"Whole-stream changes, tail coverage, silence and bounded memory passed; no audio played.\n";
}
