// SPDX-License-Identifier: AGPL-3.0-only
#include <cstdlib>
#include <new>
#include <cassert>
thread_local bool forbidAllocation=false;
void* operator new(std::size_t n){assert(!forbidAllocation);if(auto*p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void*p) noexcept {std::free(p);}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete[](void*p) noexcept {std::free(p);}
#include "../../plugins/tape-workshop-prism/prism.cpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <vector>
using namespace prism;
void midi(Effect* e,int status,int key,int velocity,int at=0){MidiEvent m{};m.type=midiEventType;m.byteSize=sizeof m;m.deltaFrames=at;m.data[0]=char(status);m.data[1]=char(key);m.data[2]=char(velocity);Events<1> list{};list.count=1;list.events[0]=&m;e->dispatcher(e,op::processEvents,0,0,&list,0);}
int main(){
 auto*e=VSTPluginMain(nullptr);assert(e&&e->numParams==Count&&Count==180);float left[128],right[128];float*out[]={left,right};
 auto block=[&]{forbidAllocation=true;e->processReplacing(e,nullptr,out,128);forbidAllocation=false;double energy=0;for(int n=0;n<128;++n){assert(std::isfinite(left[n])&&std::isfinite(right[n]));assert(std::abs(left[n])<=1&&std::abs(right[n])<=1);energy+=left[n]*left[n]+right[n]*right[n];}return energy;};
 for(int n=0;n<Count;++n){char name[8]{};e->dispatcher(e,op::getParamName,n,0,name,0);assert(name[0]&&name[7]==0);}
 for(int program=0;program<16;++program){e->dispatcher(e,op::setProgram,0,program,nullptr,0);e->dispatcher(e,op::mainsChanged,0,0,nullptr,0);midi(e,0x90,60,100);double energy=0;for(int n=0;n<150;++n)energy+=block();assert(energy>.001);midi(e,0x80,60,0);for(int n=0;n<3100;++n)block();for(auto&v:self(e).voices)assert(!v.active);}
 e->dispatcher(e,op::setProgram,0,15,nullptr,0);midi(e,0x90,60,100,200);assert(block()<1e-8);assert(block()>.00001);
 midi(e,0xb0,64,127);midi(e,0x80,60,0);block();assert(self(e).voices[0].env[0].stage!=3);midi(e,0xb0,64,0);block();assert(self(e).voices[0].env[0].stage==3);
 set(e,Polyphony,1);for(int n=0;n<20;++n)midi(e,0x90,48+n,100);block();int active=0;for(auto&v:self(e).voices)active+=v.active;assert(active==8);
 set(e,editBase,1);set(e,editBase+1,1);set(e,editBase+2,.37f);assert(std::abs(self(e).partials[5][127].load()-.37f)<1e-6);set(e,editBase+3,.4f);set(e,editBase+4,1);set(e,editBase+5,.81f);
 void*data=nullptr;auto size=e->dispatcher(e,op::getChunk,0,0,&data,0);assert(size==sizeof(Chunk));std::vector<char>saved(static_cast<char*>(data),static_cast<char*>(data)+size);e->dispatcher(e,op::setProgram,0,0,nullptr,0);assert(e->dispatcher(e,op::setChunk,0,size,saved.data(),0)==1);assert(std::abs(get(e,editBase+2)-.37f)<1e-6);assert(std::abs(get(e,editBase+5)-.81f)<1e-6);
 // Last operator and both matrix types must actually change the carrier sound.
 auto sound=[&](bool fm,bool rm){
  e->dispatcher(e,op::setProgram,0,15,nullptr,0);
  set(e,opBase+50+Level,1);set(e,opBase+50+Ratio,ratioValue(2.3f));
  set(e,fmBase+5,fm?.8f:0);set(e,rmBase+5,rm?.8f:0);
  for(int n=0;n<100;++n)block();e->dispatcher(e,op::mainsChanged,0,0,nullptr,0);midi(e,0x90,60,100);
  std::array<float,512> result{};for(int n=0;n<4;++n){block();std::copy(left,left+128,result.begin()+n*128);}return result;
 };
 auto dry=sound(false,false),fm=sound(true,false),rm=sound(false,true);double fmDiff=0,rmDiff=0;for(int n=0;n<512;++n){fmDiff+=std::abs(fm[n]-dry[n]);rmDiff+=std::abs(rm[n]-dry[n]);}assert(fmDiff>1&&rmDiff>1);
 melodee::prism_ui::Info info;assert(e->dispatcher(e,melodee::prism_ui::opcode,melodee::prism_ui::command,sizeof info,&info,0)==1);assert(info.version==1);assert(!e->dispatcher(e,melodee::prism_ui::opcode,melodee::prism_ui::command,1,&info,0));
 auto bad=saved;reinterpret_cast<Chunk*>(bad.data())->params[0]=NAN;assert(!e->dispatcher(e,op::setChunk,0,size,bad.data(),0));assert(!e->dispatcher(e,op::setChunk,0,3,saved.data(),0));set(e,0,NAN);assert(std::isfinite(get(e,0)));
 midi(e,0xb0,120,0);block();for(auto&v:self(e).voices)assert(!v.active);
 // Dense FM/RM, resonance extremes and repeated parameter edits must stay finite.
 for(float rate:{44100.f,48000.f,96000.f}){e->dispatcher(e,op::setSampleRate,0,0,nullptr,rate);for(int n=0;n<Count;++n)if(n<editBase)set(e,n,.99f);for(int o=0;o<6;++o)set(e,opBase+o*10+Route,float(1+o%3)/4);for(int n=0;n<8;++n)midi(e,0x90,48+3*n,110);for(int n=0;n<150;++n)block();}
 e->dispatcher(e,op::setSampleRate,0,0,nullptr,48000);e->dispatcher(e,op::setProgram,0,5,nullptr,0);for(int n=0;n<8;++n)midi(e,0x90,48+3*n,110);
 auto start=std::chrono::steady_clock::now();for(int n=0;n<750;++n)block();double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();std::cout<<"Prism FM: "<<Count<<" parameters, "<<sizeof(Synth)<<" bytes; desktop light factory-patch 2s render: "<<seconds<<" s\n";
 e->dispatcher(e,op::close,0,0,nullptr,0);
}
