// SPDX-License-Identifier: AGPL-3.0-only
// Render-only headroom and mute checks, no device audio playback.
#include "melodee/plugin/vst2_abi.h"
#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstdio>
#include <dlfcn.h>
using namespace vst2;
int main(int argc,char**argv){assert(argc==2);auto lib=dlopen(argv[1],RTLD_NOW);assert(lib);auto entry=(EntryPoint)dlsym(lib,"VSTPluginMain");assert(entry);double peak=0;unsigned hot=0,samples=0;
for(unsigned mode=0;mode<2;++mode){for(int preset=0;preset<32;++preset){auto* e=entry(nullptr);if(preset>=e->numPrograms){e->dispatcher(e,1,0,0,nullptr,0);break;}e->dispatcher(e,10,0,0,nullptr,48000);e->dispatcher(e,2,0,preset,nullptr,0);auto note=[&](int type,int key,int vel){MidiEvent m{};m.type=1;m.byteSize=sizeof(m);m.data[0]=char(type);m.data[1]=char(key);m.data[2]=char(vel);Events<1> es{};es.count=1;es.events[0]=&m;e->dispatcher(e,25,0,0,&es,0);};if(mode)for(int key=48;key<64;++key)note(0x90,key,127);else for(int key:{60,64,67,71})note(0x90,key,100);
float l[256]{},r[256]{};float* out[]={l,r};for(unsigned b=0;b<375;++b){e->processReplacing(e,nullptr,out,256);for(unsigned i=0;i<256;++i){assert(std::isfinite(l[i])&&std::isfinite(r[i]));double p=std::max(std::abs(l[i]),std::abs(r[i]));assert(p<=1);peak=std::max(peak,p);hot+=p>.99;++samples;}}
e->setParameter(e,19,0);for(unsigned b=0;b<100;++b)e->processReplacing(e,nullptr,out,256);for(unsigned i=0;i<256;++i)assert(std::abs(l[i])+std::abs(r[i])<1e-7);note(0xb0,120,0);e->dispatcher(e,1,0,0,nullptr,0);}
printf("%s max peak %.3f (%.2f dBFS), >.99: %.6f%%; finite/stereo bounds and Output=0 mute passed\n",mode?"16-note stress":"4-note chord",peak,20*log10(peak),100.*hot/samples);peak=0;hot=samples=0;}dlclose(lib);}
