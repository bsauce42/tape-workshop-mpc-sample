// SPDX-License-Identifier: AGPL-3.0-only
// Original Tape Workshop reverb. Host ABI: Melodee by Kerem Kiliç (Ellic Studio).
#include "melodee/plugin/vst2_abi.h"
#include "convolution.h"
#include "melodee/plugin/bloom_ir_info.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <filesystem>
#include <cstdio>
#include <new>
#include <dlfcn.h>
namespace bloom {
using namespace vst2;
static_assert(std::atomic<float>::is_always_lock_free && std::atomic<int>::is_always_lock_free);
constexpr uint32_t ID=0x54574252;
enum Param {Space,Decay,Mix,Bright,Character,Distance,Predelay,Width,EarlyLate,LowDecay,MidDecay,HighDecay,LowCut,HighCut,Tilt,Shimmer,Reverse,Freeze,IRSlot,IRBlend,Output,ModRate,Motion,Diffusion,Count};
constexpr const char* names[]={"Space","Decay","Mix","Bright","Char","Distance","PreDelay","Width","EarlyLate","LowDecay","MidDecay","HiDecay","LowCut","HighCut","Tilt","Shimmer","Reverse","Freeze","IRSlot","IRBlend","Output","ModRate","Motion","Diffuse"};
constexpr const char* presets[]={"Dark Cathedral","Infinite Bloom","Ghost Reverse","Shimmer Cloud","Metal Tunnel","Frozen Orbit","Distant Cosmos","Plate Mirage","Drum Chamber","Vocal Halo","User IR"};
constexpr unsigned Programs=sizeof(presets)/sizeof(*presets);
constexpr float defaults[]={.8f,.65f,.3f,.42f,.5f,.2f,.12f,.85f,.75f,.6f,.5f,.3f,.2f,.85f,.5f,0,0,0,0,1,.5f,.25f,.25f,.8f};
static_assert(sizeof(defaults)/sizeof(*defaults)==Count);
inline float seconds(float v){return .25f*std::pow(120.f,v);}
inline float cutLow(float v){return 20.f*std::pow(40.f,v);}
inline float cutHigh(float v){return 1000.f*std::pow(20.f,v);}
inline float coeff(double sr,float hz){return 1-std::exp(-6.28318530718f*std::min(float(sr*.4),hz)/float(sr));}
inline float safe(float v){return std::isfinite(v)?std::clamp(v,-8.f,8.f):0.f;}
inline float limit(float v){return std::abs(v)<.95f?v:std::copysign(.95f+.05f*std::tanh((std::abs(v)-.95f)*20),v);}
struct Reverb {
    Effect effect{};std::array<std::atomic<float>,Count> params;std::array<float,Count> smooth{};
    std::array<std::array<float,16384>,8> lines{};std::array<float,8> low{},high{},length{};
    std::array<std::array<float,32768>,2> pre{};std::array<std::array<float,8192>,2> grain{};
    std::array<std::array<std::array<float,2048>,2>,2> allpass{};
    std::array<float,2> hp{},lp{},toneLow{},postLow{},grainLast{};
    unsigned position=0;float phase=0,grainPhase=0,irFade=0,gain=1;double rate=48000;
    FFT fft;Convolution convolution;std::array<IRBank,2> banks;
    std::array<std::atomic<int>,2> states{};std::array<int,2> bankSlots{};std::atomic<int> ready{-1},irStatus{0},irStatusSlot{0};int active=-1;
    std::array<std::string,8> paths;std::array<std::array<char,256>,8> filenames{};unsigned irCount=0;std::atomic<double> wantedRate{48000};std::atomic<bool> quit{false};std::thread worker;
    int program=0;struct Chunk{uint32_t magic=ID,version=1;int32_t program=0;float values[Count]{};} chunk;
    Reverb(){preset(0);smooth=defaultsArray();reset();}
    ~Reverb(){quit.store(true);if(worker.joinable())worker.join();}
    static std::array<float,Count> defaultsArray(){std::array<float,Count> a;std::copy_n(defaults,Count,a.begin());return a;}
    void preset(int n){program=std::clamp(n,0,int(Programs-1));auto p=defaultsArray();switch(program){
        case 1:p[Decay]=1;p[Mix]=.48f;p[Motion]=.7f;p[Bright]=.6f;p[Shimmer]=.3f;break;
        case 2:p[Reverse]=.85f;p[Decay]=.8f;p[Predelay]=.3f;p[Mix]=.5f;break;
        case 3:p[Shimmer]=.8f;p[Decay]=.9f;p[Mix]=.45f;p[Bright]=.75f;break;
        case 4:p[Space]=.35f;p[Diffusion]=.2f;p[Character]=.95f;p[Decay]=.85f;p[HighDecay]=.75f;break;
        case 5:p[Decay]=.95f;p[Motion]=1;p[ModRate]=.8f;p[Reverse]=.4f;p[Mix]=.5f;break;
        case 6:p[Distance]=.95f;p[Bright]=.15f;p[Decay]=.95f;p[Predelay]=.5f;p[Mix]=.5f;break;
        case 7:p[Space]=.4f;p[Decay]=.5f;p[Diffusion]=1;p[Bright]=.8f;p[LowDecay]=.25f;break;
        case 8:p[Space]=.2f;p[Decay]=.25f;p[Mix]=.2f;p[EarlyLate]=.25f;break;
        case 9:p[Decay]=.5f;p[Predelay]=.15f;p[LowCut]=.5f;p[HighCut]=.65f;p[Mix]=.2f;break;
        case 10:p[IRSlot]=1.f/8;p[IRBlend]=.65f;break;
    }for(unsigned i=0;i<Count;++i)params[i].store(p[i],std::memory_order_relaxed);}
    void reset(){for(auto& l:lines)l.fill(0);for(auto& l:pre)l.fill(0);for(auto& l:grain)l.fill(0);for(auto& ch:allpass)for(auto& l:ch)l.fill(0);low.fill(0);high.fill(0);hp.fill(0);lp.fill(0);toneLow.fill(0);postLow.fill(0);grainLast.fill(0);position=0;phase=grainPhase=irFade=0;convolution.reset();for(unsigned i=0;i<8;++i)length[i]=float(rate)*(.029f+i*.007f);}
    void startIR(const std::string& folder){
        std::error_code ec;std::vector<std::string> files;
        for(std::filesystem::directory_iterator it(folder,ec),end;!ec&&it!=end;it.increment(ec)){if(files.size()>=128)break;if(it->is_regular_file(ec)&&(it->path().extension()==".wav"||it->path().extension()==".WAV"))files.push_back(it->path().string());}
        std::sort(files.begin(),files.end());irCount=std::min(8u,unsigned(files.size()));for(unsigned i=0;i<irCount;++i){paths[i]=files[i];std::snprintf(filenames[i].data(),filenames[i].size(),"%s",std::filesystem::path(files[i]).filename().string().c_str());}
        worker=std::thread([this]{int last=-1;double lastRate=0;while(!quit.load(std::memory_order_relaxed)){
            int slot=int(std::lround(params[IRSlot].load()*8));double sr=wantedRate.load();
            if(slot==0){irStatusSlot.store(0);irStatus.store(0);last=0;lastRate=sr;}
            else if(slot!=last||sr!=lastRate){int free=-1;for(int b=0;b<2;++b){int expected=0;if(states[b].compare_exchange_strong(expected,1)){free=b;break;}}
                if(free>=0){irStatusSlot.store(slot);irStatus.store(1);bool ok=unsigned(slot)<=irCount&&loadIR(paths[slot-1],sr,banks[free],fft);
                    if(ok){bankSlots[free]=slot;states[free].store(2);ready.store(free,std::memory_order_release);irStatus.store(2);}else{states[free].store(0);irStatus.store(3);}last=slot;lastRate=sr;}}
            std::this_thread::sleep_for(std::chrono::milliseconds(25));}});
    }
    float read(const std::array<float,16384>& line,float delay){float at=float(position&16383)-std::clamp(delay,2.f,16380.f);int n=int(std::floor(at));float f=at-n;return line[unsigned(n)&16383]*(1-f)+line[unsigned(n+1)&16383]*f;}
    float grainRead(unsigned ch,float delay){float at=float(position&8191)-delay;int n=int(std::floor(at));float f=at-n;return grain[ch][unsigned(n)&8191]*(1-f)+grain[ch][unsigned(n+1)&8191]*f;}
};
Reverb& self(Effect* e){return *static_cast<Reverb*>(e->object);}
float get(Effect* e,int32_t n){return n>=0&&n<Count?self(e).params[n].load(std::memory_order_relaxed):0;}
void set(Effect* e,int32_t n,float v){if(n>=0&&n<Count&&std::isfinite(v))self(e).params[n].store((n==Freeze?(v>=.5f?1.f:0.f):std::clamp(v,0.f,1.f)),std::memory_order_relaxed);}
void render(Effect* e,float** inputs,float** outputs,int32_t frames){
    if(frames<=0||!outputs||!outputs[0]||!outputs[1])return;auto& s=self(e);
    int bank=s.ready.load(std::memory_order_acquire);if(bank>=0&&(s.active<0||s.irFade<.001f)){bank=s.ready.exchange(-1,std::memory_order_acquire);}else bank=-1;if(bank>=0){int old=s.active;s.active=bank;s.states[bank].store(3);s.convolution.reset();s.irFade=0;if(old>=0)s.states[old].store(0,std::memory_order_release);}
    std::array<float,Count> target;for(unsigned i=0;i<Count;++i)target[i]=s.params[i].load(std::memory_order_relaxed);
    const int slot=int(std::lround(target[IRSlot]*8));const bool ir=s.active>=0&&std::abs(s.banks[s.active].rate-s.rate)<1;const bool wanted=ir&&slot>0&&slot==s.bankSlots[s.active];const bool irRun=ir&&(wanted||s.irFade>.001f);
    float k=coeff(s.rate,8),lowK=coeff(s.rate,250),highK=coeff(s.rate,3500),hpK=coeff(s.rate,cutLow(target[LowCut])),lpK=coeff(s.rate,cutHigh(target[HighCut]));
    float brightness=coeff(s.rate,(1000+17000*target[Bright])*(1-.65f*target[Distance])),postK=coeff(s.rate,900),tiltLow=std::pow(10.f,(.5f-target[Tilt])*12/20),tiltHigh=1/tiltLow,outGain=std::pow(10.f,(-12+24*target[Output])/20);
    float feedback[8][3];const float bases[]={1423,1699,1889,2141,2549,2857,3209,3581};float times[3]={seconds(target[Decay])*(.25f+1.75f*target[LowDecay]),seconds(target[Decay])*(.25f+1.75f*target[MidDecay]),seconds(target[Decay])*(.25f+1.75f*target[HighDecay])};
    for(unsigned n=0;n<8;++n)for(unsigned band=0;band<3;++band)feedback[n][band]=std::min(.99998f,std::exp(-6.907755f*(bases[n]/48000.f)*(.45f+1.2f*target[Space])/times[band]));
    for(int i=0;i<frames;++i){for(unsigned p=0;p<Count;++p)s.smooth[p]+=k*(target[p]-s.smooth[p]);const auto& p=s.smooth;s.gain+=k*(outGain-s.gain);if(std::abs(outGain-s.gain)<.0001f)s.gain=outGain;
        float dry[2]={safe(inputs&&inputs[0]?inputs[0][i]:0),safe(inputs&&inputs[1]?inputs[1][i]:0)},input[2],early[2];
        for(unsigned ch=0;ch<2;++ch){s.pre[ch][s.position&32767]=dry[ch];float delay=p[Predelay]*.25f*float(s.rate);unsigned a=unsigned(delay);float f=delay-a;
            input[ch]=s.pre[ch][(s.position-a)&32767]*(1-f)+s.pre[ch][(s.position-a-1)&32767]*f;
            early[ch]=0;for(unsigned tap=0;tap<4;++tap)early[ch]+=s.pre[ch][(s.position-a-unsigned(s.rate*(.009+.011*tap)*(1+.8*p[Space]))-ch*37)&32767]*(tap%2?-.12f:.18f);
            for(unsigned stage=0;stage<2;++stage){unsigned len=stage?313:97,at=s.position%len;float delayed=s.allpass[ch][stage][at],g=.2f+.5f*p[Diffusion];float y=delayed-g*input[ch];s.allpass[ch][stage][at]=input[ch]+g*y;input[ch]=y;}}
        auto irWet=irRun?s.convolution.process(input[0],input[1],s.banks[s.active],s.fft):std::array<float,2>{0,0};
        float delayed[8],sum=0;for(unsigned n=0;n<8;++n){float desired=bases[n]*float(s.rate/48000)*(.45f+1.2f*p[Space]);s.length[n]+=.0002f*(desired-s.length[n]);float ph=s.phase+n*.123f;ph-=std::floor(ph);float motion=(1-4*std::abs(ph-.5f))*p[Motion]*(1+6*p[Character]);float x=s.read(s.lines[n],s.length[n]+motion);s.low[n]+=lowK*(x-s.low[n]);s.high[n]+=highK*(x-s.high[n]);float freeze=p[Freeze];delayed[n]=(s.low[n]*feedback[n][0]+(s.high[n]-s.low[n])*feedback[n][1]+(x-s.high[n])*feedback[n][2])*(1-freeze)+x*.99998f*freeze;delayed[n]*=1-.02f*p[Shimmer]*(1-p[Freeze]);sum+=delayed[n];}
        float wet[2]={0,0};for(unsigned n=0;n<8;++n){float inject=(input[0]+(n%2?input[1]:-input[1]))*.15f*(1-p[Freeze]);inject+=s.grainLast[n%2]*p[Shimmer]*.008f*(1-p[Freeze]);float next=delayed[n]-.25f*sum+inject;if(std::abs(next)<1e-20f)next=0;s.lines[n][s.position&16383]=std::clamp(next,-8.f,8.f);wet[n%2]+=delayed[n]*.3f;}
        s.irFade+=k*((wanted?1.f:0.f)-s.irFade);float ph=s.grainPhase,ph2=std::fmod(ph+.5f,1.f),win=.5f-.5f*std::cos(6.28318530718f*ph);
        for(unsigned ch=0;ch<2;++ch){s.grain[ch][s.position&8191]=wet[ch];float shifted=s.grainRead(ch,512+2048*(1-ph))*win+s.grainRead(ch,512+2048*(1-ph2))*(1-win);s.grainLast[ch]=shifted;
            float rev=s.grainRead(ch,512+4096*ph)*win+s.grainRead(ch,512+4096*ph2)*(1-win);wet[ch]=(wet[ch]+shifted*p[Shimmer]*.7f)*(1-p[Reverse])+rev*p[Reverse];
            wet[ch]=wet[ch]*p[EarlyLate]+early[ch]*(1-p[EarlyLate]);float blend=ir?p[IRBlend]*s.irFade:0;wet[ch]=wet[ch]*(1-blend)+irWet[ch]*blend;
            s.hp[ch]+=hpK*(wet[ch]-s.hp[ch]);wet[ch]-=s.hp[ch];s.lp[ch]+=lpK*(wet[ch]-s.lp[ch]);wet[ch]=s.lp[ch];s.toneLow[ch]+=brightness*(wet[ch]-s.toneLow[ch]);wet[ch]=s.toneLow[ch];s.postLow[ch]+=postK*(wet[ch]-s.postLow[ch]);wet[ch]=s.postLow[ch]*tiltLow+(wet[ch]-s.postLow[ch])*tiltHigh;}
        float mid=(wet[0]+wet[1])*.5f,side=(wet[0]-wet[1])*.5f*(p[Width]*2);float dist=1-.65f*p[Distance];
        outputs[0][i]=limit((dry[0]*(1-p[Mix])+(mid+side)*p[Mix]*dist)*s.gain);outputs[1][i]=limit((dry[1]*(1-p[Mix])+(mid-side)*p[Mix]*dist)*s.gain);
        s.phase+=(.05f+1.95f*p[ModRate])/float(s.rate);if(s.phase>=1)s.phase-=1;s.grainPhase+=1.f/2048;if(s.grainPhase>=1)s.grainPhase-=1;++s.position;
    }
}
intptr_t dispatch(Effect* e,int32_t opCode,int32_t index,intptr_t value,void* ptr,float opt){auto& s=self(e);switch(opCode){
    case op::open:return 1;case op::close:delete &s;return 1;
    case op::setSampleRate:if(std::isfinite(opt)&&opt>=8000&&opt<=96000){s.rate=opt;s.wantedRate.store(opt);s.reset();return 1;}return 0;
    case op::mainsChanged:if(!value)s.reset();return 1;
    case op::setProgram:s.preset(int(value));return 1;case op::getProgram:return s.program;
    case op::getProgramName:if(ptr)std::snprintf(static_cast<char*>(ptr),24,"%s",presets[s.program]);return 1;
    case op::getProgramNameIndexed:if(!ptr||index<0||unsigned(index)>=Programs)return 0;std::snprintf(static_cast<char*>(ptr),24,"%s",presets[index]);return 1;
    case melodee::bloom_ir::opcode:{
        using namespace melodee::bloom_ir;
        if(index!=command||value!=sizeof(Info)||!ptr)return 0;
        Info info;info.slot=int(std::lround(get(e,IRSlot)*8));
        if(!info.slot)info.status=Algorithmic;
        else if(unsigned(info.slot)>s.irCount)info.status=Missing;
        else {std::copy(s.filenames[info.slot-1].begin(),s.filenames[info.slot-1].end(),info.filename);info.status=s.irStatusSlot.load()==info.slot?s.irStatus.load():Loading;}
        std::memcpy(ptr,&info,sizeof(info));return 1;
    }
    case op::getParamName:if(ptr&&index>=0&&index<Count)std::snprintf(static_cast<char*>(ptr),9,"%s",names[index]);return 1;
    case op::getParamLabel:if(ptr)*static_cast<char*>(ptr)=0;return 1;
    case op::getParamDisplay:if(ptr&&index>=0&&index<Count){auto* t=static_cast<char*>(ptr);float v=get(e,index);if(index==Decay)std::snprintf(t,8,"%.1fs",seconds(v));else if(index==Predelay)std::snprintf(t,8,"%.0fms",v*250);else if(index==Width)std::snprintf(t,8,"%.0f%%",v*200);else if(index==LowDecay||index==MidDecay||index==HighDecay)std::snprintf(t,8,"%.0f%%",25+175*v);else if(index==Output||index==Tilt)std::snprintf(t,8,"%+.1fdB",(v-.5f)*(index==Output?24:12));else if(index==ModRate)std::snprintf(t,8,"%.2fHz",.05f+1.95f*v);else if(index==Freeze)std::snprintf(t,8,"%s",v>.5f?"HOLD":"Off");else if(index==IRSlot){int slot=int(std::lround(v*8));std::snprintf(t,8,"%s",!slot?"Algo":unsigned(slot)>s.irCount?"No IR":s.irStatus.load()==1?"Load IR":s.irStatus.load()==3?"Bad IR":"IR Ready");}else if(index==LowCut||index==HighCut)std::snprintf(t,8,"%.0fHz",index==LowCut?cutLow(v):cutHigh(v));else std::snprintf(t,8,"%.0f%%",v*100);}return 1;
    case op::getChunk:if(!ptr)return 0;s.chunk.program=s.program;for(unsigned n=0;n<Count;++n)s.chunk.values[n]=get(e,n);*static_cast<void**>(ptr)=&s.chunk;return sizeof(s.chunk);
    case op::setChunk:{if(!ptr||value!=sizeof(s.chunk))return 0;Reverb::Chunk c;std::memcpy(&c,ptr,sizeof(c));if(c.magic!=ID||c.version!=1||c.program<0||unsigned(c.program)>=Programs)return 0;for(float v:c.values)if(!std::isfinite(v)||v<0||v>1)return 0;for(unsigned n=0;n<Count;++n)set(e,n,c.values[n]);s.program=c.program;return 1;}
    case op::getEffectName:case op::getProductString:if(ptr)std::snprintf(static_cast<char*>(ptr),32,"Bloom Reverb");return 1;
    case op::getVendorString:if(ptr)std::snprintf(static_cast<char*>(ptr),32,"Tape Workshop");return 1;
    case op::getVendorVersion:return 101;case op::getVstVersion:return 2400;case op::getTailSize:return int(s.rate*60);default:return 0;
}}
}
extern "C" __attribute__((visibility("default"))) vst2::Effect* VSTPluginMain(vst2::HostCallback){
    auto* s=new(std::nothrow) bloom::Reverb;if(!s)return nullptr;auto& e=s->effect;e.magic=vst2::magic;e.dispatcher=bloom::dispatch;e.setParameter=bloom::set;e.getParameter=bloom::get;e.numPrograms=bloom::Programs;e.numParams=bloom::Count;e.numInputs=e.numOutputs=2;e.flags=vst2::flag::canReplace|vst2::flag::programChunks;e.uniqueId=int32_t(bloom::ID);e.version=101;e.processReplacing=bloom::render;e.object=s;
    try{Dl_info info{};if(dladdr(reinterpret_cast<void*>(&VSTPluginMain),&info)&&info.dli_fname)s->startIR((std::filesystem::path(info.dli_fname).parent_path()/"IRs").string());}catch(...){delete s;return nullptr;}return &e;
}
