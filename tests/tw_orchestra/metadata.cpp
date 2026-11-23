// SPDX-License-Identifier: AGPL-3.0-only
// Metadata/state validation only: no render callback or audio playback.
#include "melodee/plugin/vst2_abi.h"
#include "melodee/plugin/orchestra_info.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <dlfcn.h>
#include <cstdio>
#include <set>
#include <string>
#include <vector>
int main(int argc,char**argv){
    assert(argc==2);auto* library=dlopen(argv[1],RTLD_NOW);assert(library);
    auto entry=reinterpret_cast<vst2::EntryPoint>(dlsym(library,"VSTPluginMain"));assert(entry);
    auto* effect=entry(nullptr);assert(effect&&effect->uniqueId==0x54574f31&&effect->numPrograms==32&&effect->numParams==48&&effect->numInputs==0&&effect->numOutputs==2);
    std::set<std::string> names;std::set<std::vector<float>> sounds;
    for(int program=0;program<32;++program){
        effect->dispatcher(effect,vst2::op::setProgram,0,program,nullptr,0);
        char name[64]{};assert(effect->dispatcher(effect,vst2::op::getProgramNameIndexed,program,0,name,0));assert(*name&&names.insert(name).second);
        std::vector<float> values;
        for(int p=0;p<48;++p){float v=effect->getParameter(effect,p);assert(std::isfinite(v)&&v>=0&&v<=1);values.push_back(v);}
        assert(sounds.insert(values).second);
        assert(values[35]==0&&values[47]==1); // generator off, project-follow on in every factory preset
        for(unsigned family=0;family<4;++family){auto id=melodee::orchestra_player(family,values[30+family]);assert(id>=melodee::orchestra_first[family]&&id<melodee::orchestra_first[family]+melodee::orchestra_count[family]);}
        void* data=nullptr;auto size=effect->dispatcher(effect,vst2::op::getChunk,0,0,&data,0);assert(size==204);
        std::vector<char> copy(static_cast<char*>(data),static_cast<char*>(data)+size);
        effect->setParameter(effect,34,.123f);assert(effect->dispatcher(effect,vst2::op::setChunk,0,size,copy.data(),0));assert(effect->getParameter(effect,34)==values[34]);
        std::vector<char> legacy(copy.begin(),copy.begin()+152);uint32_t oldVersion=1;std::memcpy(legacy.data()+4,&oldVersion,4);
        effect->setParameter(effect,35,1);assert(effect->dispatcher(effect,vst2::op::setChunk,0,152,legacy.data(),0));
        assert(effect->getParameter(effect,35)==0&&effect->getParameter(effect,47)==1);
        copy[0]^=1;assert(!effect->dispatcher(effect,vst2::op::setChunk,0,size,copy.data(),0));
    }
    effect->setParameter(effect,34,NAN);assert(std::isfinite(effect->getParameter(effect,34)));
    effect->dispatcher(effect,vst2::op::close,0,0,nullptr,0);dlclose(library);
    std::puts("32 distinct presets, 17 player mappings, bounded parameters and state roundtrips validated; no audio rendered.");
}
