// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/plugin/vst2_abi.h"
#include <cassert>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <vector>
int main(int argc,char** argv){
    assert(argc==2);auto* lib=dlopen(argv[1],RTLD_NOW);if(!lib){std::puts(dlerror());return 1;}
    auto entry=reinterpret_cast<vst2::EntryPoint>(dlsym(lib,"VSTPluginMain"));assert(entry);
    auto* e=entry(nullptr);assert(e&&e->numPrograms==24);e->dispatcher(e,10,0,0,nullptr,48000);
    float l[128]{},r[128]{};float* out[]={l,r};
    auto midi=[&](int type,int key,int vel){vst2::MidiEvent m{};m.type=1;m.byteSize=sizeof(m);m.data[0]=char(type);m.data[1]=char(key);m.data[2]=char(vel);vst2::Events<1> es{};es.count=1;es.events[0]=&m;e->dispatcher(e,25,0,0,&es,0);};
    auto render=[&](int blocks){double energy=0;for(int b=0;b<blocks;++b){e->processReplacing(e,nullptr,out,128);for(int i=0;i<128;++i){assert(std::isfinite(l[i])&&std::isfinite(r[i]));assert(std::abs(l[i])<=1&&std::abs(r[i])<=1);energy+=l[i]*l[i]+r[i]*r[i];}}return energy;};
    for(int p=0;p<24;++p){e->dispatcher(e,12,0,0,nullptr,0);e->dispatcher(e,2,0,p,nullptr,0);midi(0x90,60,100);assert(render(400)>.001);midi(0x80,60,0);render(1400);assert(render(20)<.00001);}
    e->dispatcher(e,12,0,0,nullptr,0);e->dispatcher(e,2,0,5,nullptr,0);
    for(int n:{60,64,67})midi(0x90,n,100);
    render(7500); // Twenty seconds: much longer than the source sample.
    assert(render(375)>.001); // Still sounding while held.
    midi(0xb0,64,127);for(int n:{60,64,67})midi(0x80,n,0);assert(render(375)>.001);
    midi(0xb0,64,0);render(2500);assert(render(20)<.00001);
    // Every movement target must alter the held sound and remain finite.
    for(int target=0;target<4;++target){e->dispatcher(e,12,0,0,nullptr,0);e->setParameter(e,23,target/3.f);e->setParameter(e,21,.8f);e->setParameter(e,20,.7f);midi(0x90,60,100);assert(render(1000)>.001);midi(0x80,60,0);render(2500);}
    void* state=nullptr;auto size=e->dispatcher(e,23,0,0,&state,0);assert(size>0);
    std::vector<char> copy(static_cast<char*>(state),static_cast<char*>(state)+size);float saved=e->getParameter(e,12);e->setParameter(e,12,0);assert(e->dispatcher(e,24,0,size,copy.data(),0));assert(e->getParameter(e,12)==saved);
    e->setParameter(e,12,NAN);assert(std::isfinite(e->getParameter(e,12)));
    for(int n=0;n<48;++n)midi(0x90,40+n,127);
    auto start=std::chrono::steady_clock::now();render(3750);double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::printf("16 held voices, 10 seconds rendered in %.3fs on desktop (not MPC CPU).\n",seconds);
    midi(0xb0,120,0);render(3000);assert(render(20)<.00001);
    auto* original=e;
    for(int target=0;target<4;++target){
        std::array<float,256> reference{};
        for(int variant=0;variant<2;++variant){
            e=entry(nullptr);assert(e);e->dispatcher(e,10,0,0,nullptr,48000);
            e->setParameter(e,17,0);e->setParameter(e,23,target/3.f);
            e->setParameter(e,21,variant?.9f:0);e->setParameter(e,20,.7f);
            midi(0x90,64,100);render(300);
            double difference=0;
            for(int i=0;i<128;++i){if(!variant){reference[i]=l[i];reference[128+i]=r[i];}
                else difference+=std::abs(l[i]-reference[i])+std::abs(r[i]-reference[128+i]);}
            if(variant)assert(difference>.00001);
            e->dispatcher(e,1,0,0,nullptr,0);
        }
    }
    e=original;
    e->dispatcher(e,1,0,0,nullptr,0);dlclose(lib);std::puts("Programs, held chord loops, release, sustain pedal, state, finite output and voice stealing passed.");
}
