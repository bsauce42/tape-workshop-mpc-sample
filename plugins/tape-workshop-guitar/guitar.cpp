// SPDX-License-Identifier: AGPL-3.0-only
// Original Tape Workshop string model. ABI: Melodee by Kerem Kiliç (Ellic Studio).
#include "melodee/plugin/vst2_abi.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
using namespace vst2;
namespace {
constexpr uint32_t id=0x54574731;
constexpr float pi=3.14159265358979323846f;
enum Param { Material, Pick, Bright, Decay, Mute, Body, Attack, Release, Velocity,
    Strum, Direction, Detune, Width, Vibrato, VibRate, Room, Echo, Output, Count };
constexpr const char* names[]={"String","PickPos","Bright","Decay","Mute","Body","Attack","Release","VelSens","Strum","StrmDir","Detune","Width","Vibrato","VibRate","Room","Echo","Output"};
constexpr float defaults[]={1,.23f,.65f,.55f,0,.65f,0,.2f,.8f,0,0,.1f,.55f,0,.35f,.15f,0,.65f};
constexpr const char* presets[]={"Steel Fingerstyle","Nylon Warm","Steel Picked","Palm Muted","Wide Strum","Dark Nylon","Bright Harmonics","Ambient Guitar"};
constexpr int programCount=sizeof(presets)/sizeof(*presets);
struct Voice {
    std::array<float,8192> line{};
    unsigned write=0, age=0, wait=0; int note=0,channel=0;
    bool active=false,released=false,sustained=false;
    float env=0,velocity=0,previous=0,damp=0,pan=0,dc=0,dcIn=0;
    double basePeriod=100,phase=0;
};
struct Instrument {
    Effect effect{}; std::array<std::atomic<float>,Count> params;
    std::array<Voice,12> voices{}; std::array<MidiEvent,256> events{}; unsigned count=0;
    std::array<float,16> bend{}, expression{}; std::array<bool,16> pedal{};
    std::array<std::array<float,32768>,2> room{},echo{};
    std::array<float,2> bodyLow{},bodyBand{},roomTone{};
    unsigned position=0; uint32_t random=0x74574731; double rate=44100; int program=0;
    struct Chunk {uint32_t magic=id,version=1;int32_t program=0;float params[Count]{};} chunk;
    Instrument(){expression.fill(1);preset(0);}
    float noise(){random^=random<<13;random^=random>>17;random^=random<<5;return float(random>>8)*(1.f/8388608.f)-1.f;}
    void preset(int n){program=std::clamp(n,0,programCount-1);float p[Count];std::copy(std::begin(defaults),std::end(defaults),p);
        switch(program){
        case 1:p[Material]=0;p[Bright]=.35f;p[Pick]=.32f;p[Decay]=.5f;break;
        case 2:p[Bright]=.9f;p[Pick]=.13f;p[Velocity]=.95f;break;
        case 3:p[Mute]=.8f;p[Decay]=.2f;p[Bright]=.45f;p[Room]=.04f;break;
        case 4:p[Strum]=.28f;p[Width]=.9f;p[Detune]=.2f;break;
        case 5:p[Material]=0;p[Bright]=.2f;p[Pick]=.4f;p[Body]=.9f;break;
        case 6:p[Pick]=.5f;p[Bright]=1;p[Body]=.25f;break;
        case 7:p[Attack]=.12f;p[Decay]=.9f;p[Release]=.7f;p[Room]=.65f;p[Echo]=.35f;break;
        }for(int i=0;i<Count;++i)params[i].store(p[i],std::memory_order_relaxed);
    }
    void reset(){for(auto& v:voices)v.active=false;count=0;pedal.fill(false);bend.fill(0);expression.fill(1);for(auto& x:room)x.fill(0);for(auto& x:echo)x.fill(0);bodyLow.fill(0);bodyBand.fill(0);roomTone.fill(0);position=0;}
    void start(int note,int channel,int velocity,unsigned strumRank,const float* p){
        Voice* v=nullptr;for(auto& x:voices)if(!x.active){v=&x;break;}
        if(!v){v=&voices[0];for(auto& x:voices)if(x.env<v->env||(x.env==v->env&&x.age>v->age))v=&x;}
        v->active=true;v->released=false;v->sustained=false;v->note=note;v->channel=channel;
        v->age=0;v->env=0;v->write=0;v->previous=v->damp=v->dc=v->dcIn=0;v->phase=0;
        v->velocity=(1-p[Velocity])+p[Velocity]*velocity/127.f;
        float cents=noise()*p[Detune]*8;
        v->basePeriod=rate/(440*std::pow(2.,(note-69+cents/100.)/12.));
        v->pan=(note%6-2.5f)/3.f;
        v->wait=unsigned(strumRank*p[Strum]*.06*rate);
        // Excite one period with a pick-position comb; no files or heap work.
        unsigned length=std::clamp(unsigned(v->basePeriod),8u,8190u);
        unsigned offset=std::max(1u,unsigned(length*(.05f+.45f*p[Pick])));
        float previous=0;double mean=0;
        for(unsigned n=0;n<length;++n){float x=noise();previous+=(.15f+.8f*p[Bright])*(x-previous);v->line[n]=previous;mean+=previous;}
        float avg=float(mean/length);
        for(unsigned n=0;n<length;++n)v->line[n]-=avg;
        // Use separate fixed stack scratch so comb does not recursively alter its source.
        std::array<float,8192> scratch{};
        for(unsigned n=0;n<length;++n)scratch[n]=v->line[n]-v->line[(n+length-offset)%length];
        v->line.fill(0);
        for(unsigned n=0;n<length;++n)v->line[(8192-length+n)&8191]=scratch[n]*.55f;
    }
    void event(const MidiEvent& e,const float* p,unsigned rank){unsigned status=uint8_t(e.data[0]),ch=status&15,a=uint8_t(e.data[1])&127,b=uint8_t(e.data[2])&127;
        switch(status&240){
        case 144:if(b){start(a,ch,b,rank,p);break;}[[fallthrough]];
        case 128:for(auto& v:voices)if(v.active&&v.note==int(a)&&v.channel==int(ch)){if(pedal[ch])v.sustained=true;else v.released=true;}break;
        case 224:bend[ch]=(int(a+128*b)-8192)/8192.f;break;
        case 176:if(a==11)expression[ch]=b/127.f;
            if(a==64){pedal[ch]=b>=64;if(!pedal[ch])for(auto& v:voices)if(v.channel==int(ch)&&v.sustained){v.released=true;v.sustained=false;}}
            if(a==120||a==123){for(auto& v:voices)if(v.channel==int(ch)){if(a==120)v.active=false;else v.released=true;}pedal[ch]=false;}
            if(a==121){bend[ch]=0;expression[ch]=1;pedal[ch]=false;for(auto& v:voices)if(v.channel==int(ch)&&v.sustained){v.released=true;v.sustained=false;}}break;
        }
    }
};
Instrument& self(Effect* e){return *static_cast<Instrument*>(e->object);}
float get(Effect* e,int32_t i){return i>=0&&i<Count?self(e).params[i].load(std::memory_order_relaxed):0;}
void set(Effect* e,int32_t i,float v){if(i>=0&&i<Count&&std::isfinite(v))self(e).params[i].store(std::clamp(v,0.f,1.f),std::memory_order_relaxed);}
void render(Effect* e,float**,float** out,int32_t frames){auto& s=self(e);float p[Count];for(int n=0;n<Count;++n)p[n]=get(e,n);
    // Coefficients prepared once per block, bounded by negotiated sample rate.
    float release=std::exp(-1.f/float(s.rate*(.015+1.5*p[Release])));
    float attack=1.f/float(s.rate*(.0005+.15*p[Attack]));
    float bodyG=std::tan(pi*float(110+150*p[Body])/float(s.rate));
    unsigned event=0;
    std::array<float,12> period{},feedback{};
    auto coefficients=[&]{for(unsigned n=0;n<s.voices.size();++n){auto& v=s.voices[n];
        double wanted=v.basePeriod/std::pow(2.,s.bend[v.channel]*2./12.);
        // Compensate the averaging and brightness filters' phase delay to keep strings in tune.
        double omega=2*pi/std::max(8.,wanted),damping=.18+.79*p[Bright];
        if(p[Material]<.5f)damping*=.8;
        double phase=std::atan2((1-damping)*std::sin(omega),1-(1-damping)*std::cos(omega));
        period[n]=float(std::clamp(wanted-.5-phase/omega,8.,8190.));
        double seconds=(.3+7*p[Decay])*(1-.94*p[Mute]);
        feedback[n]=float(std::pow(.001,period[n]/(s.rate*seconds)));}};
    coefficients();
    for(int i=0;i<frames;++i){bool changed=false;
        while(event<s.count&&s.events[event].deltaFrames<=i){unsigned rank=0;const auto& m=s.events[event];
            if((uint8_t(m.data[0])&240)==144&&uint8_t(m.data[2]))for(unsigned n=0;n<s.count;++n){auto& other=s.events[n];if(other.deltaFrames==m.deltaFrames&&(uint8_t(other.data[0])&255)==(uint8_t(m.data[0])&255)&&uint8_t(other.data[2])&&((p[Direction]<.5f&&uint8_t(other.data[1])<uint8_t(m.data[1]))||(p[Direction]>=.5f&&uint8_t(other.data[1])>uint8_t(m.data[1]))))++rank;}
            s.event(m,p,std::min(rank,11u));++event;changed=true;
        }if(changed)coefficients();
        float sum[2]{};
        for(unsigned n=0;n<s.voices.size();++n){auto& v=s.voices[n];if(!v.active)continue;
            if(v.wait){--v.wait;if(v.released)v.active=false;continue;}
            if(v.released)v.env*=release;else v.env=std::min(1.f,v.env+attack);
            ++v.age;if((v.released&&v.env<.0001f)||v.age>s.rate*40){v.active=false;continue;}
            v.phase+=(.3+7*p[VibRate])/s.rate;if(v.phase>=1)v.phase-=1;
            float delay=std::clamp(period[n]*(1+std::sin(float(v.phase*2*pi))*p[Vibrato]*.003f),8.f,8190.f);
            float read=float(v.write)-delay;int whole=int(std::floor(read));float frac=read-whole;
            float x=v.line[unsigned(whole)&8191]*(1-frac)+v.line[unsigned(whole+1)&8191]*frac;
            float averaged=.5f*(x+v.previous);v.previous=x;
            float damping=.18f+.79f*p[Bright];if(p[Material]<.5f)damping*=.8f;
            v.damp+=damping*(averaged-v.damp);
            v.line[v.write]=v.damp*feedback[n];v.write=(v.write+1)&8191;
            float clean=x-v.dcIn+.995f*v.dc;v.dcIn=x;v.dc=clean;
            float amp=clean*v.env*v.velocity*s.expression[v.channel]*.35f;
            sum[0]+=amp*(1-v.pan*p[Width]*.6f);sum[1]+=amp*(1+v.pan*p[Width]*.6f);
        }
        for(unsigned ch=0;ch<2;++ch){float g=bodyG,damp=1.1f,a=1/(1+g*(g+damp));
            float band=a*(s.bodyBand[ch]+g*(sum[ch]-s.bodyLow[ch]));float low=s.bodyLow[ch]+g*band;
            s.bodyBand[ch]=2*band-s.bodyBand[ch];s.bodyLow[ch]=2*low-s.bodyLow[ch];
            float dry=sum[ch]+band*p[Body]*.8f;
            unsigned at=s.position&32767;unsigned d1=unsigned(s.rate*(ch?.041:.037)),d2=unsigned(s.rate*(ch?.067:.061));
            float wet=.5f*(s.room[ch][(s.position-d1)&32767]+s.room[ch][(s.position-d2)&32767]);
            s.roomTone[ch]+=.35f*(wet-s.roomTone[ch]);s.room[ch][at]=dry+s.roomTone[ch]*.7f;
            unsigned d=unsigned(s.rate*(ch?.139:.121));float echo=s.echo[ch][(s.position-d)&32767];s.echo[ch][at]=dry+echo*.38f;
            out[ch][i]=std::tanh((dry+wet*p[Room]+echo*p[Echo])*(p[Output]*2));
        }++s.position;
    }
    // Preserve future offsets across blocks, rather than firing early or dropping them.
    unsigned remaining=0;for(unsigned n=event;n<s.count;++n){auto m=s.events[n];m.deltaFrames-=frames;s.events[remaining++]=m;}s.count=remaining;
}
intptr_t dispatch(Effect* e,int32_t opcode,int32_t index,intptr_t value,void* ptr,float opt){auto& s=self(e);switch(opcode){
case op::open:return 1;case op::close:delete &s;return 1;
case op::setSampleRate:if(std::isfinite(opt)&&opt>=8000&&opt<=192000){s.rate=opt;s.reset();return 1;}return 0;
case op::mainsChanged:if(!value)s.reset();return 1;
case op::setProgram:s.preset(int(value));return 1;case op::getProgram:return s.program;
case op::getProgramName:if(ptr)std::snprintf(static_cast<char*>(ptr),24,"%s",presets[s.program]);return 1;
case op::getProgramNameIndexed:if(index<0||index>=programCount)return 0;if(ptr)std::snprintf(static_cast<char*>(ptr),24,"%s",presets[index]);return 1;
case op::getParamName:if(ptr&&index>=0&&index<Count)std::snprintf(static_cast<char*>(ptr),8,"%s",names[index]);return 1;
case op::getParamLabel:if(ptr)*static_cast<char*>(ptr)=0;return 1;
case op::getParamDisplay:if(ptr){float v=get(e,index);auto* t=static_cast<char*>(ptr);
    if(index==Material)std::snprintf(t,8,"%s",v<.5f?"Nylon":"Steel");
    else if(index==Direction)std::snprintf(t,8,"%s",v<.5f?"Down":"Up");
    else if(index==Strum)std::snprintf(t,8,"%.0f ms",60*v);
    else if(index==Release)std::snprintf(t,8,"%.0f ms",15+1500*v);
    else if(index==Decay)std::snprintf(t,8,"%.1f s",.3+7*v);
    else std::snprintf(t,8,"%.0f%%",100*v);}return 1;
case op::getChunk:if(!ptr)return 0;s.chunk.program=s.program;for(int i=0;i<Count;++i)s.chunk.params[i]=get(e,i);*static_cast<void**>(ptr)=&s.chunk;return sizeof(s.chunk);
case op::setChunk:if(!ptr||value!=sizeof(s.chunk))return 0;{Instrument::Chunk c;std::memcpy(&c,ptr,sizeof(c));if(c.magic!=id||c.version!=1||c.program<0||c.program>=programCount)return 0;for(float v:c.params)if(!std::isfinite(v)||v<0||v>1)return 0;for(int i=0;i<Count;++i)set(e,i,c.params[i]);s.program=c.program;}return 1;
case op::processEvents:if(ptr){auto* events=static_cast<Events<256>*>(ptr);for(int n=0;n<std::clamp(events->count,0,256);++n){auto* m=events->events[n];if(!m||m->type!=midiEventType||m->byteSize<int(sizeof(MidiEvent)))continue;
    if(s.count==256){s.reset();return 0;}auto copy=*m;copy.deltaFrames=std::max(0,copy.deltaFrames);unsigned at=s.count++;while(at&&s.events[at-1].deltaFrames>copy.deltaFrames){s.events[at]=s.events[at-1];--at;}s.events[at]=copy;}}return 1;
case op::getEffectName:case op::getProductString:if(ptr)std::snprintf(static_cast<char*>(ptr),32,"Tape Workshop Guitar");return 1;
case op::getVendorString:if(ptr)std::snprintf(static_cast<char*>(ptr),32,"Tape Workshop");return 1;
case op::getVendorVersion:return 100;case op::getVstVersion:return 2400;case op::getTailSize:return int(s.rate*40);
case op::canDo:return ptr&&(!std::strcmp(static_cast<char*>(ptr),"receiveVstEvents")||!std::strcmp(static_cast<char*>(ptr),"receiveVstMidiEvent"))?1:0;
default:return 0;}}
}
extern "C" __attribute__((visibility("default"))) vst2::Effect* VSTPluginMain(vst2::HostCallback){
    auto* s=new(std::nothrow) Instrument;if(!s)return nullptr;auto& e=s->effect;e.magic=magic;e.dispatcher=dispatch;e.setParameter=set;e.getParameter=get;e.numPrograms=programCount;e.numParams=Count;e.numOutputs=2;e.flags=flag::canReplace|flag::isSynth|flag::programChunks;e.uniqueId=int32_t(id);e.version=100;e.processReplacing=render;e.object=s;return &e;
}
