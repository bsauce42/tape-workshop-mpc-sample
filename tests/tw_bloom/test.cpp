// SPDX-License-Identifier: AGPL-3.0-only
#include <cassert>
#include <cstdlib>
#include <new>
#include <iostream>
#include <chrono>
#include <vector>
static thread_local bool inAudio=false;
void* operator new(std::size_t n){assert(!inAudio);if(auto* p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
#include "../../plugins/tape-workshop-bloom/bloom.cpp"
int main(int argc,char** argv){using namespace bloom;using namespace vst2;
    auto* e=VSTPluginMain(nullptr);assert(e&&e->numInputs==2&&e->numOutputs==2&&!(e->flags&flag::isSynth));
    constexpr unsigned N=128;float l[N]{},r[N]{},ol[N],orr[N];float* input[]={l,r};float* output[]={ol,orr};
    auto run=[&](){inAudio=true;e->processReplacing(e,input,output,N);inAudio=false;double energy=0;for(unsigned n=0;n<N;++n){assert(std::isfinite(ol[n])&&std::isfinite(orr[n]));assert(std::abs(ol[n])<=1&&std::abs(orr[n])<=1);energy+=ol[n]*ol[n]+orr[n]*orr[n];}return energy;};
    for(unsigned preset=0;preset<Programs;++preset){e->dispatcher(e,op::setProgram,0,preset,nullptr,0);e->dispatcher(e,op::mainsChanged,0,0,nullptr,0);e->setParameter(e,Mix,1);l[0]=r[0]=.5f;double tail=0;run();l[0]=r[0]=0;for(unsigned block=0;block<1200;++block)tail+=run();assert(tail>.00001);}
    // Extreme controls, odd block sizes, mono input, and invalid floating point input remain bounded.
    for(unsigned n=0;n<Count;++n)e->setParameter(e,n,1);l[0]=NAN;r[0]=INFINITY;for(unsigned n=0;n<400;++n)run();l[0]=r[0]=0;
    void* ptr=nullptr;auto count=e->dispatcher(e,op::getChunk,0,0,&ptr,0);std::vector<char> saved(static_cast<char*>(ptr),static_cast<char*>(ptr)+count);e->setParameter(e,Mix,0);assert(e->dispatcher(e,op::setChunk,0,count,saved.data(),0));assert(e->getParameter(e,Mix)==1);saved[0]^=1;assert(!e->dispatcher(e,op::setChunk,0,count,saved.data(),0));
    for(unsigned n:{1u,7u,127u}){inAudio=true;e->processReplacing(e,input,output,int(n));inAudio=false;}
    for(float sr:{8000.f,44100.f,48000.f,96000.f}){assert(e->dispatcher(e,op::setSampleRate,0,0,nullptr,sr));run();}
    assert(!e->dispatcher(e,op::setSampleRate,0,0,nullptr,192000));e->dispatcher(e,op::setSampleRate,0,0,nullptr,48000);
    e->setParameter(e,Mix,NAN);assert(e->getParameter(e,Mix)==1);
    // Direct convolution of a stereo impulse gives its exact independent IR,
    // delayed by two partitions, including the final partition's overlap.
    auto fft=std::make_unique<FFT>();auto bank=std::make_unique<IRBank>();auto conv=std::make_unique<Convolution>();bank->parts=2;bank->frames=Hop+3;
    for(unsigned ch=0;ch<2;++ch)for(unsigned p=0;p<2;++p){std::array<std::complex<float>,FFTSize> b{};if(!p)b[0]=ch?.25f:1.f;else b[2]=ch?.5f:.125f;fft->run(b);std::copy_n(b.begin(),Bins,bank->spectra[ch][p].begin());}
    for(unsigned n=0;n<Hop*5;++n){inAudio=true;auto out=conv->process(n==0?1:0,n==0?1:0,*bank,*fft);inAudio=false;for(unsigned ch=0;ch<2;++ch){float expected=n==Hop*2?(ch?.25f:1.f):n==Hop*3+2?(ch?.5f:.125f):0;assert(std::abs(out[ch]-expected)<.00001f);}}
    bank->parts=MaxParts;conv->reset();for(unsigned n=0;n<MaxIR;++n)conv->process(0,0,*bank,*fft);
    double worst=0;auto irStart=std::chrono::steady_clock::now();
    for(unsigned block=0;block<375;++block){auto begin=std::chrono::steady_clock::now();inAudio=true;for(unsigned n=0;n<128;++n)conv->process(.01f,-.01f,*bank,*fft);inAudio=false;worst=std::max(worst,std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count());}
    std::cout<<"Maximum IR: one audio second / "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-irStart).count()<<" wall s; worst 128-frame block "<<worst*1000<<" ms (48 kHz budget 2.67 ms)\n";
    e->dispatcher(e,op::setProgram,0,0,nullptr,0);e->setParameter(e,Mix,0);for(unsigned n=0;n<200;++n)run();l[0]=.2f;r[0]=-.15f;run();assert(std::abs(ol[0]-.2f)<.00001&&std::abs(orr[0]+.15f)<.00001);l[0]=r[0]=0;
    auto start=std::chrono::steady_clock::now();for(unsigned n=0;n<3750;++n)run();double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();std::cout<<"Algorithmic 10-second render on this Mac: "<<seconds<<" s (not an MPC CPU measurement)\n";
    if(argc>1){std::string folder=argv[1];auto loaded=std::make_unique<IRBank>();assert(loadIR(folder+"/1-impulse.wav",48000,*loaded,*fft));assert(loaded->parts&&loaded->frames==1024);assert(!loadIR(folder+"/2-silent.wav",48000,*loaded,*fft));assert(!loadIR(folder+"/3-long.wav",48000,*loaded,*fft));assert(!loadIR(folder+"/4-bad.wav",48000,*loaded,*fft));assert(loadIR(folder+"/5-float.wav",48000,*loaded,*fft));
        auto probe=std::make_unique<Reverb>();probe->effect.object=probe.get();probe->startIR(folder);probe->params[IRSlot].store(1.f/8);
        for(unsigned n=0;n<100&&probe->ready.load()<0;++n)std::this_thread::sleep_for(std::chrono::milliseconds(10));assert(probe->ready.load()>=0);
        inAudio=true;render(&probe->effect,input,output,N);inAudio=false;assert(probe->active>=0);
        probe->params[IRSlot].store(5.f/8);for(unsigned n=0;n<100&&probe->ready.load()<0;++n)std::this_thread::sleep_for(std::chrono::milliseconds(10));assert(probe->ready.load()>=0);inAudio=true;render(&probe->effect,input,output,N);inAudio=false;
        for(unsigned n=0;n<200&&(probe->active<0||probe->bankSlots[probe->active]!=5);++n){inAudio=true;render(&probe->effect,input,output,N);inAudio=false;}
        assert(probe->active>=0&&probe->bankSlots[probe->active]==5);
    }
    // The bounded filename extension never expands the standard eight-byte VST2 display buffer.
    {
        melodee::bloom_ir::Info info;
        assert(e->dispatcher(e,melodee::bloom_ir::opcode,melodee::bloom_ir::command,sizeof(info),&info,0)==1);
        assert(info.version==1 && info.slot==0 && info.filename[0]==0);
        assert(!e->dispatcher(e,melodee::bloom_ir::opcode,melodee::bloom_ir::command,sizeof(info)-1,&info,0));
        assert(!e->dispatcher(e,melodee::bloom_ir::opcode,123,sizeof(info),&info,0));
        if(argc>1) {
            auto named=std::make_unique<Reverb>();named->effect.object=named.get();named->startIR(argv[1]);
            named->params[IRSlot].store(1.f/8);
            assert(dispatch(&named->effect,melodee::bloom_ir::opcode,melodee::bloom_ir::command,sizeof(info),&info,0)==1);
            assert(info.slot==1 && std::string(info.filename)=="1-impulse.wav");
            named->params[IRSlot].store(5.f/8);
            dispatch(&named->effect,melodee::bloom_ir::opcode,melodee::bloom_ir::command,sizeof(info),&info,0);
            assert(info.slot==5 && std::string(info.filename)=="5-float.wav");
            named->params[IRSlot].store(1);
            dispatch(&named->effect,melodee::bloom_ir::opcode,melodee::bloom_ir::command,sizeof(info),&info,0);
            assert(info.slot==8 && info.status==melodee::bloom_ir::Missing && info.filename[0]==0);
        }
    }
    std::cout<<"Bounded instance memory: "<<sizeof(Reverb)<<" bytes\n";
    e->dispatcher(e,op::close,0,0,nullptr,0);std::cout<<"Reverb tails, extremes, nonfinite input, no render allocations, state roundtrip, dry path and exact stereo IR convolution passed\n";
}
