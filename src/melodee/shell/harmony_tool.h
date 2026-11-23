// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/model/phrase_line.h"
namespace melodee {
class Context;
struct HarmonySetup {
    bool used=false,follow_project=true;
    uint8_t scale_choice=0;
    uint8_t mode=0,bars=4,wav_track=0,wav_pad=0,midi_track=0,midi_pattern=0,midi_pad=0,midi_read=0;
    harmony::Settings settings;harmony::OutputSettings parts;
};
struct HarmonyTool {
    bool open=false,stale=false,follow_project=true,confirm=false,busy=false,midi_source_set=false,midi_import_ready=false,options=false;
    uint8_t step=0,choice=0,scale_choice=0;
    uint8_t mode=0,row=13,target=0,source=0,pattern=0,bars=4,wav_track=0,wav_pad=0;
    uint8_t confirm_track=0,confirm_pattern=0,confirm_pick=0,percent=0;
    uint8_t midi_track=0,midi_pattern=0,midi_pad=0,midi_read=0;
    uint16_t selected=0,audio_tempo=0,midi_notes=0;
    uint32_t field_changed_at=0,result_length=0,audio_bar=0;
    bool auto_key_job=false,wav_key_valid=false,wav_key_minor=false;
    uint8_t wav_key_root=0,key_job_track=0,key_job_pad=0;
    uint64_t job=0;
    std::string audio_name;
    harmony::Settings settings;harmony::OutputSettings parts;harmony::Result result;
    std::array<harmony::Note,512> rhythm_preview{};
    uint16_t rhythm_preview_count=0,rhythm_preview_next=0,rhythm_preview_tempo=1200;
    uint32_t rhythm_preview_start=0;
    std::array<bool,512> rhythm_preview_active{};
    std::array<uint8_t,8> preview{};uint8_t preview_count=0,preview_track=0;uint32_t preview_until=0;
};
inline HarmonySetup harmony_setup(const HarmonyTool& h){HarmonySetup s;s.used=true;s.scale_choice=h.scale_choice;s.follow_project=h.follow_project;s.mode=h.mode;s.bars=h.bars;s.wav_track=h.wav_track;s.wav_pad=h.wav_pad;s.midi_track=h.midi_track;s.midi_pattern=h.midi_pattern;s.midi_pad=h.midi_pad;s.midi_read=h.midi_read;s.settings=h.settings;s.parts=h.parts;return s;}
inline void restore_harmony_setup(HarmonyTool& h,const HarmonySetup& s){h.scale_choice=s.scale_choice;h.follow_project=s.follow_project;h.mode=s.mode;h.bars=s.bars;h.wav_track=s.wav_track;h.wav_pad=s.wav_pad;h.midi_track=s.midi_track;h.midi_pattern=s.midi_pattern;h.midi_pad=s.midi_pad;h.midi_read=s.midi_read;h.midi_source_set=true;h.settings=s.settings;h.parts=s.parts;h.parts.genre=h.settings.genre;}
inline void vary_harmony_setup(HarmonyTool& h){h.settings.variation=uint16_t((h.settings.variation+1)%1000);if(h.parts.output>=2){auto& p=h.parts.output==2?h.parts.pluck:h.parts.melody;p.variation=uint16_t((p.variation+1)%1000);}h.stale=true;}
inline bool harmony_edits_melody(const HarmonyTool& h){return h.parts.output==3||h.parts.output==4;}
inline const harmony::PhraseSettings& harmony_phrase(const HarmonyTool& h){return harmony_edits_melody(h)?h.parts.melody:h.parts.pluck;}
inline harmony::PhraseSettings& harmony_phrase(HarmonyTool& h){return harmony_edits_melody(h)?h.parts.melody:h.parts.pluck;}
struct HarmonyRows {std::array<uint8_t,34> at{};unsigned count=0;};
inline HarmonyRows harmony_rows(const HarmonyTool& h){
    HarmonyRows rows;auto add=[&](unsigned n){rows.at[rows.count++]=uint8_t(n);};
    if(h.mode){add(0);add(1);}for(unsigned n=2;n<=5;++n)add(n);add(6);add(33);if(h.parts.output==0)add(7);
    add(8);add(9);add(10);if(h.mode!=2)add(11);if(h.mode==1)add(12);add(13);
    if(h.mode==0)add(14);else if(h.mode==2){add(14);add(15);add(16);}else {add(26);add(27);add(28);if(h.midi_read)add(29);}
    add(17);add(30);if(h.parts.output==1){add(18);add(19);add(20);}else if(h.parts.output>=2)for(unsigned n=21;n<=25;++n)if(n!=21||(!harmony::output_rhythm(h.parts)&&!h.settings.genre))add(n);
    return rows;
}
// Guided pages use only the fields needed for the current choice.
inline HarmonyRows harmony_step_rows(const HarmonyTool& h) {
    HarmonyRows rows;
    auto add = [&](unsigned n) { rows.at[rows.count++] = uint8_t(n); };
    if (h.step == 0) {
        add(13);
        if (h.mode == 0) add(14);
        else if (h.mode == 2) { add(15); add(16); }
        else { add(26); add(27); add(28); if (h.midi_read) add(29); }
    } else if(h.step==1){add(17);add(6);add(33);add(30);if(h.parts.output==1)add(19);}
    else add(h.step == 2 ? 10 : h.step == 3 ? 31 : 32);
    return rows;
}
inline void harmony_step_begin(HarmonyTool& h, unsigned step) {
    h.step = uint8_t(std::min(step, 4u));
    h.choice = 0;
    h.row = harmony_step_rows(h).at[0];
}
inline bool harmony_detect_on_confirm(const HarmonyTool& h) { return h.step==0 && h.mode==2 && h.row==16; }
inline bool harmony_step_forward(HarmonyTool& h) {
    auto rows = harmony_step_rows(h);
    for (unsigned n = 0; n + 1 < rows.count; ++n)
        if (rows.at[n] == h.row) { h.row = rows.at[n + 1]; return false; }
    if (h.step < 4) harmony_step_begin(h, h.step + 1);
    return true;
}
inline void harmony_step_back(HarmonyTool& h) {
    auto rows = harmony_step_rows(h);
    for (unsigned n = 1; n < rows.count; ++n)
        if (rows.at[n] == h.row) { h.row = rows.at[n - 1]; return; }
    if (h.step) harmony_step_begin(h, h.step - 1);
}
void harmony_stop_preview(Context& c);
void harmony_open(Context& c);
void harmony_generate(Context& c);
void harmony_again(Context& c,bool from_roll);
void harmony_pot(Context& c,unsigned knob,int value);
void harmony_wheel(Context& c,int delta);
void harmony_soft(Context& c,int key);
void harmony_preview(Context& c);
void harmony_tick(Context& c);
}
