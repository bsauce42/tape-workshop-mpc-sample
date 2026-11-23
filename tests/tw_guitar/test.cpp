// SPDX-License-Identifier: AGPL-3.0-only
#include <cassert>
#include <cstdlib>
#include <new>
#include <vector>
#include <iostream>
static bool audio=false;
void* operator new(std::size_t n){assert(!audio);if(auto* p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
#include "../../plugins/tape-workshop-guitar/guitar.cpp"
int main(){auto* e=VSTPluginMain(nullptr);assert(e&&e->numParams==18&&e->numPrograms==8);
    constexpr int block=256;float l[block],r[block];float* out[]={l,r};
    auto run=[&]{audio=true;e->processReplacing(e,nullptr,out,block);audio=false;double energy=0;for(int n=0;n<block;++n){assert(std::isfinite(l[n])&&std::isfinite(r[n])&&std::abs(l[n])<=1&&std::abs(r[n])<=1);energy+=l[n]*l[n]+r[n]*r[n];}return energy;};
    auto midi=[&](int status,int note,int velocity,int offset=0){MidiEvent m{};m.type=1;m.byteSize=sizeof(m);m.deltaFrames=offset;m.data[0]=char(status);m.data[1]=char(note);m.data[2]=char(velocity);Events<1> es{};es.count=1;es.events[0]=&m;e->dispatcher(e,op::processEvents,0,0,&es,0);};
    for(int preset=0;preset<8;++preset){e->dispatcher(e,op::mainsChanged,0,0,nullptr,0);e->dispatcher(e,op::setProgram,0,preset,nullptr,0);midi(144,52,100);double energy=0;for(int n=0;n<100;++n)energy+=run();assert(energy>.001);midi(128,52,0);for(int n=0;n<1500;++n)run();}
    e->dispatcher(e,op::mainsChanged,0,0,nullptr,0);e->dispatcher(e,op::setProgram,0,0,nullptr,0);midi(144,60,100,block);assert(run()==0);assert(run()>0);
    for(int note=40;note<80;++note)midi(144,note,100);run();unsigned voices=0;for(auto& v:self(e).voices)voices+=v.active;assert(voices==12);
    midi(176,120,0);run();for(auto& v:self(e).voices)assert(!v.active);
    midi(176,64,127);midi(144,60,100);run();midi(128,60,0);run();bool held=false;for(auto& v:self(e).voices)held|=v.active&&v.sustained;assert(held);midi(176,64,0);run();for(auto& v:self(e).voices)if(v.active)assert(v.released);
    void* chunk=nullptr;auto bytes=e->dispatcher(e,op::getChunk,0,0,&chunk,0);std::vector<char> saved(static_cast<char*>(chunk),static_cast<char*>(chunk)+bytes);set(e,Bright,.1f);assert(e->dispatcher(e,op::setChunk,0,bytes,saved.data(),0));assert(get(e,Bright)==defaults[Bright]);saved[0]^=1;assert(!e->dispatcher(e,op::setChunk,0,bytes,saved.data(),0));
    for(double rate:{8000.,44100.,48000.,96000.,192000.}){e->dispatcher(e,op::setSampleRate,0,0,nullptr,float(rate));for(int i=0;i<Count;++i)set(e,i,1);for(int note:{0,24,40,64,88,127})midi(144,note,127);for(int n=0;n<300;++n)run();}
    // Autocorrelation checks phase-delay compensation across the normal guitar register.
    e->dispatcher(e,op::setSampleRate,0,0,nullptr,44100);
    for(int note:{40,52,64,76}) {
        self(e).reset();self(e).preset(0);set(e,Detune,0);set(e,Room,0);set(e,Echo,0);set(e,Width,0);
        midi(144,note,100);std::vector<float> samples;
        for(int b=0;b<80;++b){run();if(b>=10)samples.insert(samples.end(),l,l+block);}
        double expected=44100/(440*std::pow(2.,(note-69)/12.)),score=1e30;int best=0;
        for(int lag=int(expected*.96);lag<=int(expected*1.04);++lag){double error=0;
            for(unsigned n=0;n<4000;++n){double d=samples[n]-samples[n+lag];error+=d*d;}
            if(error<score){score=error;best=lag;}
        }assert(std::abs(best-expected)<1.5);
    }
    // Strum direction delays by pitch, independent of the submitted note order.
    for(float direction:{0.f,1.f}) {
        self(e).reset();set(e,Strum,.5f);set(e,Direction,direction);
        midi(144,64,100);midi(144,52,100);midi(144,59,100);run();
        unsigned low=0,high=0;for(const auto& v:self(e).voices)if(v.active){if(v.note==52)low=v.wait;if(v.note==64)high=v.wait;}
        assert(direction<.5f?high>low:low>high);
    }
    e->dispatcher(e,op::close,0,0,nullptr,0);std::cout<<"PASS: 8 presets, note timing, 12 voices, release/pedal/panic, state, finite output at 5 rates, allocation-free rendering, guitar tuning and strum direction\n";
}
