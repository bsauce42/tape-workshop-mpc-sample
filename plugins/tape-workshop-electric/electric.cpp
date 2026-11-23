// SPDX-License-Identifier: AGPL-3.0-only
// Original DSP. Host ABI: Melodee by Kerem Kiliç (Ellic Studio).
// Reuse the bounded plucked-string engine, with separate identity/state and an electric rack.
#define VSTPluginMain AcousticCoreEntry
#include "../tape-workshop-guitar/guitar.cpp"
#undef VSTPluginMain
namespace electric {
constexpr uint32_t uid=0x54574547;
enum Parameter { Pickup, Position, Brightness, Ring, PalmMute, Resonance, Attack, Release, Velocity,
    Strum, Direction, Detune, Width, Vibrato, VibRate, Room, Echo, Output,
    Amp, Gain, AmpLevel, Bass, Mid, Treble, Cabinet, Mic, Gate,
    Pedal1, Amount1, Tone1, Pedal2, Amount2, Tone2, Pedal3, Amount3, Tone3,
    Rate, Time, Parameters };
constexpr const char* labels[]={"Pickup","PickPos","Bright","Ring","Mute","Reson","Attack","Release","VelSens","Strum","StrmDir","Detune","Width","Vibrato","VibRate","Room","Echo","Output","Amp","Gain","AmpLevel","Bass","Mid","Treble","Cabinet","Mic","Gate","Slot1","Amount1","Tone1","Slot2","Amount2","Tone2","Slot3","Amount3","Tone3","FxRate","DelayMs"};
constexpr const char* amps[]={"Clean","Jazz","Brit","HighGain"};
constexpr const char* cabs[]={"Direct","Open112","Closed212","Stack412"};
constexpr const char* pedals[]={"Off","Comp","Overdrv","Fuzz","Wah","Chorus","Phaser","Tremolo","Delay","Reverb"};
constexpr const char* programs[]={"Clean Single","Warm Jazz","Brit Crunch","High Gain","Funk Wah","Dream Chorus","Lead Echo","Ambient Swell"};
constexpr float initial[]={1,.23f,.8f,.65f,0,.15f,0,.18f,.8f,0,0,.08f,.25f,0,.35f,.05f,0,.6f,0,.25f,.55f,.5f,.5f,.5f,1.f/3,.55f,.02f,0,.4f,.5f,0,.4f,.5f,0,.35f,.5f,.25f,.5f};
static_assert(sizeof(initial)/sizeof(*initial)==Parameters);
struct Slot {
    std::array<std::array<float,65536>,2> line{};
    std::array<float,2> low{},env{},last{},dc{},dcIn{};
    std::array<std::array<float,4>,2> phase{};
    double lfo=0;unsigned position=0;int type=-1;float mix=0;
};
struct Guitar {
    Effect effect{};Effect* core=nullptr;std::array<std::atomic<float>,Parameters> parameters;
    std::array<Slot,3> slots{};
    std::array<float,2> bass{},treble{},cab{},cab2{},gateEnv{},ampLast{},ampTone{},ampDC{},ampDCIn{};
    double rate=44100;int program=0;bool resetting=false;
    struct Chunk {uint32_t magic=uid,version=1;int32_t program=0;float params[Parameters]{};} chunk;
    Guitar(){core=AcousticCoreEntry(nullptr);}
    ~Guitar(){if(core)core->dispatcher(core,op::close,0,0,nullptr,0);}
    void preset(int n){program=std::clamp(n,0,7);float p[Parameters];std::copy(std::begin(initial),std::end(initial),p);
        switch(program){
        case 1:p[Amp]=1.f/3;p[Gain]=.15f;p[Brightness]=.5f;p[Treble]=.3f;p[Pickup]=0;break;
        case 2:p[Amp]=2.f/3;p[Gain]=.55f;p[Cabinet]=2.f/3;p[Pedal1]=2.f/9;p[Amount1]=.2f;break;
        case 3:p[Amp]=1;p[Gain]=.7f;p[Cabinet]=1;p[Gate]=.3f;p[PalmMute]=.2f;break;
        case 4:p[Pedal1]=1.f/9;p[Pedal2]=4.f/9;p[Amount2]=.65f;p[Ring]=.4f;break;
        case 5:p[Pedal2]=5.f/9;p[Amount2]=.5f;p[Room]=.25f;break;
        case 6:p[Amp]=2.f/3;p[Gain]=.5f;p[Pedal1]=1.f/9;p[Pedal3]=8.f/9;p[Amount3]=.35f;break;
        case 7:p[Attack]=.7f;p[Ring]=.95f;p[Release]=.7f;p[Pedal2]=5.f/9;p[Pedal3]=9.f/9;p[Amount3]=.7f;break;
        }for(int i=0;i<Parameters;++i)parameters[i].store(p[i],std::memory_order_relaxed);
    }
    void reset(){for(auto& slot:slots){for(auto& c:slot.line)c.fill(0);for(auto& c:slot.phase)c.fill(0);slot.low.fill(0);slot.env.fill(0);slot.last.fill(0);slot.dc.fill(0);slot.dcIn.fill(0);slot.position=0;slot.lfo=0;slot.type=-1;slot.mix=0;}
        bass.fill(0);treble.fill(0);cab.fill(0);cab2.fill(0);gateEnv.fill(0);ampLast.fill(0);ampTone.fill(0);ampDC.fill(0);ampDCIn.fill(0);
    }
};
Guitar& self(Effect* e){return *static_cast<Guitar*>(e->object);}
float get(Effect* e,int32_t n){return n>=0&&n<Parameters?self(e).parameters[n].load(std::memory_order_relaxed):0;}
void set(Effect* e,int32_t n,float v){if(n>=0&&n<Parameters&&std::isfinite(v))self(e).parameters[n].store(std::clamp(v,0.f,1.f),std::memory_order_relaxed);}
int choice(float p,int max){return int(std::lround(p*max));}
float coefficient(double rate,float hz){return 1-std::exp(-2*pi*std::min(float(rate*.4),hz)/float(rate));}
float pedal(Slot& s,int type,float amount,float tone,float rate,float time,double sr,unsigned ch,float x){
    float lfo=std::sin(float(s.lfo*2*pi)+(ch? .7f:0));unsigned at=s.position&65535;
    float wet=x;
    switch(type){
    case 1:{float level=std::abs(x);s.env[ch]+=(level>s.env[ch]?.03f:.0008f)*(level-s.env[ch]);float threshold=.25f-.22f*amount;float gain=s.env[ch]>threshold?std::sqrt(threshold/std::max(.0001f,s.env[ch])):1;wet=x*gain*(1+amount);break;}
    case 2:case 3:{// Two nonlinear substeps and post filtering reduce sharp digital edges.
        float drive=type==2?1+amount*12:3+amount*35;
        float a=std::tanh((x+s.last[ch])*.5f*drive),b=std::tanh(x*drive);s.last[ch]=x;
        wet=(a+b)*.5f/(1+amount*2);if(type==3)wet=std::tanh(wet*2+.15f)-std::tanh(.15f);
        s.low[ch]+= (.08f+.75f*tone)*(wet-s.low[ch]);wet=s.low[ch];break;}
    case 4:{// Stable swept state-variable bandpass; Tone sets sweep center.
        float hz=250+2200*std::clamp(tone+amount*lfo*.4f,0.f,1.f);
        float g=std::tan(pi*std::min(hz,float(sr*.2))/float(sr));float a=1/(1+g*(g+.6f));
        float band=a*(s.phase[ch][1]+g*(x-s.phase[ch][0]));float low=s.phase[ch][0]+g*band;
        s.phase[ch][1]=2*band-s.phase[ch][1];s.phase[ch][0]=2*low-s.phase[ch][0];wet=x*(1-amount)+band*amount*1.5f;break;}
    case 5:{float delay=float(sr)*(.012f+.006f*lfo*(.2f+.8f*amount));unsigned whole=unsigned(delay);float frac=delay-whole;
        float delayed=(s.position>=whole?s.line[ch][(s.position-whole)&65535]:0)*(1-frac)+(s.position>whole?s.line[ch][(s.position-whole-1)&65535]:0)*frac;
        s.line[ch][at]=x;wet=x*(1-amount*.5f)+delayed*amount*.5f;break;}
    case 6:{float a=.1f+.75f*(.5f+.5f*lfo);float y=x;
        for(auto& state:s.phase[ch]){float z=-a*y+state;state=y+a*z;y=z;}
        wet=x*(1-amount*.5f)+y*amount*.5f;break;}
    case 7:wet=x*(1-amount*(.5f+.5f*lfo));break;
    case 8:{unsigned delay=unsigned(sr*(.04f+.26f*time));float y=s.position>=delay?s.line[ch][(s.position-delay)&65535]:0;s.low[ch]+=(.1f+.7f*tone)*(y-s.low[ch]);s.line[ch][at]=x+s.low[ch]*amount*.65f;wet=x+y*amount;break;}
    case 9:{unsigned d1=unsigned(sr*(ch?.043:.037)),d2=unsigned(sr*(ch?.071:.061));float y=.5f*((s.position>=d1?s.line[ch][(s.position-d1)&65535]:0)+(s.position>=d2?s.line[ch][(s.position-d2)&65535]:0));s.low[ch]+=(.05f+.5f*tone)*(y-s.low[ch]);s.line[ch][at]=x+s.low[ch]*(.45f+.35f*amount);wet=x+y*amount;break;}
    }
    // DC rejection also handles asymmetrical fuzz.
    float clean=wet-s.dcIn[ch]+.995f*s.dc[ch];s.dcIn[ch]=wet;s.dc[ch]=clean;
    return type?x+(clean-x)*s.mix:x;
}
void render(Effect* e,float**,float** out,int32_t frames){auto& s=self(e);float p[Parameters];for(int i=0;i<Parameters;++i)p[i]=get(e,i);
    // Excitation comes from the existing steel model; Pickup changes damping/brightness, not steel/nylon.
    for(int i=0;i<18;++i)s.core->setParameter(s.core,i,p[i]);
    s.core->setParameter(s.core,0,1);s.core->setParameter(s.core,2,std::clamp(p[Brightness]*(.7f+.3f*p[Pickup]),0.f,1.f));
    s.core->setParameter(s.core,17,.6f);s.core->processReplacing(s.core,nullptr,out,frames);
    int amp=choice(p[Amp],3),cabinet=choice(p[Cabinet],3);float lowC=coefficient(s.rate,220),highC=coefficient(s.rate,2500);
    float cabC=coefficient(s.rate,(cabinet==1?6500: cabinet==2?4800:3500)*(.7f+.6f*p[Mic]));
    float gateThreshold=p[Gate]*.02f,drive=1+p[Gain]*(amp==3?60:amp==2?18:amp==1?3:5);
    int types[3];for(int n=0;n<3;++n){types[n]=choice(p[Pedal1+n*3],9);auto& slot=s.slots[n];
        // Fade out old slot before clearing a short fixed state and fading in the new effect.
        if(slot.type!=types[n]&&slot.mix<=.001f){slot.type=types[n];slot.low.fill(0);slot.env.fill(0);slot.last.fill(0);for(auto& c:slot.phase)c.fill(0);slot.dc.fill(0);slot.dcIn.fill(0);
            // Reads are zero until each new delay history is populated; stale rack tails cannot reappear.
            slot.position=0;
        }
    }
    for(int i=0;i<frames;++i){for(auto& slot:s.slots){slot.lfo+=(.08+7*p[Rate]*p[Rate])/s.rate;if(slot.lfo>=1)slot.lfo-=1;}
        for(int n=0;n<3;++n){auto& slot=s.slots[n];float target=slot.type==types[n]?1:0;slot.mix+=std::clamp(target-slot.mix,-.002f,.002f);}
        for(unsigned ch=0;ch<2;++ch){float x=out[ch][i];
            // Rack slots 1 and 2 precede amp/cab; slot 3 follows cabinet.
            for(int n=0;n<2;++n)x=pedal(s.slots[n],s.slots[n].type,p[Amount1+n*3],p[Tone1+n*3],p[Rate],p[Time],s.rate,ch,x);
            s.gateEnv[ch]+=.01f*(std::abs(x)-s.gateEnv[ch]);if(gateThreshold>0)x*=std::min(1.f,s.gateEnv[ch]/std::max(.000001f,gateThreshold));
            float a=std::tanh((x+s.ampLast[ch])*.5f*drive),b=std::tanh(x*drive);s.ampLast[ch]=x;x=(a+b)*.5f;
            float clean=x-s.ampDCIn[ch]+.995f*s.ampDC[ch];s.ampDCIn[ch]=x;s.ampDC[ch]=clean;x=clean;
            x*=p[AmpLevel]*(amp>=2?.8f:1.3f);
            s.bass[ch]+=lowC*(x-s.bass[ch]);s.treble[ch]+=highC*(x-s.treble[ch]);
            x=s.bass[ch]*(.25f+1.5f*p[Bass])+(s.treble[ch]-s.bass[ch])*(.25f+1.5f*p[Mid])+(x-s.treble[ch])*(.25f+1.5f*p[Treble]);
            if(cabinet){s.cab[ch]+=cabC*(x-s.cab[ch]);s.cab2[ch]+=cabC*(s.cab[ch]-s.cab2[ch]);x=s.cab2[ch];}
            x=pedal(s.slots[2],s.slots[2].type,p[Amount3],p[Tone3],p[Rate],p[Time],s.rate,ch,x);
            out[ch][i]=std::tanh(x*p[Output]*1.5f);
        }for(auto& slot:s.slots)++slot.position;
    }
}
intptr_t dispatch(Effect* e,int32_t opcode,int32_t index,intptr_t value,void* ptr,float opt){auto& s=self(e);switch(opcode){
case op::open:return 1;case op::close:delete &s;return 1;
case op::setSampleRate:if(!std::isfinite(opt)||opt<8000||opt>192000)return 0;s.rate=opt;s.reset();return s.core->dispatcher(s.core,opcode,index,value,ptr,opt);
case op::mainsChanged:if(!value)s.reset();return s.core->dispatcher(s.core,opcode,index,value,ptr,opt);
case op::processEvents:return s.core->dispatcher(s.core,opcode,index,value,ptr,opt);
case op::setProgram:s.preset(int(value));return 1;case op::getProgram:return s.program;
case op::getProgramName:if(ptr)std::snprintf(static_cast<char*>(ptr),24,"%s",programs[s.program]);return 1;
case op::getProgramNameIndexed:if(index<0||index>7)return 0;if(ptr)std::snprintf(static_cast<char*>(ptr),24,"%s",programs[index]);return 1;
case op::getParamName:if(ptr&&index>=0&&index<Parameters)std::snprintf(static_cast<char*>(ptr),8,"%s",labels[index]);return 1;
case op::getParamLabel:if(ptr)*static_cast<char*>(ptr)=0;return 1;
case op::getParamDisplay:if(ptr){float v=get(e,index);auto* t=static_cast<char*>(ptr);
    if(index==Amp)std::snprintf(t,8,"%s",amps[choice(v,3)]);
    else if(index==Cabinet)std::snprintf(t,8,"%s",cabs[choice(v,3)]);
    else if(index==Pickup)std::snprintf(t,8,"%s",v<.5f?"Humbuck":"Single");
    else if(index==Pedal1||index==Pedal2||index==Pedal3)std::snprintf(t,8,"%s",pedals[choice(v,9)]);
    else if(index==Time)std::snprintf(t,8,"%.0f ms",40+260*v);
    else if(index==Rate)std::snprintf(t,8,"%.2f Hz",.08+7*v*v);
    else std::snprintf(t,8,"%.0f%%",100*v);}return 1;
case op::getChunk:if(!ptr)return 0;s.chunk.program=s.program;for(int i=0;i<Parameters;++i)s.chunk.params[i]=get(e,i);*static_cast<void**>(ptr)=&s.chunk;return sizeof(s.chunk);
case op::setChunk:if(!ptr||value!=sizeof(s.chunk))return 0;{Guitar::Chunk c;std::memcpy(&c,ptr,sizeof(c));if(c.magic!=uid||c.version!=1||c.program<0||c.program>7)return 0;for(float v:c.params)if(!std::isfinite(v)||v<0||v>1)return 0;for(int i=0;i<Parameters;++i)set(e,i,c.params[i]);s.program=c.program;}return 1;
case op::getEffectName:case op::getProductString:if(ptr)std::snprintf(static_cast<char*>(ptr),32,"Tape Workshop Electric");return 1;
case op::getVendorString:if(ptr)std::snprintf(static_cast<char*>(ptr),32,"Tape Workshop");return 1;
case op::getVendorVersion:return 100;case op::getVstVersion:return 2400;
default:return s.core->dispatcher(s.core,opcode,index,value,ptr,opt);
}}
}
extern "C" __attribute__((visibility("default"))) vst2::Effect* VSTPluginMain(vst2::HostCallback){
    auto* s=new(std::nothrow) electric::Guitar;if(!s)return nullptr;if(!s->core){delete s;return nullptr;}s->preset(0);auto& e=s->effect;e.magic=vst2::magic;e.dispatcher=electric::dispatch;e.setParameter=electric::set;e.getParameter=electric::get;e.numPrograms=8;e.numParams=electric::Parameters;e.numOutputs=2;e.flags=flag::canReplace|flag::isSynth|flag::programChunks;e.uniqueId=int32_t(electric::uid);e.version=100;e.processReplacing=electric::render;e.object=s;return &e;
}
