// SPDX-License-Identifier: AGPL-3.0-only
// Original DSP; host ABI from Melodee by Kerem Kiliç (Ellic Studio).
#include "melodee/plugin/vst2_abi.h"
#include "melodee/plugin/melody_pattern.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <vector>
#ifdef TW_ORCHESTRA
#include "melodee/plugin/orchestra_info.h"
#endif
using namespace vst2;
namespace {
#ifdef TW_ORCHESTRA
constexpr const char* instrumentName="Tape Workshop Orchestra";
constexpr const char* bankFile="/orchestra.bank";
constexpr uint32_t pluginId=0x54574f31;
constexpr unsigned maximumGroup=33,voiceCount=12,maximumZones=768;
constexpr uint32_t stateVersion=2;constexpr int binaryVersion=201;
#else
constexpr const char* instrumentName="Tape Workshop Strings";
constexpr const char* bankFile="/violin.bank";
constexpr uint32_t pluginId=0x54575631;
constexpr unsigned maximumGroup=7,voiceCount=16,maximumZones=512;
constexpr uint32_t stateVersion=4;constexpr int binaryVersion=401;
#endif
// +9.54 dB makeup before the existing soft ceiling; Output and saved states retain their ranges.
constexpr float outputMakeup = 3.f;
constexpr float pi = 3.14159265358979323846f;
enum Parameter { Articulation, Violin, Viola, Cello, Bass, Ensemble, Detune, Width,
    Attack, Decay, Sustain, Release, Cutoff, Resonance, Vibrato, VibRate, Expression,
    Reverb, Tone, Output, MotionRate, MotionDepth, MotionShape, MotionTarget, RoomEcho, EchoTime, EchoFeedback, Legato, GlideTime, GlideRange,
#ifdef TW_ORCHESTRA
    StringPlayer, BrassPlayer, WindPlayer, PercPlayer, Dynamics,
#endif
    MelodyPlay, MelodyVariation, MelodyKey, MelodyScale, MelodySpeed, MelodyOctave, MelodyRange,
    MelodySteps, MelodyGate, MelodySwing, MelodyDensity, MelodyVelocity, MelodyFollowProject, Count };
constexpr const char* names[] = {"Articulation","Violin","Viola","Cello","Bass","Ensemble","Detune","Width",
    "Attack","Decay","Sustain","Release","Cutoff","Resonance","Vibrato","Vib Rate","Expression","Reverb","Tone","Output","MoveRate","MoveDepth","MoveShape","MoveTo","Rm Echo","Echo ms","Echo FB","Legato","Glide","MaxGlid"
#ifdef TW_ORCHESTRA
    ,"String","Brass","Wind","Perc","Dyn"
#endif
    ,"MelPlay","NewMel","Key","Scale","Speed","Octave","Range","Steps","Gate","Swing","Density","MelVel","ProjKey"
};
constexpr float defaults[] = {0,1,0,0,0,0,.25f,.65f,.12f,.2f,1,.25f,.85f,.1f,0,.45f,1,.2f,.5f,.6f,.25f,0,0,0,0,.28f,.35f,0,.2f,.5f
#ifdef TW_ORCHESTRA
    ,0,0,0,0,.65f
#endif
    ,0,0,0,1.f/12,1.f/3,2.f/3,0,.5f,.65f,0,.9f,.8f,1
};
#ifdef TW_ORCHESTRA
constexpr auto& programs=melodee::orchestra_programs;
constexpr int programCount=int(programs.size());
#else
constexpr const char* programs[] = {"Solo Violin","Warm Viola","Cello Legato","Deep Contrabass","Chamber Strings","Full Strings","Soft String Pad","Pizzicato Ensemble","Tremolo Strings","Breathing Strings",
    "Soft Solo Violin","Bright Solo Violin","Dark Solo Violin","Warm Chamber Violin",
    "Wide Violins","Expressive Violin","Slow Swell Violin","Tremolo Violin",
    "Pizzicato Violin","Echo Violin","Cinematic Violins","Intimate Violin","Solo Legato Violin","Portamento Violin"};
constexpr int programCount = int(sizeof(programs)/sizeof(programs[0]));
#endif
static_assert(sizeof(defaults)/sizeof(defaults[0])==Count);
struct Zone { uint32_t group,root,layer,rate,offset,frames; int32_t tune; float gain; };
struct Bank {
    std::vector<Zone> zones;
    std::vector<int16_t> pcm;
    explicit Bank(const std::string& file) {
        std::ifstream in(file,std::ios::binary); char magic[4]{}; uint32_t count=0,frames=0;
        in.read(magic,4); in.read(reinterpret_cast<char*>(&count),4); in.read(reinterpret_cast<char*>(&frames),4);
        if(!in || std::memcmp(magic,"TWV2",4) || count>maximumZones || !count || frames>100*1024*1024) return;
        zones.resize(count); pcm.resize(frames);
        in.read(reinterpret_cast<char*>(zones.data()),count*sizeof(Zone));
        in.read(reinterpret_cast<char*>(pcm.data()),frames*sizeof(int16_t));
        if(!in) { zones.clear(); pcm.clear(); return; }
        for(const auto& z:zones) if(z.group>maximumGroup || z.root>127 || z.layer>1 || z.rate<8000 || z.rate>192000 || z.frames<8 || !std::isfinite(z.gain) || z.gain<=0 || z.gain>100 || std::abs(z.tune)>1200 || uint64_t(z.offset)+z.frames>pcm.size()) {
            zones.clear(); pcm.clear(); return;
        }
    }
};
// Share immutable samples across instances; this lock and file read occur only at creation.
std::shared_ptr<const Bank> loadBank() {
    static std::mutex lock; static std::weak_ptr<const Bank> cached;
    std::lock_guard<std::mutex> guard(lock);
    if(auto bank=cached.lock()) return bank;
    Dl_info info{}; std::string dir=".";
    if(dladdr(reinterpret_cast<void*>(&loadBank),&info) && info.dli_fname) {
        std::string path=info.dli_fname; auto slash=path.find_last_of('/');
        if(slash!=std::string::npos) dir=path.substr(0,slash);
    }
    auto bank=std::make_shared<Bank>(dir+bankFile); cached=bank; return bank;
}
struct Part { const Zone* zone=nullptr; double position=0,step=1; float pan=0;bool oneShot=false; };
struct Voice { bool active=false,held=false; int note=0,channel=0,stage=0; unsigned age=0;
    float env=0,releaseStep=0,velocity=0,low[2]{},band[2]{}; bool pluck=false,linked=false,generated=false;
    float pitch=0,targetPitch=0,basePitch=0,glideDistance=0;
    std::array<Part,8> parts{};
};
struct Instrument {
    Effect effect{};HostCallback host=nullptr;
    melodee::melody::Clock melodyClock;bool melodyBlocked=false;int64_t blockedMelodyStep=-1;
 std::shared_ptr<const Bank> bank=loadBank();
    std::array<std::atomic<float>,Count> values{}; std::array<float,Count> smooth{};
    std::array<Voice,voiceCount> voices{}; std::array<bool,16> pedal{};
    std::array<float,16> bend{},mod{},expression{};
    std::array<std::array<uint8_t,128>,16> heldVelocity{};
    std::array<std::array<unsigned,128>,16> heldOrder{};
    std::array<MidiEvent,512> events{}; unsigned count=0,serial=0,position=0;
    std::array<std::array<float,16384>,2> room{}; std::array<float,2048> sine{};
    std::array<std::array<float,262144>,2> echo{};std::array<float,2> echoTone{};
    double rate=48000,phase=0,motionPhase=0; std::atomic<int> program{0};
    struct Chunk { uint32_t magic=pluginId,version=stateVersion; int32_t preset=0; float params[Count]{}; } chunk;
    Instrument() { for(int n=0;n<Count;++n) values[n]=smooth[n]=defaults[n]; expression.fill(1);
        for(unsigned i=0;i<sine.size();++i) sine[i]=std::sin(2*pi*i/sine.size()); }
    float sin(double p) const { p-=std::floor(p); return sine[unsigned(p*2048)&2047]; }
    void preset(int n) {
#ifdef TW_ORCHESTRA
#include "../tape-workshop-orchestra/presets.inc"
#else
        n=std::clamp(n,0,programCount-1); for(int i=0;i<Count;++i) values[i]=defaults[i];
        for(int i=Violin;i<=Bass;++i) values[i]=0;
        if(n<4) values[Violin+n]=1;
        else { values[Violin]=.85f; values[Viola]=.6f; values[Cello]=.6f; values[Bass]=n==4?.25f:.5f;
            values[Ensemble]=n==4?0:1; values[Reverb]=n==6?.5f:.28f; }
        if(n==6) { values[Attack]=.55f; values[Release]=.6f; values[Cutoff]=.55f; values[MotionDepth]=.35f; values[MotionRate]=.08f; }
        if(n==7) { values[Articulation]=1; values[Attack]=0; values[Release]=.1f; }
        if(n==8){values[MotionDepth]=.8f;values[MotionRate]=.75f;}
        if(n==9){values[MotionDepth]=.65f;values[MotionRate]=.1f;values[Attack]=.4f;}
        if(n>=10) {
            // These are sound-design variations of the acoustic recordings, not new sample libraries.
            values[Violin]=1;values[Viola]=values[Cello]=values[Bass]=0;
            values[Ensemble]=0;values[Reverb]=.2f;
            switch(n) {
                case 10: values[Cutoff]=.57f;values[Tone]=.2f;values[Attack]=.2f;values[Output]=.5f;break;
                case 11: values[Cutoff]=1;values[Tone]=.9f;values[Attack]=.04f;values[Reverb]=.12f;break;
                case 12: values[Cutoff]=.35f;values[Tone]=.05f;values[Release]=.4f;break;
                case 13: values[Viola]=.3f;values[Cutoff]=.65f;values[Reverb]=.3f;break;
                case 14: values[Ensemble]=1;values[Detune]=.7f;values[Width]=1;values[Reverb]=.32f;break;
                case 15: values[Vibrato]=.25f;values[VibRate]=.55f;values[Attack]=.08f;values[Release]=.32f;break;
                case 16: values[Attack]=.65f;values[Release]=.65f;values[MotionRate]=.07f;values[MotionDepth]=.5f;break;
                case 17: values[MotionRate]=.82f;values[MotionDepth]=.85f;values[Attack]=.05f;break;
                case 18: values[Articulation]=1;values[Attack]=0;values[Release]=.08f;values[Reverb]=.15f;break;
                case 19: values[RoomEcho]=.3f;values[EchoTime]=.4f;values[EchoFeedback]=.5f;values[Reverb]=.12f;break;
                case 20: values[Viola]=.25f;values[Ensemble]=1;values[Width]=1;values[Attack]=.4f;values[Release]=.6f;values[Reverb]=.5f;values[MotionDepth]=.2f;values[MotionTarget]=1.f/3;values[MotionRate]=.12f;break;
                case 22: values[Legato]=1;values[GlideTime]=.08f;values[GlideRange]=.5f;values[Attack]=.08f;break;
                case 23: values[Legato]=1;values[GlideTime]=.4f;values[GlideRange]=1;values[Vibrato]=.15f;break;
                case 21: values[Width]=.1f;values[Reverb]=.04f;values[Cutoff]=.7f;values[Tone]=.3f;break;
            }
        }
        program=n;
#endif
    }
    void off(Voice& v) { v.held=false; v.stage=3;
        v.releaseStep=std::max(.000001f,v.env/float((.015+2.5*values[Release].load()*values[Release].load())*rate)); }
    void retarget(Voice& v,unsigned key,unsigned velocity) {
        v.note=int(key);v.targetPitch=float(key);v.glideDistance=std::abs(v.targetPitch-v.pitch);v.velocity=velocity/127.f;v.held=true;
        const int limit=int(std::lround(values[GlideRange]*24));
        if(values[GlideTime]<=0 || std::abs(v.targetPitch-v.pitch)>limit)v.pitch=v.targetPitch;
    }
    void midi(const MidiEvent& e,bool generated=false) {
        unsigned type=uint8_t(e.data[0])&240,ch=uint8_t(e.data[0])&15;
        unsigned key=uint8_t(e.data[1])&127,vel=uint8_t(e.data[2])&127;
        if(type==0xe0) { bend[ch]=(int(key)+128*int(vel)-8192)/8192.f; return; }
        if(type==0xb0) {
            if(key==1) {mod[ch]=vel/127.f;
#ifdef TW_ORCHESTRA
                values[Dynamics]=mod[ch];
#endif
            }
            if(key==11) expression[ch]=vel/127.f;
            if(key==64) { pedal[ch]=vel>=64; if(!pedal[ch]) for(auto& v:voices) if(v.active&&v.channel==int(ch)&&!v.held) off(v); }
            if(key==120||key==123) {
                stopMelody(key==120);if(key==120){melodyBlocked=true;blockedMelodyStep=melodyClock.last;}
                heldVelocity[ch].fill(0);heldOrder[ch].fill(0);
                for(auto& v:voices) if(v.channel==int(ch)) { if(key==120) v={}; else off(v); }
            }
            return;
        }
        if(type==0x80||(type==0x90&&!vel)) {
            heldVelocity[ch][key]=0;heldOrder[ch][key]=0;
            for(auto& v:voices) if(v.active&&v.note==int(key)&&v.channel==int(ch)&&v.generated==generated) {
                int fallback=-1;unsigned latest=0;
                if(v.linked&&values[Legato]>.5f)for(unsigned n=0;n<128;++n)
                    if(heldVelocity[ch][n]&&heldOrder[ch][n]>=latest){fallback=int(n);latest=heldOrder[ch][n];}
                if(fallback>=0)retarget(v,unsigned(fallback),heldVelocity[ch][unsigned(fallback)]);
                else {v.held=false;if(!pedal[ch])off(v);}
            }
            return;
        }
        if(type!=0x90 || bank->zones.empty()) return;
        if(!generated){heldVelocity[ch][key]=uint8_t(vel);heldOrder[ch][key]=++serial;}else ++serial;
        if(!generated&&values[Legato]>.5f&&values[Articulation]<.5f) {
            Voice* linked=nullptr;
            for(auto& v:voices)if(v.active&&v.held&&!v.generated&&!v.pluck&&v.channel==int(ch)&&(!linked||v.age>linked->age))linked=&v;
            if(linked){linked->linked=true;linked->age=serial;retarget(*linked,key,vel);return;}
        }
        Voice* take=&voices[0]; for(auto& v:voices) {if(!v.active){take=&v;break;}if(v.age<take->age)take=&v;}
        *take={}; auto& v=*take; v.active=v.held=true;v.age=serial;v.note=key;v.pitch=v.targetPitch=v.basePitch=float(key);v.linked=values[Legato]>.5f;v.channel=ch;v.velocity=vel/127.f;v.pluck=values[Articulation]>.5f;v.generated=generated;
        for(unsigned group=0;group<4;++group) for(unsigned layer=0;layer<2;++layer) {
            const Zone* best=nullptr;int score=10000;
            unsigned zoneGroup=group*2+unsigned(v.pluck);
#ifdef TW_ORCHESTRA
            zoneGroup=melodee::orchestra_player(group,values[StringPlayer+group])*2+unsigned(v.pluck);
            int lowest=128,highest=-1;
            for(const auto& z:bank->zones)if(z.group==zoneGroup){lowest=std::min(lowest,int(z.root));highest=std::max(highest,int(z.root));}
            if(int(key)<lowest-3||int(key)>highest+3)continue;
#endif
            for(const auto& z:bank->zones) if(z.group==zoneGroup) {
                int s=std::abs(int(z.root)-int(key))*4+std::abs(int(z.layer)-int(layer));
                if(s<score){score=s;best=&z;}
            }
            auto& part=v.parts[group*2+layer];part.zone=best;
#ifdef TW_ORCHESTRA
            part.oneShot=group==3;
#endif
            if(best) part.step=double(best->rate)/rate*std::pow(2.,(int(key)-int(best->root))/12.+best->tune/1200.);
            part.pan=(float(group)-1.5f)*.3f+(layer?.14f:-.14f);
        }
    }
    void stopMelody(bool immediate=false){for(auto& v:voices)if(v.active&&v.generated){if(immediate)v={};else off(v);}}
    void melodyNote(int key,unsigned velocity){MidiEvent event{};event.data[0]=char(0x90);event.data[1]=char(key);event.data[2]=char(velocity);midi(event,true);}
    float sample(Part& part,double speed,bool pluck) const {
        if(!part.zone) return 0;const auto& z=*part.zone;
        // A 180 ms crossfade repeats a steady bowed portion while a key is held.
        double loopStart=z.frames*.36,loopEnd=z.frames*.80;
        double cross=std::min(double(z.rate)*.18,(loopEnd-loopStart)*.2);
        auto read=[&](double at) { unsigned i=unsigned(std::clamp(at,0.,double(z.frames-2)));
            float f=float(at-i);return (bank->pcm[z.offset+i]*(1-f)+bank->pcm[z.offset+i+1]*f)/32768.f; };
        if(pluck && part.position>=z.frames-2) return 0;
        float x=read(part.position);
        if(!pluck&&part.position>=loopEnd-cross) {float w=float((part.position-(loopEnd-cross))/cross);w=std::clamp(w,0.f,1.f);x=x*(1-w)+read(loopStart+part.position-(loopEnd-cross))*w;}
        part.position+=part.step*speed;
        if(!pluck&&part.position>=loopEnd) part.position=loopStart+cross+std::fmod(part.position-loopEnd,loopEnd-loopStart-cross);
        return x*z.gain;
    }
};
Instrument& self(Effect* e){return *static_cast<Instrument*>(e->object);}
float get(Effect* e,int32_t n){return n>=0&&n<Count?self(e).values[n].load():0;}
void set(Effect* e,int32_t n,float v){if(n>=0&&n<Count&&std::isfinite(v)) self(e).values[n]=std::clamp(v,0.f,1.f);}
void render(Effect* e,float**,float** out,int32_t frames) {
    if(frames<=0||!out||!out[0]||!out[1])return;auto& s=self(e);
    std::array<float,Count> target{};for(int n=0;n<Count;++n)target[n]=s.values[n];
    std::array<float,melodee::melody::parameters> melodyValues{};
    for(unsigned n=0;n<melodyValues.size();++n)melodyValues[n]=target[MelodyPlay+n];
    auto melodySettings=melodee::melody::settings(melodyValues);
    const TimeInfo* time=s.host?reinterpret_cast<const TimeInfo*>(s.host(e,host::getTime,0,time::ppqValid|time::tempoValid,nullptr,0)):nullptr;
    bool playing=time&&(time->flags&time::playing)&&(time->flags&time::ppqValid);
    double ppq=time?time->ppqPos:0,tempo=time&&std::isfinite(time->tempo)?std::clamp(time->tempo,10.,400.):120.;
    const double increment=tempo/(60*s.rate);
    if(!playing||!melodySettings.enabled){s.melodyBlocked=false;s.blockedMelodyStep=-1;}
    unsigned event=0;
    for(int i=0;i<frames;++i){while(event<s.count&&s.events[event].deltaFrames<=i)s.midi(s.events[event++]);
        if(s.melodyBlocked){
            const auto step=int64_t(std::floor((ppq+i*increment)/melodee::melody::beats(melodySettings)));
            if(s.blockedMelodyStep<0)s.blockedMelodyStep=step;
            else if(step!=s.blockedMelodyStep)s.melodyBlocked=false;
        }
        s.melodyClock.tick(melodySettings,playing&&!s.melodyBlocked,ppq+i*increment,increment,
            [&](int key,unsigned velocity){s.melodyNote(key,velocity);},[&]{s.stopMelody();});
        for(int n=0;n<Count;++n)s.smooth[n]+=.002f*(target[n]-s.smooth[n]);const auto& p=s.smooth;
        s.phase+=(3+5*p[VibRate])/s.rate;s.phase-=std::floor(s.phase);float sum[2]{};
        s.motionPhase+=(.05+10*p[MotionRate]*p[MotionRate])/s.rate;s.motionPhase-=std::floor(s.motionPhase);
        float motion=p[MotionShape]<.5f?s.sin(s.motionPhase):1-4*std::abs(float(s.motionPhase)-.5f);
        int destination=int(std::lround(p[MotionTarget]*3));
        float attack=1/float((.003+2*p[Attack]*p[Attack])*s.rate),decay=1/float((.02+2*p[Decay])*s.rate);
        float movingCutoff=std::clamp(p[Cutoff]+(destination==1?motion*p[MotionDepth]*.35f:0),0.f,1.f);
        float hz=100*std::pow(150.f,movingCutoff);float g=std::tan(pi*std::min(hz,float(s.rate*.4))/float(s.rate));
        float damp=1.9f-1.5f*p[Resonance];
        for(auto& v:s.voices){if(!v.active)continue;
            if(v.stage==0){v.env=std::min(1.f,v.env+attack);if(v.env==1)v.stage=1;}
            else if(v.stage==1){v.env=std::max(p[Sustain],v.env-decay);if(v.env<=p[Sustain])v.stage=2;}
            else if(v.stage==2)v.env=p[Sustain];else v.env=std::max(0.f,v.env-v.releaseStep);
            bool percussionTail=false;
#ifdef TW_ORCHESTRA
            if(p[Bass]>.0001f)for(unsigned part=6;part<8;++part)
                if(v.parts[part].zone&&v.parts[part].position<v.parts[part].zone->frames-2)percussionTail=true;
#endif
            if(v.stage==3&&v.env==0&&!percussionTail){v.active=false;continue;}
            if(target[Legato]<.5f)v.linked=false;
            const float difference=v.targetPitch-v.pitch;
            const float step=v.glideDistance/float(std::max(1.,double(p[GlideTime])*s.rate));
            v.pitch+=std::clamp(difference,-step,step);
            float dynamicGain=1;
#ifdef TW_ORCHESTRA
            dynamicGain=.2f+.8f*p[Dynamics];
#endif
            float vibratoDepth=p[Vibrato];
#ifndef TW_ORCHESTRA
            vibratoDepth+=s.mod[v.channel];
#endif
            float vibrato=s.sin(s.phase+v.age*.13)*vibratoDepth*.4f;
            double speed=std::pow(2.,(v.pitch-v.basePitch+s.bend[v.channel]*2+vibrato+(destination==3?motion*p[MotionDepth]*.8f:0))/12.);float signal[2]{};bool ended=v.pluck;
#ifdef TW_ORCHESTRA
            ended=true;
#endif
            for(unsigned group=0;group<4;++group){float level=p[Violin+group];
                for(unsigned layer=0;layer<2;++layer){auto& part=v.parts[group*2+layer];
#ifdef TW_ORCHESTRA
                    if(level>.0001f&&part.zone&&((!v.pluck&&!part.oneShot)||part.position<part.zone->frames-2))ended=false;
#else
                    if(part.zone&&part.position<part.zone->frames-2)ended=false;
#endif
                    float blend=std::clamp((v.velocity-.2f)/.65f,0.f,1.f);
#ifdef TW_ORCHESTRA
                    blend=p[Dynamics];
#endif
                    float weight=layer?blend:1-blend;
                    double det=std::pow(2.,(layer?1:-1)*p[Detune]*p[Ensemble]*.18/12.);
                    float x=s.sample(part,speed*det,v.pluck||part.oneShot)*weight*level;
#ifdef TW_ORCHESTRA
                    x*=part.oneShot?1.f:v.env;
#endif
                    float pan=std::clamp(part.pan*p[Width]+(destination==2?motion*p[MotionDepth]*.8f:0),-.95f,.95f);signal[0]+=x*(1-pan);signal[1]+=x*(1+pan);
                }
            }
            if(ended){v.active=false;continue;}
            float ampEnvelope=v.env;
#ifdef TW_ORCHESTRA
            ampEnvelope=1;
#endif
            for(unsigned ch=0;ch<2;++ch){float a1=1/(1+g*(g+damp));float band=a1*(v.band[ch]+g*(signal[ch]-v.low[ch]));float low=v.low[ch]+g*band;
                v.band[ch]=2*band-v.band[ch];v.low[ch]=2*low-v.low[ch];float high=signal[ch]-damp*band-low;
                float x=low+p[Tone]*.25f*high;sum[ch]+=x*ampEnvelope*v.velocity*p[Expression]*s.expression[v.channel]*.16f*dynamicGain*(destination==0?1-p[MotionDepth]*(.5f+.5f*motion):1);}
        }
        for(unsigned ch=0;ch<2;++ch){unsigned at=s.position&16383,delay=ch?7919:6113;float wet=s.room[ch][(s.position-delay)&16383];s.room[ch][at]=sum[ch]+wet*.65f;
            float delayFrames=float((.04+.76*p[EchoTime]+ch*.013)*s.rate);
            unsigned whole=unsigned(delayFrames);float fraction=delayFrames-whole;
            float repeat=s.echo[ch][(s.position-whole)&262143]*(1-fraction)
                +s.echo[ch][(s.position-whole-1)&262143]*fraction;
            s.echoTone[ch]+=.35f*(repeat-s.echoTone[ch]);
            s.echo[ch][s.position&262143]=sum[ch]+s.echoTone[ch]*p[EchoFeedback]*.78f;
            out[ch][i]=std::tanh((sum[ch]+wet*p[Reverb]+repeat*p[RoomEcho]) *p[Output]*(2*outputMakeup));}
        ++s.position;
    }
    s.count=0;
}
void text(void* p,const char* t){if(p){std::strncpy(static_cast<char*>(p),t,63);static_cast<char*>(p)[63]=0;}}
intptr_t dispatch(Effect* e,int32_t op,int32_t index,intptr_t value,void* ptr,float opt){auto& s=self(e);switch(op){
case op::open:return 1;case op::close:delete &s;return 1;
case op::setSampleRate:if(std::isfinite(opt)&&opt>=8000&&opt<=192000){s.rate=opt;for(auto& v:s.voices)v={};}return 1;
case op::mainsChanged:if(!value){s.melodyClock.stop([&]{s.stopMelody(true);});s.melodyBlocked=false;s.blockedMelodyStep=-1;for(auto& v:s.voices)v={};s.count=0;s.pedal.fill(false);s.bend.fill(0);s.mod.fill(0);s.expression.fill(1);for(auto& keys:s.heldVelocity)keys.fill(0);for(auto& keys:s.heldOrder)keys.fill(0);}return 1;
case op::setProgram:s.preset(int(value));return 1;case op::getProgram:return s.program;
case op::getProgramName:if(ptr)std::snprintf(static_cast<char*>(ptr),24,"%s",programs[s.program]);return 1;
case op::getProgramNameIndexed:if(index<0||index>=programCount)return 0;if(ptr)std::snprintf(static_cast<char*>(ptr),24,"%s",programs[index]);return 1;
case op::getParamName:if(ptr&&index>=0&&index<Count)std::snprintf(static_cast<char*>(ptr),8,"%s",names[index]);return 1;
case op::getParamDisplay:if(ptr){auto* out=static_cast<char*>(ptr);float v=get(e,index);
    #ifdef TW_ORCHESTRA
    if(index>=StringPlayer&&index<=PercPlayer){auto family=unsigned(index-StringPlayer);std::snprintf(out,8,"%s",melodee::orchestra_instruments[melodee::orchestra_player(family,v)]);}
    else if(index==Articulation)std::snprintf(out,8,"%s",v>.5f?"Short":"Sustain");
    else
#endif
    if(index>=MelodyPlay){auto n=index-MelodyPlay;
        if(n==0||n==12)std::snprintf(out,8,"%s",v>.5f?"On":"Off");
        else if(n==1)std::snprintf(out,8,"%u",melodee::melody::choice(v,999));
        else if(n==2)std::snprintf(out,8,"%s",melodee::melody::keys[melodee::melody::choice(v,11)]);
        else if(n==3)std::snprintf(out,8,"%s",melodee::melody::scale_names[melodee::melody::choice(v,12)]);
        else if(n==4)std::snprintf(out,8,"%s",melodee::melody::speeds[melodee::melody::choice(v,3)]);
        else if(n==5)std::snprintf(out,8,"C%u",2+melodee::melody::choice(v,3));
        else if(n==6)std::snprintf(out,8,"%u oct",1+melodee::melody::choice(v,2));
        else if(n==7)std::snprintf(out,8,"%u",8u<<melodee::melody::choice(v,2));
        else if(n==8)std::snprintf(out,8,"%.0f%%",10+85*v);
        else if(n==9)std::snprintf(out,8,"%.0f%%",50+22.5f*v);
        else std::snprintf(out,8,"%.0f%%",100*v);
    }
    else if(index==Legato)std::snprintf(out,8,"%s",v>.5f?"On":"Off");
    else if(index==GlideTime)std::snprintf(out,8,"%.0f ms",v*1000);
    else if(index==GlideRange)std::snprintf(out,8,"%d semi",int(std::lround(v*24)));
    else if(index==Articulation)std::snprintf(out,8,"%s",v>.5f?"Plucked":"Bowed");
    else if(index==MotionTarget){const char* labels[]={"Volume","Filter","Pan","Pitch"};std::snprintf(out,8,"%s",labels[int(std::lround(v*3))]);}
    else if(index==MotionShape)std::snprintf(out,8,"%s",v>.5f?"Tri":"Sine");
    else if(index==EchoTime)std::snprintf(out,8,"%.0f ms",40+760*v);
    else if(index==MotionRate)std::snprintf(out,8,"%.2f Hz",.05f+10*v*v);
    else std::snprintf(out,8,"%.1f",v*100);
}return 1;
case op::getParamLabel:if(ptr)static_cast<char*>(ptr)[0]=0;return 1;
case op::getChunk:if(!ptr)return 0;s.chunk.preset=s.program;for(int n=0;n<Count;++n)s.chunk.params[n]=get(e,n);*static_cast<void**>(ptr)=&s.chunk;return sizeof(s.chunk);
case op::setChunk:if(!ptr||(value!=sizeof(s.chunk)&&value!=108&&value!=120&&value!=132&&value!=152))return 0;{
    Instrument::Chunk c;for(int n=0;n<Count;++n)c.params[n]=defaults[n];
    std::memcpy(&c,ptr,size_t(value));
    bool validVersion=c.version==stateVersion&&value==sizeof(c);
#ifndef TW_ORCHESTRA
    validVersion=validVersion||(c.version==1&&value==108)||(c.version==2&&value==120)||(c.version==3&&value==132);
#else
    validVersion=validVersion||(c.version==1&&value==152);
#endif
    if(c.magic!=pluginId||!validVersion)return 0;
    for(float v:c.params)if(!std::isfinite(v)||v<0||v>1)return 0;
    for(int n=0;n<Count;++n)set(e,n,c.params[n]);s.program=std::clamp(c.preset,0,programCount-1);
}return 1;
case op::processEvents:if(ptr){auto* es=static_cast<Events<512>*>(ptr);for(int n=0;n<std::clamp(es->count,0,512);++n){auto* m=es->events[n];if(!m||m->type!=midiEventType||m->byteSize<int(sizeof(MidiEvent)))continue;
    if(s.count==512){for(auto& v:s.voices)s.off(v);for(auto& keys:s.heldVelocity)keys.fill(0);for(auto& keys:s.heldOrder)keys.fill(0);s.count=0;}auto copy=*m;copy.deltaFrames=std::max(0,copy.deltaFrames);unsigned at=s.count++;while(at&&s.events[at-1].deltaFrames>copy.deltaFrames){s.events[at]=s.events[at-1];--at;}s.events[at]=copy;}}return 1;
case op::getEffectName:case op::getProductString:text(ptr,instrumentName);return 1;case op::getVendorString:text(ptr,"Tape Workshop");return 1;
case op::getVendorVersion:return binaryVersion;case op::getVstVersion:return 2400;case op::getTailSize:return int(s.rate*45);
case op::canDo:return ptr&&(!std::strcmp(static_cast<char*>(ptr),"receiveVstMidiEvent")||!std::strcmp(static_cast<char*>(ptr),"receiveVstEvents"))?1:0;
default:return 0;}}
}
extern "C" __attribute__((visibility("default"))) vst2::Effect* VSTPluginMain(vst2::HostCallback host){
    Instrument* s=nullptr;try{s=new Instrument;}catch(...){return nullptr;}
    if(s->bank->zones.empty()){delete s;return nullptr;}s->host=host;s->preset(0);auto& e=s->effect;e.magic=vst2::magic;e.dispatcher=dispatch;e.setParameter=set;e.getParameter=get;
    e.numPrograms=programCount;e.numParams=Count;e.numOutputs=2;e.flags=flag::canReplace|flag::isSynth|flag::programChunks;e.uniqueId=int32_t(pluginId);e.version=binaryVersion;e.processReplacing=render;e.object=s;return &e;
}
