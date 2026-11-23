// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop mixer; based on Melodee by Kerem Kiliç (Ellic Studio).
#include "melodee/pages/mixer_look.h"
#include "melodee/pages/pages.h"
#include "melodee/shell/mixer.h"
#include "melodee/shell/new_track.h"
#include "melodee/shell/pad_bank.h"
#include "melodee/shell/erase_copy.h"
namespace melodee {
namespace {
using namespace mixer_ui;
using namespace ui;
juce::String pad_id(unsigned pad){char name[4];pad_label(pad,name);return name;}
juce::String track_id(unsigned t){return "T"+juce::String(int(t+1)).paddedLeft('0',2);}
juce::String channel_name(const Snapshot& s,unsigned n,bool pad){
    auto name=pad?pad_name(s,n,false):juce::String::fromUTF8(s.project.tracks[n].name);
    if(name.isEmpty())name=pad?"EMPTY":engine_info(s.project.tracks[n].engine).name;
    return name_case(name).toUpperCase();
}
void strip(Paint& p,const Snapshot& s,unsigned at,bool pad,unsigned column){
    float x=6+column*61;bool selected=!s.ui.mixer_master&&(pad?at==s.track().last_pad:at==s.ui.track);
    const auto& t=pad?s.track():s.project.tracks[at];const auto& sound=t.pads[at%sampler_pads];
    int level=pad?sound.level:t.mix.level;bool mute=pad?pad_muted_now(t,s.pattern,s.positions[s.ui.track],at):t.mix.mute,solo=pad?t.pad_soloed(at):t.mix.solo;
    p.fill(x,73,57,111,selected?colour::raised:colour::panel,4);p.ring(x,73,57,111,selected?ink():colour::line,4);
    label(p,pad?pad_id(at):track_id(at),x+3,76,51,16,11,selected?ink():colour::mid,juce::Justification::centred);
    auto name=channel_name(s,at,pad);if(name.length()>7)name=name.substring(0,7);
    label(p,name,x+3,92,51,16,11,colour::text,juce::Justification::centred);
    float peak=pad?s.mixer_pad_meters[at%16]:s.meters[at];meter(p,x+8,112,5,39,peak);
    p.fill(x+27,112,2,39,colour::line,1);p.fill(x+19,149-std::clamp(level,0,127)/127.f*37,18,6,selected?ink():colour::mid,2);
    label(p,level_text(level,pad),x+15,148,39,14,11,colour::text,juce::Justification::centredRight);
    p.fill(x+4,163,22,17,mute?colour::rec:colour::quiet,2);label(p,"M",x+4,163,22,17,11,mute?colour::bg:colour::mid,juce::Justification::centred);
    p.fill(x+31,163,22,17,solo?colour::amber:colour::quiet,2);label(p,"S",x+31,163,22,17,11,solo?colour::bg:colour::mid,juce::Justification::centred);
    // The effect count stays visible above the level: no extra tiny labels.
    auto count=pad?s.project.chain(pad_chain(s.ui.track,at)).count():t.fx.count();
    if(count)label(p,"FX"+juce::String(count),x+15,111,40,13,11,ink(),juce::Justification::centredRight);
}
void master_strip(Paint& p,const Snapshot& s){
    float x=254; p.fill(x,73,60,111,colour::panel,4);p.ring(x,73,60,111,s.ui.mixer_master?ink():colour::line,4);
    label(p,"MASTER",x+3,78,54,17,11,colour::text,juce::Justification::centred);
    meter(p,x+9,106,6,46,s.mixer_master_meters[0]);meter(p,x+18,106,6,46,s.mixer_master_meters[1]);
    label(p,level_text(s.project.master_level),x+26,128,31,16,11,colour::text,juce::Justification::centredRight);
    label(p,"FX "+juce::String(s.project.master.count()),x+3,162,54,18,11,ink(),juce::Justification::centred);
}
void details(Paint& p,const Snapshot& s,bool master){
    bool pad=s.ui.mixer_view==MixerView::Pad;const auto& sound=s.track().pads[s.track().last_pad%sampler_pads];
    int level=master?s.project.master_level:pad?sound.level:s.track().mix.level;
    int pan=master?s.project.master_pan:pad?sound.pan:s.track().mix.pan;
    label(p,master?"ALL TRACKS > MASTER":channel_name(s,pad?s.track().last_pad:s.ui.track,pad),8,50,304,18,11,colour::mid);
    p.fill(8,75,204,107,colour::panel,4);p.ring(8,75,204,107,colour::line,4);
    label(p,master?"MASTER LEVEL":pad?"PAD LEVEL":"TRACK LEVEL",18,82,184,17,11,colour::mid);
    label(p,level_text(level,pad)+" dB",18,101,184,34,26);
    label(p,master?"BALANCE":"PAN",18,143,70,18,11,colour::mid);label(p,pan_text(pan),90,143,110,18,12,colour::text,juce::Justification::centredRight);
    p.fill(18,170,182,2,colour::line);p.fill(18+(pan+50)/100.f*178,166,4,10,ink(),2);
    p.fill(222,75,90,107,colour::panel,4);p.ring(222,75,90,107,colour::line,4);label(p,"OUTPUT",228,82,78,18,11,colour::text,juce::Justification::centred);
    float l=master?s.mixer_master_meters[0]:pad?s.mixer_pad_sides[s.track().last_pad%16][0]:s.meters[s.ui.track];
    float r=master?s.mixer_master_meters[1]:pad?s.mixer_pad_sides[s.track().last_pad%16][1]:l;
    if(master||pad){meter(p,244,106,10,48,l);meter(p,261,106,10,48,r);}else meter(p,254,106,12,48,l);
    unsigned n=master?master_chain:pad?pad_chain(s.ui.track,s.track().last_pad):s.ui.track;
    label(p,"FX "+juce::String(s.project.chain(n).count())+" >",228,160,78,18,11,ink(),juce::Justification::centred);
}
void menu(Paint& p,const Snapshot& s){
    bool pad=mixer_bank(s.ui);unsigned at=s.track().last_pad%sampler_pads;
    bool mute=pad?pad_muted_now(s.track(),s.pattern,s.positions[s.ui.track],at):s.track().mix.mute,solo=pad?s.track().pad_soloed(at):s.track().mix.solo;
    p.fill(24,68,272,114,colour::raised,5);p.ring(24,68,272,114,ink(),5);label(p,"MUTE / SOLO",36,74,248,20,13);
    const juce::String labels[]={pad?"Mute pad":"Mute track",pad?"Solo pad":"Solo track","Clear all solos"};
    for(unsigned i=0;i<3;++i){float y=101+i*25;if(i==s.ui.mixer_menu_row)p.fill(34,y,252,23,hex(0x42304F),2);label(p,labels[i],43,y,180,23,12);label(p,i==2?">":(i?solo:mute)?"ON":"OFF",228,y,48,23,12,teal(),juce::Justification::centredRight);}
}
void draw(Paint& p,const Snapshot& s){
    Look look;const auto& ui=s.ui;bool bank=mixer_bank(ui),master=ui.mixer_view==MixerView::Master;
    auto title=master?"MASTER MIXER":ui.mixer_view==MixerView::Pad?"PAD DETAILS":ui.mixer_view==MixerView::Track?"TRACK DETAILS":bank?"BANK MIXER":"TRACK MIXER";
    auto context=master?juce::String("PROJECT"):bank?track_id(ui.track)+" / "+pad_id(s.track().last_pad):juce::String(s.project.name);
    header(p,title,context,ui.mixer_menu?std::array<juce::String,3>{"BACK","SELECT","CONFIRM"}:std::array<juce::String,3>{"BACK",master?"MONO":"MUTE / SOLO","FX"});
    if(master||ui.mixer_view==MixerView::Pad||ui.mixer_view==MixerView::Track)details(p,s,master);
    else{
        unsigned selected=bank?s.track().last_pad%16:ui.track,first=selected/4*4;
        if(bank){for(unsigned n=0;n<8;++n){float x=8+n*26;p.fill(x,50,23,18,n==s.track().bank?ink():colour::panel,3);label(p,juce::String::charToString(char('A'+n)),x,50,23,18,11,n==s.track().bank?colour::bg:colour::mid,juce::Justification::centred);}label(p,juce::String(first+1)+"-"+juce::String(first+4),222,50,90,18,11,colour::mid,juce::Justification::centredRight);}
        else{label(p,"ALL 16 TRACKS",8,50,175,18,11,colour::mid);label(p,juce::String(first+1)+"-"+juce::String(first+4)+" / 16",192,50,120,18,11,colour::mid,juce::Justification::centredRight);}
        for(unsigned n=0;n<4;++n)strip(p,s,(bank?s.track().bank*16:0)+first+n,bank,n);master_strip(p,s);
    }
    if(ui.mixer_menu)menu(p,s);cards(p,s);
}
#ifndef MIXER_PREVIEW
void pad(Context& c,unsigned pad,uint8_t velocity,bool down){
    if(mixer_bank(c.ui)){c.ui.mixer_master=false;play_pad(c,pad,velocity,down);return;}
    if(!down)return;c.ui.mixer_master=false;c.select_track(pad%track_count);if(c.ui.erase)erase_tap(c);
}
void wheel(Context& c,int delta){
    if(c.ui.mixer_menu){c.ui.mixer_menu_row=uint8_t(std::clamp<int>(c.ui.mixer_menu_row+delta,0,2));return;}
    if(c.ui.mixer_view==MixerView::Master)return;
    int pick=int(mixer_pick(c.track(),c.ui));mixer_select(c,unsigned(std::clamp(pick+delta,0,16)));
}
void push(Context& c){
    if(c.ui.mixer_menu){mixer_action(c,c.ui.mixer_menu_row);return;}
    if(c.ui.shift){c.ui.shift_used=true;c.name_track(c.ui.track);return;}
    if(c.ui.mixer_view==MixerView::Tracks&&!c.ui.mixer_master&&!c.track().used()){offer_new_track(c);return;}
    if(!mixer_enter(c.ui,c.track()))mixer_open_fx(c);
}
void soft(Context& c,int key){
    if(c.ui.shift){if(key>0)start_track_pick(c.ui,key==1);return;}
    if(key==0){if(!mixer_back(c))c.back();return;}
    if(c.ui.mixer_menu){if(key==1)c.ui.mixer_menu_row=uint8_t((c.ui.mixer_menu_row+1)%3);else mixer_action(c,c.ui.mixer_menu_row);return;}
    if(key==2){mixer_open_fx(c);return;}
    if(c.ui.mixer_view==MixerView::Master){c.apply(Binding{Binding::Kind::MasterMono,0,0,1,int(c.project.master_mono)},!c.project.master_mono);return;}
    if(c.ui.mixer_master){c.notice("Master: press Data to open");return;}
    c.ui.mixer_menu=true;c.ui.mixer_menu_row=0;
}
#endif
}
#ifndef MIXER_PREVIEW
const PageSpec tracks_page {draw,pad,wheel,push,soft,wheel};
#endif
}
