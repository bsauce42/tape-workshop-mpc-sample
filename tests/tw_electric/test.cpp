// SPDX-License-Identifier: AGPL-3.0-only
#include <cassert>
#include <cstdlib>
#include <new>
#include <vector>
#include <iostream>
static bool audio=false;
void* operator new(std::size_t n){assert(!audio);if(auto* p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
#include "../../plugins/tape-workshop-electric/electric.cpp"
int main(){auto* e=VSTPluginMain(nullptr);assert(e&&e->numParams==38&&e->uniqueId==int32_t(electric::uid));
    constexpr int frames=256;float l[frames],r[frames];float* out[]={l,r};
    auto run=[&]{audio=true;e->processReplacing(e,nullptr,out,frames);audio=false;double energy=0;for(int i=0;i<frames;++i){assert(std::isfinite(l[i])&&std::isfinite(r[i])&&std::abs(l[i])<=1&&std::abs(r[i])<=1);energy+=l[i]*l[i]+r[i]*r[i];}return energy;};
    auto note=[&](int pitch){MidiEvent m{};m.type=1;m.byteSize=sizeof(m);m.data[0]=char(144);m.data[1]=char(pitch);m.data[2]=100;Events<1> es{};es.count=1;es.events[0]=&m;e->dispatcher(e,op::processEvents,0,0,&es,0);};
    for(int p=0;p<8;++p){e->dispatcher(e,op::mainsChanged,0,0,nullptr,0);e->dispatcher(e,op::setProgram,0,p,nullptr,0);note(40);note(47);note(52);double energy=0;for(int n=0;n<150;++n)energy+=run();assert(energy>.001);}
    // Every rack effect in every position across all amp/cab choices.
    for(int type=0;type<10;++type)for(int amp=0;amp<4;++amp){
        e->dispatcher(e,op::mainsChanged,0,0,nullptr,0);e->dispatcher(e,op::setProgram,0,0,nullptr,0);
        e->setParameter(e,electric::Amp,amp/3.f);e->setParameter(e,electric::Cabinet,amp/3.f);
        for(int slot=0;slot<3;++slot){e->setParameter(e,electric::Pedal1+slot*3,type/9.f);e->setParameter(e,electric::Amount1+slot*3,1);}
        note(52);for(int n=0;n<100;++n)run();
    }
    void* ptr=nullptr;auto bytes=e->dispatcher(e,op::getChunk,0,0,&ptr,0);std::vector<char> saved(static_cast<char*>(ptr),static_cast<char*>(ptr)+bytes);float old=e->getParameter(e,electric::Amp);e->setParameter(e,electric::Amp,0);assert(e->dispatcher(e,op::setChunk,0,bytes,saved.data(),0));assert(e->getParameter(e,electric::Amp)==old);saved[0]^=1;assert(!e->dispatcher(e,op::setChunk,0,bytes,saved.data(),0));
    // Exercise effect transitions while notes are active.
    for(int type=0;type<10;++type){for(int slot=0;slot<3;++slot)e->setParameter(e,electric::Pedal1+slot*3,type/9.f);for(int n=0;n<20;++n)run();}
    for(float rate:{8000,44100,48000,96000,192000}){e->dispatcher(e,op::setSampleRate,0,0,nullptr,rate);for(int i=0;i<38;++i)e->setParameter(e,i,1);for(int pitch:{24,40,52,64,76,127})note(pitch);for(int n=0;n<200;++n)run();}
    e->dispatcher(e,op::close,0,0,nullptr,0);std::cout<<"PASS: 8 presets, 10 pedal choices x 4 amp/cab models, rack transitions, state, 5 sample rates, finite allocation-free audio\n";
}
