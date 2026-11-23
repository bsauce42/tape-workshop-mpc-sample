// SPDX-License-Identifier: AGPL-3.0-only
// Original Tape Workshop DSP. Host ABI: Melodee by Kerem Kiliç (Ellic Studio).
#include "melodee/plugin/vst2_abi.h"
#include "melodee/plugin/prism_controls.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstring>
#include <new>
#include <thread>
using namespace vst2;
namespace prism {
constexpr float pi=3.14159265358979323846f;
constexpr int uid=0x54575046, operators=6, harmonics=128, tableSize=512, levels=8, voiceCount=8;
enum Global {Output,Polyphony,Unison,Spread,UniDetune,MacroX,MacroY,LfoRate,LfoPitch,LfoFilter,Velocity,FMDepth,RMDepth,Chorus,Delay,DelayTime,Feedback,Reverb,Room,Drive,Transpose,MotionRate,MotionDepth,BendRange,Globals};
constexpr int opBase=Globals,opStride=10,filterBase=opBase+60,fmBase=filterBase+18,rmBase=fmBase+36,editBase=rmBase+36,Count=editBase+6;
enum Operator {Ratio,Fine,Level,Shape,Fold,Attack,Decay,Sustain,Release,Route};
constexpr const char* globalNames[]={"Output","Voices","Unison","Spread","UniTune","Macro X","Macro Y","LFO Hz","Vibrato","LFO Flt","VelSens","FM Amt","RM Amt","Chorus","Delay","DlyTime","DlyFeed","Reverb","Room","Drive","Tune","Move Hz","MoveAmt","Bend"};
constexpr const char* opNames[]={"Ratio","Fine","Level","Shape","Fold","Atk","Dec","Sus","Rel","Route"};
constexpr const char* filterNames[]={"Cut","Res","Mode","Drive","Env","EnvTime"};
constexpr const char* editNames[]={"Hrm Op","Partial","HrmGain","Move Op","Point","PtValue"};
constexpr const char* waveNames[]={"Sine","Saw","Square","Tri","Additiv","Noise"};
constexpr const char* presetNames[]={"Trap Crystal","Soul Keys","Dark Pluck","808 FM Bass","Cloud Pad","Late Night Pad","Trap Metal Bell","West Coast Lead","LoFi Keys","Dark Ring Pluck","Gospel Organ","Trap Atmosphere","Soft Trap Mallet","Gritty Mono Lead","Airy Choir Pad","Init Sine"};
inline int choice(float v,int last){return int(std::lround(v*last));}
inline float saturate(float x){x=std::clamp(x,-3.f,3.f);float xx=x*x;return x*(27+xx)/(27+9*xx);}
inline float seconds(float v){return .002f*std::pow(4000.f,v);}
inline float ratio(float v){return .25f*std::pow(64.f,v);}
inline float ratioValue(float r){return std::log(r/.25f)/std::log(64.f);}
struct Env {float value=0,start=0;int stage=0;void off(){if(stage!=3){start=value;stage=3;}}float step(float a,float d,float sus,float r){if(stage==0){value+=a;if(value>=1){value=1;stage=1;}}else if(stage==1){value=std::max(sus,value-d);if(value<=sus)stage=2;}else if(stage==2)value=sus;else value=std::max(0.f,value-start*r);return value;}};
struct Voice {bool active=false,held=false;int note=0,channel=0;unsigned age=0;float velocity=0,base=0,move=0,fade=0,elapsed=0;std::array<float,3> filterG{};std::array<Env,6> env{};std::array<std::array<float,6>,2> phase{},previous{};std::array<std::array<std::array<float,2>,3>,2> filter{};};
using Tables=std::array<std::array<std::array<float,tableSize>,levels>,operators>;
struct Chunk {uint32_t magic=uid,version=1;int32_t program=0;float params[Count]{},partials[6][128]{},curve[6][8]{};};
struct Synth {
    Effect effect{};std::array<std::atomic<float>,Count> params{};std::array<float,Count> smooth{};
    std::array<std::array<std::atomic<float>,128>,6> partials{};std::array<std::array<std::atomic<float>,8>,6> curve{};
    std::array<Tables,3> tables{};std::atomic<unsigned> middle{1};unsigned front=0,back=2;
    std::atomic<bool> running{true},dirty{false};std::thread worker;std::atomic<int> program{0};Chunk chunk{};
    std::array<Voice,8> voices{};std::array<float,16> bend{};std::array<bool,16> pedal{};
    std::array<MidiEvent,512> events{};unsigned eventCount=0,serial=0;bool overflow=false;
    std::array<float,2048> sine{};std::array<std::array<float,65536>,2> echo{};std::array<std::array<float,4096>,2> chorus{};
    std::array<std::array<float,8192>,4> room{};std::array<float,4> roomLow{};std::array<float,2> dcIn{},dcOut{};
    float rate=48000,lfo=0;unsigned pos=0;uint32_t noise=1234567;
    Synth(){for(unsigned n=0;n<sine.size();++n)sine[n]=std::sin(2*pi*n/sine.size());preset(0);for(auto& t:tables)buildTables(t);dirty=false;for(int n=0;n<Count;++n)smooth[n]=params[n].load();worker=std::thread([this]{while(running.load()){if(dirty.exchange(false)){buildTables(tables[back]);back=middle.exchange(back|8u,std::memory_order_acq_rel)&7u;}std::this_thread::sleep_for(std::chrono::milliseconds(3));}});}
    ~Synth(){running=false;if(worker.joinable())worker.join();}
    float sin(float phase)const{phase-=std::floor(phase);float at=phase*2048;unsigned n=unsigned(at);return sine[n&2047]+(sine[(n+1)&2047]-sine[n&2047])*(at-n);}
    // IFFT and all table construction run on the worker, never in the audio callback.
    void buildTables(Tables& dest){for(int op=0;op<6;++op){float harmonic[128];float total=0;for(int h=0;h<128;++h){harmonic[h]=partials[op][h].load(std::memory_order_relaxed);total+=std::abs(harmonic[h]);}float gain=1/std::max(1.f,total*.65f);
        for(int level=0;level<levels;++level){std::array<std::complex<float>,tableSize> bins{};int limit=128>>level;
            for(int h=1;h<=limit;++h){float a=harmonic[h-1]*gain*tableSize*.5f;bins[h]={0,-a};bins[tableSize-h]={0,a};}
            for(unsigned i=1,j=0;i<tableSize;++i){unsigned bit=tableSize>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(bins[i],bins[j]);}
            for(unsigned len=2;len<=tableSize;len<<=1){std::complex<float> wlen(std::cos(2*pi/len),std::sin(2*pi/len));for(unsigned i=0;i<tableSize;i+=len){std::complex<float>w(1,0);for(unsigned j=0;j<len/2;++j){auto a=bins[i+j],b=bins[i+j+len/2]*w;bins[i+j]=a+b;bins[i+j+len/2]=a-b;w*=wlen;}}}
            for(int i=0;i<tableSize;++i)dest[op][level][i]=bins[i].real()/tableSize;
        }
    }}
    void preset(int index){index=std::clamp(index,0,15);float p[Count]{};p[Output]=.65f;p[Polyphony]=3.f/7;p[Spread]=.7f;p[UniDetune]=.22f;p[MacroX]=p[MacroY]=.5f;p[LfoRate]=.25f;p[Velocity]=.8f;p[FMDepth]=.45f;p[RMDepth]=.6f;p[DelayTime]=.35f;p[Feedback]=.3f;p[Room]=.55f;p[Transpose]=.5f;p[MotionRate]=.25f;p[BendRange]=1.f/11;
        for(int op=0;op<6;++op){int b=opBase+op*10;p[b+Ratio]=ratioValue(op%2?2:1);p[b+Fine]=.5f;p[b+Level]=op==0?.8f:op==2?.35f:0;p[b+Attack]=.02f;p[b+Decay]=.35f;p[b+Sustain]=.55f;p[b+Release]=.2f;for(int h=0;h<128;++h)partials[op][h]=h==0?1:0;for(int n=0;n<8;++n)curve[op][n]=.5f;}
        for(int f=0;f<3;++f){p[filterBase+f*6]=.8f;p[filterBase+f*6+1]=.1f;p[filterBase+f*6+4]=.5f;p[filterBase+f*6+5]=.3f;}
        p[fmBase+1]=.45f;p[opBase+10+Level]=.8f; // source 2 -> destination 1; modulator need not be audible
        p[opBase+10+Route]=0;p[opBase+Level]=.8f;
        switch(index){
        case 0:p[opBase+10+Ratio]=ratioValue(3.5f);p[opBase+Sustain]=0;p[opBase+10+Sustain]=0;p[Reverb]=.28f;break;
        case 1:p[fmBase+1]=.25f;p[opBase+10+Ratio]=ratioValue(1);p[opBase+10+Decay]=.24f;p[opBase+10+Sustain]=.05f;p[Chorus]=.35f;break;
        case 2:p[opBase+Sustain]=0;p[opBase+Decay]=.28f;p[opBase+10+Ratio]=ratioValue(4);p[Delay]=.25f;break;
        case 3:p[Transpose]=.25f;p[opBase+Route]=.25f;p[filterBase]=.35f;p[opBase+10+Ratio]=ratioValue(1);p[opBase+10+Sustain]=.2f;p[fmBase+1]=.3f;break;
        case 4:case 5:p[Unison]=0;p[opBase+Shape]=.2f;p[opBase+Attack]=.55f;p[opBase+Release]=.36f;p[opBase+Route]=.25f;p[filterBase]=.5f;p[fmBase+1]=.08f;p[Chorus]=.4f;p[Reverb]=.4f;p[MotionDepth]=.35f;if(index==5){p[Delay]=.32f;p[MacroX]=.75f;p[LfoFilter]=.25f;p[UniDetune]=.4f;p[opBase+20+Fine]=.54f;}break;
        case 6:p[fmBase+1]=.7f;p[fmBase+6]=.3f;p[opBase+10+Ratio]=ratioValue(2.71f);p[MotionDepth]=.8f;p[Delay]=.4f;break;
        case 7:p[Polyphony]=0;p[Unison]=0;p[opBase+Shape]=.2f;p[opBase+Route]=.25f;p[filterBase]=.6f;p[LfoPitch]=.12f;p[Delay]=.25f;break;
        case 8:p[opBase+10+Ratio]=ratioValue(3);p[fmBase+1]=.18f;p[Reverb]=.25f;break;
        case 9:p[opBase+Sustain]=0;p[opBase+Decay]=.3f;p[fmBase+1]=0;p[rmBase+1]=.75f;p[opBase+10+Ratio]=ratioValue(1.41f);p[Reverb]=.5f;break;
        case 10:p[opBase+Shape]=.8f;p[fmBase+1]=0;for(int h:{1,2,3,4,6,8})partials[0][h-1]=1.f/h;p[Chorus]=.3f;break;
        case 11:p[opBase+Attack]=.8f;p[opBase+Release]=.4f;p[fmBase+1]=.6f;p[Reverb]=.55f;p[MotionDepth]=.7f;p[Unison]=0;break;
        case 12:p[opBase+Sustain]=0;p[opBase+Decay]=.3f;p[fmBase+1]=.15f;p[Reverb]=.2f;break;
        case 13:p[opBase+Shape]=.2f;p[opBase+Route]=.25f;p[filterBase]=.3f;p[filterBase+1]=.65f;p[filterBase+4]=.85f;p[fmBase+1]=0;p[Drive]=.35f;break;
        case 14:p[opBase+Shape]=.8f;for(int h=1;h<=128;++h)partials[0][h-1]=(h%5==1||h%7==0)?1.f/h:0;p[opBase+Attack]=.6f;p[opBase+Release]=.36f;p[Chorus]=.5f;p[Reverb]=.5f;break;
        case 15:p[fmBase+1]=0;p[opBase+20+Level]=0;break;
        }
        // Modulators have Route=Off. Carrier 1 defaults to Direct; operator 3 is a quiet direct layer.
        p[opBase+10+Route]=1;for(int op=3;op<6;++op)p[opBase+op*10+Route]=1;
        for(int op=0;op<6;++op)for(int n=0;n<8;++n)curve[op][n]=.5f+.4f*std::sin(2*pi*n/8+op*.5f);
        for(int n=0;n<Count;++n)params[n].store(p[n],std::memory_order_relaxed);program=index;dirty=true;
    }
    void release(Voice& v){v.held=false;for(auto& env:v.env)env.off();}
    void panic(){for(auto&v:voices)v={};pedal.fill(false);eventCount=0;}
    void midi(const MidiEvent& e){unsigned ch=uint8_t(e.data[0])&15,type=uint8_t(e.data[0])&240,key=uint8_t(e.data[1])&127,vel=uint8_t(e.data[2])&127;
        if(type==0xe0){bend[ch]=(int(key)+128*int(vel)-8192)/8192.f;return;}
        if(type==0xb0){if(key==64){pedal[ch]=vel>=64;if(!pedal[ch])for(auto&v:voices)if(v.active&&v.channel==int(ch)&&!v.held)release(v);}if(key==120||key==123)for(auto&v:voices)if(v.channel==int(ch)){if(key==120)v={};else release(v);}return;}
        if(type==0x80||(type==0x90&&!vel)){for(auto&v:voices)if(v.active&&v.channel==int(ch)&&v.note==int(key)){v.held=false;if(!pedal[ch])release(v);}return;}
        if(type!=0x90)return;int cap=1+choice(params[Polyphony].load(),7);Voice*take=&voices[0];for(int n=0;n<cap;++n){auto&v=voices[n];if(!v.active){take=&v;break;}if(v.age<take->age)take=&v;}*take={};take->active=take->held=true;take->note=key;take->channel=ch;take->velocity=vel/127.f;take->age=++serial;take->base=440*std::pow(2.f,(int(key)-69)/12.f)/rate;
    }
};
Synth& self(Effect*e){return *static_cast<Synth*>(e->object);}
float get(Effect*e,int32_t n){if(n<0||n>=Count)return 0;auto&s=self(e);if(n==editBase+2)return s.partials[choice(s.params[editBase].load(),5)][choice(s.params[editBase+1].load(),127)].load();if(n==editBase+5)return s.curve[choice(s.params[editBase+3].load(),5)][choice(s.params[editBase+4].load(),7)].load();return s.params[n].load(std::memory_order_relaxed);}
void set(Effect*e,int32_t n,float v){if(n<0||n>=Count||!std::isfinite(v))return;auto&s=self(e);v=std::clamp(v,0.f,1.f);if(n==editBase+2){s.partials[choice(s.params[editBase].load(),5)][choice(s.params[editBase+1].load(),127)]=v;s.dirty=true;}else if(n==editBase+5)s.curve[choice(s.params[editBase+3].load(),5)][choice(s.params[editBase+4].load(),7)]=v;s.params[n]=v;}
float blep(float t,float dt){if(t<dt){t/=dt;return t+t-t*t-1;}if(t>1-dt){t=(t-1)/dt;return t*t+t+t+1;}return 0;}
float wave(Synth&s,int op,int shape,float phase,float dt){phase-=std::floor(phase);switch(shape){case 0:return s.sin(phase);case 1:return 2*phase-1-blep(phase,dt);case 2:return (phase<.5f?1.f:-1.f)+blep(phase,dt)-blep(std::fmod(phase+.5f,1.f),dt);case 3:return 1-4*std::abs(phase-.5f);case 4:{int level=0;while(level<7&&(128>>level)*dt>.45f)++level;float at=phase*tableSize;unsigned n=unsigned(at);const auto&t=s.tables[s.front][op][level];return t[n&511]+(t[(n+1)&511]-t[n&511])*(at-n);}default:s.noise^=s.noise<<13;s.noise^=s.noise>>17;s.noise^=s.noise<<5;return int32_t(s.noise)/2147483648.f;}}
void process(Effect*e,float**,float**out,int32_t frames){auto&s=self(e);if(frames<=0||!out||!out[0]||!out[1])return;if(s.middle.load(std::memory_order_acquire)&8)s.front=s.middle.exchange(s.front,std::memory_order_acq_rel)&7;
    if(s.overflow){s.panic();s.overflow=false;}
    std::array<float,Count> target{};for(int n=0;n<Count;++n)target[n]=s.params[n].load(std::memory_order_relaxed);
    int cap=1+choice(target[Polyphony],7),unison=1+choice(target[Unison],1);for(int n=cap;n<voiceCount;++n)if(s.voices[n].active)s.release(s.voices[n]);
    struct Op {float ratio,fine,a,d,sus,r;int shape,route;};std::array<Op,6> op{};
    for(int o=0;o<6;++o){int b=opBase+o*10;op[o]={ratio(target[b]),std::pow(2.f,(target[b+Fine]-.5f)*2/12),1/(seconds(target[b+Attack])*s.rate),1/(seconds(target[b+Decay])*s.rate),target[b+Sustain],1/(seconds(target[b+Release])*s.rate),choice(target[b+Shape],5),choice(target[b+Route],4)};}
    float bends[16];for(int ch=0;ch<16;++ch)bends[ch]=std::pow(2.f,s.bend[ch]*(1+choice(target[BendRange],11))/12);
    float move[6][8];for(int o=0;o<6;++o)for(int n=0;n<8;++n)move[o][n]=s.curve[o][n].load(std::memory_order_relaxed);
    float trans=std::pow(2.f,std::round((target[Transpose]-.5f)*48)/12),detune=std::pow(2.f,target[UniDetune]*.4f/12),glide=1-std::exp(-16/(s.rate*.008f));
    unsigned event=0;float filterG[3],filterRes[3];int filterMode[3];for(int f=0;f<3;++f){int b=filterBase+6*f;filterG[f]=target[b];filterRes[f]=2-1.85f*target[b+1];filterMode[f]=choice(target[b+2],2);}
    // Build a sparse modulation plan once per block, including fading connections.
    bool needed[6]{},filterUsed[3]{};unsigned fmSources[6][6]{},rmSources[6][6]{},fmCount[6]{},rmCount[6]{};
    for(int o=0;o<6;++o){if(op[o].route<4&&std::max(target[opBase+o*10+Level],s.smooth[opBase+o*10+Level])>.00001f)needed[o]=true;for(int source=0;source<6;++source){if(std::max(target[fmBase+o*6+source],s.smooth[fmBase+o*6+source])>.00001f)fmSources[o][fmCount[o]++]=source;if(std::max(target[rmBase+o*6+source],s.smooth[rmBase+o*6+source])>.00001f)rmSources[o][rmCount[o]++]=source;}}
    for(int pass=0;pass<6;++pass)for(int o=0;o<6;++o)if(needed[o]){for(unsigned k=0;k<fmCount[o];++k)needed[fmSources[o][k]]=true;for(unsigned k=0;k<rmCount[o];++k)needed[rmSources[o][k]]=true;}
    for(int o=0;o<6;++o)if(needed[o]&&op[o].route>0&&op[o].route<4)filterUsed[op[o].route-1]=true;
    for(int i=0;i<frames;++i){if((s.pos&15)==0)for(int n=0;n<editBase;++n)s.smooth[n]+=(target[n]-s.smooth[n])*glide;auto&p=s.smooth;while(event<s.eventCount&&s.events[event].deltaFrames<=i)s.midi(s.events[event++]);float lfo=s.sin(s.lfo);s.lfo+=(.05f+15*p[LfoRate]*p[LfoRate])/s.rate;s.lfo-=std::floor(s.lfo);float sum[2]{};
        for(auto&v:s.voices){if(!v.active)continue;v.fade=std::min(1.f,v.fade+1/(s.rate*.003f));bool alive=v.held||s.pedal[v.channel];float env[6]{};for(int o=0;o<6;++o){if(!needed[o])continue;env[o]=v.env[o].step(op[o].a,op[o].d,op[o].sus,op[o].r);alive|=v.env[o].stage!=3||env[o]>.00001f;}if(!alive){v.active=false;continue;}
            v.elapsed+=1/s.rate;v.move+=(.05f+8*p[MotionRate]*p[MotionRate])/s.rate;v.move-=std::floor(v.move);float base=v.base*trans*bends[v.channel]*(1+lfo*p[LfoPitch]*.018f);
            for(int u=0;u<unison;++u){float current[6]{},filtered[3]{},direct=0;float tune=unison==1?1:(u?detune:1/detune);
                for(int o=0;o<6;++o){if(!needed[o])continue;float fm=0,rm=1;for(unsigned k=0;k<fmCount[o];++k){unsigned source=fmSources[o][k];fm+=v.previous[u][source]*p[fmBase+o*6+source];}for(unsigned k=0;k<rmCount[o];++k){unsigned source=rmSources[o][k];float amount=p[rmBase+o*6+source]*p[RMDepth];rm*=1-amount+v.previous[u][source]*amount;}
                    float at=v.move*8;int node=int(at);float fraction=at-node;fraction=fraction*fraction*(3-2*fraction);float motion=move[o][node&7]+(move[o][(node+1)&7]-move[o][node&7])*fraction;
                    float dt=std::clamp(base*op[o].ratio*op[o].fine*tune,.000001f,.4f);float phase=v.phase[u][o]+fm*p[FMDepth]*(.1f+1.8f*p[MacroX]);float x=wave(s,o,op[o].shape,phase,dt);v.phase[u][o]+=dt;v.phase[u][o]-=std::floor(v.phase[u][o]);float fold=p[opBase+o*10+Fold];if(fold>.00001f)x=saturate(x*(1+fold*7))/(1+fold*1.7f);current[o]=x*env[o]*p[opBase+o*10+Level]*(1+p[MotionDepth]*(motion-.5f));float y=current[o]*rm;
                    if(op[o].route==0)direct+=y;else if(op[o].route<4)filtered[op[o].route-1]+=y;
                }
                for(int o=0;o<6;++o)v.previous[u][o]=current[o];
                for(int f=0;f<3;++f){if(!filterUsed[f]){v.filter[u][f]={};continue;}int b=filterBase+6*f;if((s.pos&15)==0||v.elapsed<=1/s.rate){float decay=std::exp(-v.elapsed/( .04f+target[b+5]*1.5f));float cut=std::clamp(filterG[f]+(p[MacroY]-.5f)*.5f+lfo*p[LfoFilter]*.3f+(target[b+4]-.5f)*decay,0.f,1.f);float hz=30*std::pow(600.f,cut);v.filterG[f]=std::tan(pi*std::min(hz,s.rate*.4f)/s.rate);}float g=v.filterG[f];float x=saturate(filtered[f]*(1+p[b+3]*8));auto&state=v.filter[u][f];float band=(state[0]+g*(x-state[1]))/(1+g*(g+filterRes[f])),low=state[1]+g*band;state[0]=2*band-state[0];state[1]=2*low-state[1];direct+=filterMode[f]==0?low:filterMode[f]==1?x-filterRes[f]*band-low:band;}
                float gain=.09f*v.fade*(1-p[Velocity]+p[Velocity]*v.velocity)/unison;float pan=unison==1?0:(u?1.f:-1.f)*p[Spread];sum[0]+=direct*gain*(1-pan*.5f);sum[1]+=direct*gain*(1+pan*.5f);
            }
        }
        float reverbInput=(sum[0]+sum[1])*.5f,wet[4];unsigned lengths[4]={1499,1877,2137,2671};float total=0;
        for(int n=0;n<4;++n){unsigned d=unsigned(lengths[n]*(s.rate/48000)*(.6f+p[Room]));d=std::min(8191u,d);wet[n]=s.room[n][(s.pos-d)&8191];s.roomLow[n]+=.25f*(wet[n]-s.roomLow[n]);total+=s.roomLow[n];}
        for(int n=0;n<4;++n)s.room[n][s.pos&8191]=reverbInput+(s.roomLow[n]-total*.5f)*(.55f+p[Room]*.34f);
        for(int ch=0;ch<2;++ch){float x=saturate(sum[ch]*(1+p[Drive]*6));unsigned cd=unsigned(s.rate*(.012f+.004f*s.sin(s.lfo+ch*.3f)));float chor=s.chorus[ch][(s.pos-std::min(4095u,cd))&4095];s.chorus[ch][s.pos&4095]=x;x=x*(1-p[Chorus]*.3f)+chor*p[Chorus]*.3f;
            unsigned delay=unsigned(s.rate*(.04f+p[DelayTime]*.65f));float echo=s.echo[ch][(s.pos-std::min(65535u,delay))&65535];s.echo[ch][s.pos&65535]=x+echo*p[Feedback]*.8f;x+=echo*p[Delay]+(wet[ch]+wet[ch+2])*.35f*p[Reverb];float dc=x-s.dcIn[ch]+.995f*s.dcOut[ch];s.dcIn[ch]=x;s.dcOut[ch]=dc;out[ch][i]=saturate(dc*p[Output]*1.5f);
        }++s.pos;
    }
    // Preserve events outside this block with remaining sample offsets.
    unsigned pending=0;for(;event<s.eventCount;++event){auto next=s.events[event];next.deltaFrames-=frames;s.events[pending++]=next;}s.eventCount=pending;
}
void name(int n,char*out){if(n<Globals)std::snprintf(out,8,"%s",globalNames[n]);else if(n<filterBase)std::snprintf(out,8,"%d%s",1+(n-opBase)/10,opNames[(n-opBase)%10]);else if(n<fmBase)std::snprintf(out,8,"F%d%s",1+(n-filterBase)/6,filterNames[(n-filterBase)%6]);else if(n<editBase){int k=n<(rmBase)?n-fmBase:n-rmBase;std::snprintf(out,8,"%s%d>%d",n<rmBase?"FM":"RM",1+k%6,1+k/6);}else std::snprintf(out,8,"%s",editNames[n-editBase]);}
void text(void*ptr,const char*value){if(ptr){std::strncpy(static_cast<char*>(ptr),value,63);static_cast<char*>(ptr)[63]=0;}}
intptr_t dispatch(Effect*e,int32_t opcode,int32_t index,intptr_t value,void*ptr,float opt){auto&s=self(e);switch(opcode){
case melodee::prism_ui::opcode:if(index==melodee::prism_ui::command&&ptr&&value==sizeof(melodee::prism_ui::Info)){melodee::prism_ui::Info info;int a=choice(s.params[editBase].load(),5),b=choice(s.params[editBase+3].load(),5);for(int n=0;n<128;++n)info.harmonics[n]=s.partials[a][n].load();for(int n=0;n<8;++n)info.curve[n]=s.curve[b][n].load();std::memcpy(ptr,&info,sizeof info);return 1;}return 0;
case op::open:return 1;case op::close:delete &s;return 1;
case op::setSampleRate:if(std::isfinite(opt)&&opt>=8000&&opt<=192000){s.rate=opt;s.panic();}return 1;
case op::setBlockSize:return 1;case op::mainsChanged:if(!value)s.panic();return 1;
case op::setProgram:s.preset(int(value));return 1;case op::getProgram:return s.program.load();
case op::getProgramName:if(ptr)std::snprintf(static_cast<char*>(ptr),24,"%s",presetNames[s.program.load()]);return 1;
case op::getProgramNameIndexed:if(index<0||index>=16)return 0;if(ptr)std::snprintf(static_cast<char*>(ptr),24,"%s",presetNames[index]);return 1;
case op::getParamName:if(ptr&&index>=0&&index<Count)name(index,static_cast<char*>(ptr));return 1;
case op::getParamLabel:if(ptr)static_cast<char*>(ptr)[0]=0;return 1;
case op::getParamDisplay:if(ptr&&index>=0&&index<Count){float v=get(e,index);char*out=static_cast<char*>(ptr);if(index==Polyphony)std::snprintf(out,8,"%d",1+choice(v,7));else if(index==Unison)std::snprintf(out,8,"%dx",1+choice(v,1));else if(index==Transpose)std::snprintf(out,8,"%+d",int(std::round((v-.5f)*48)));else if(index==BendRange)std::snprintf(out,8,"%d st",1+choice(v,11));else if(index==LfoRate||index==MotionRate)std::snprintf(out,8,"%.2fHz",.05f+(index==LfoRate?15:8)*v*v);else if(index==DelayTime)std::snprintf(out,8,"%.0fms",40+650*v);else if(index==UniDetune)std::snprintf(out,8,"%.0fct",40*v);else if(index>=opBase&&index<filterBase&&((index-opBase)%10==Attack||(index-opBase)%10==Decay||(index-opBase)%10==Release))std::snprintf(out,8,"%.3fs",seconds(v));else if(index>=opBase&&index<filterBase&&(index-opBase)%10==Fine)std::snprintf(out,8,"%+.0fct",(v-.5f)*200);else if(index>=filterBase&&index<fmBase&&(index-filterBase)%6==0)std::snprintf(out,8,"%.0fHz",30*std::pow(600.f,v));else if(index>=filterBase&&index<fmBase&&(index-filterBase)%6==5)std::snprintf(out,8,"%.2fs",.04f+v*1.5f);else if(index>=opBase&&index<filterBase&&(index-opBase)%10==Ratio)std::snprintf(out,8,"%.2fx",ratio(v));else if(index>=opBase&&index<filterBase&&(index-opBase)%10==Shape)std::snprintf(out,8,"%s",waveNames[choice(v,5)]);else if(index>=opBase&&index<filterBase&&(index-opBase)%10==Route){const char*r[]={"Direct","Flt 1","Flt 2","Flt 3","Off"};std::snprintf(out,8,"%s",r[choice(v,4)]);}else if(index>=filterBase&&index<fmBase&&(index-filterBase)%6==2){const char*m[]={"Lowpass","Highpas","Bandpas"};std::snprintf(out,8,"%s",m[choice(v,2)]);}else if(index==editBase||index==editBase+3)std::snprintf(out,8,"%d",1+choice(v,5));else if(index==editBase+1)std::snprintf(out,8,"%d",1+choice(v,127));else if(index==editBase+4)std::snprintf(out,8,"%d",1+choice(v,7));else std::snprintf(out,8,"%.1f%%",v*100);}return 1;
case op::getChunk:if(!ptr)return 0;s.chunk.program=s.program.load();for(int n=0;n<Count;++n)s.chunk.params[n]=get(e,n);for(int o=0;o<6;++o){for(int h=0;h<128;++h)s.chunk.partials[o][h]=s.partials[o][h].load();for(int n=0;n<8;++n)s.chunk.curve[o][n]=s.curve[o][n].load();}*static_cast<void**>(ptr)=&s.chunk;return sizeof(Chunk);
case op::setChunk:{if(!ptr||value!=sizeof(Chunk))return 0;Chunk c;std::memcpy(&c,ptr,sizeof c);if(c.magic!=uid||c.version!=1||c.program<0||c.program>=16)return 0;for(float v:c.params)if(!std::isfinite(v)||v<0||v>1)return 0;for(auto&row:c.partials)for(float v:row)if(!std::isfinite(v)||v<0||v>1)return 0;for(auto&row:c.curve)for(float v:row)if(!std::isfinite(v)||v<0||v>1)return 0;for(int n=0;n<Count;++n)s.params[n]=c.params[n];for(int o=0;o<6;++o){for(int h=0;h<128;++h)s.partials[o][h]=c.partials[o][h];for(int n=0;n<8;++n)s.curve[o][n]=c.curve[o][n];}s.program=c.program;s.dirty=true;return 1;}
case op::processEvents:if(ptr){auto&list=*static_cast<Events<512>*>(ptr);for(int n=0;n<std::clamp(list.count,0,512);++n){const auto*m=list.events[n];if(!m||m->type!=midiEventType||m->byteSize<int(sizeof(MidiEvent)))continue;if(s.eventCount>=512){s.overflow=true;break;}auto copy=*m;copy.deltaFrames=std::max(0,copy.deltaFrames);unsigned at=s.eventCount++;while(at&&s.events[at-1].deltaFrames>copy.deltaFrames){s.events[at]=s.events[at-1];--at;}s.events[at]=copy;}}return 1;
case op::getEffectName:case op::getProductString:text(ptr,"Prism FM");return 1;case op::getVendorString:text(ptr,"Tape Workshop");return 1;case op::getVendorVersion:return 102;case op::getVstVersion:return 2400;case op::getTailSize:return int(s.rate*12);
case op::canDo:return ptr&&(!std::strcmp(static_cast<char*>(ptr),"receiveVstEvents")||!std::strcmp(static_cast<char*>(ptr),"receiveVstMidiEvent"))?1:0;default:return 0;}}
}
extern "C" __attribute__((visibility("default"))) vst2::Effect* VSTPluginMain(vst2::HostCallback){auto*s=new(std::nothrow)prism::Synth;if(!s)return nullptr;auto&e=s->effect;e.magic=vst2::magic;e.dispatcher=prism::dispatch;e.setParameter=prism::set;e.getParameter=prism::get;e.numPrograms=16;e.numParams=prism::Count;e.numOutputs=2;e.flags=flag::canReplace|flag::isSynth|flag::programChunks;e.uniqueId=prism::uid;e.version=102;e.processReplacing=prism::process;e.object=s;return &e;}
