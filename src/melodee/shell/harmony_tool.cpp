// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/shell/harmony_tool.h"
#include "melodee/shell/harmony_preview.h"
#include "melodee/shell/context.h"
#include "melodee/shell/roll.h"
#include "melodee/model/scales.h"
#include "melodee/model/chord_roll.h"
#include "melodee/model/midi_harmony_source.h"
#include "melodee/shell/sample_library.h"
#include "melodee/engine/engine_host.h"
#include <filesystem>
#include <cctype>
namespace melodee {
namespace {
void remember(Context& c){auto& h=c.ui.harmony;if(!h.result.count||h.stale)return;const auto saved=harmony_setup(h);c.ui.harmony_setups[h.source%track_count]=saved;c.ui.harmony_setups[h.target%track_count]=saved;}
void end_preview(Context& c){auto& h=c.ui.harmony;for(unsigned n=0;n<h.preview_count;++n)c.note(h.preview_track,h.preview[n],0);h.preview_count=0;stop_harmony_preview(h,[&](unsigned key){c.note(h.preview_track,key,0);});}

unsigned limit(unsigned row){if(row==33)return 13;if(row==6)return music::genre_count;if(row==30)return harmony::bass_rhythm_count-1;if(row>=31)return 3;const unsigned limits[]={4,100,1,1,1,1,7,4,49151,49151,15,999,1,2,1,15,127,4,3,3,100,3,3,100,999,3,15,127,1,127};return limits[std::min(row,29u)];}
int value(const HarmonyTool& h){if(h.row==33)return h.scale_choice;if(h.row==30)return harmony::output_rhythm(h.parts);if(h.row>=31)return h.choice;switch(h.row){case 0:return int(h.settings.follow);case 1:return h.settings.strength;case 2:case 3:case 4:return !!(h.settings.complexity&(1<<(h.row-2)));case 5:return h.settings.leading;case 6:return h.settings.genre;case 7:return h.result.count?h.result.chords[h.selected%h.result.count].inversion:0;case 8:return h.result.count?int(h.result.chords[h.selected%h.result.count].tick):0;case 9:return h.result.count?int(h.result.chords[h.selected%h.result.count].length):1;case 10:return h.target;case 11:return h.settings.variation;case 12:return h.follow_project;case 13:return h.mode;case 14:return h.bars==8;case 15:return h.wav_track;case 16:return h.wav_pad;case 17:return h.parts.output;case 18:return h.parts.bass.style;case 19:return h.parts.bass.octave;case 20:return h.parts.bass.gate;case 21:return harmony_phrase(h).rate;case 22:return harmony_phrase(h).octave;case 23:return harmony_phrase(h).gate;case 24:return harmony_phrase(h).variation;case 25:return harmony_phrase(h).pattern;case 26:return h.midi_track;case 27:return h.midi_pattern;case 28:return h.midi_read;default:return h.midi_pad;}}
void set(Context& c,int v){
    auto& h=c.ui.harmony;h.field_changed_at=c.now;if(h.row<31||h.row==33)end_preview(c);if(h.row==33){h.scale_choice=uint8_t(std::clamp(v,0,13));h.stale=true;return;}if(h.row==30){harmony::set_output_rhythm(h.parts,unsigned(std::clamp(v,0,int(harmony::bass_rhythm_count-1))));return;}if(h.row>=31){h.choice=uint8_t(std::clamp(v,0,3));return;}if(h.row==7&&h.parts.output!=0)return;if(h.mode==2&&(h.row==11||h.row==12||h.row==14))return;v=std::clamp(v,0,int(limit(h.row)));
    if(h.row==0)h.settings.follow=harmony::Follow(v);if(h.row==1)h.settings.strength=uint8_t(v);
    if(h.row>=2&&h.row<=4){unsigned flag=1u<<(h.row-2);h.settings.complexity=uint8_t(v?h.settings.complexity|flag:h.settings.complexity&~flag);}
    if(h.row==5)h.settings.leading=v;if(h.row==6){h.settings.genre=uint8_t(v);h.parts.genre=uint8_t(v);}if(h.row==10)h.target=uint8_t(v);if(h.row==11)h.settings.variation=uint16_t(v);if(h.row==12)h.follow_project=v;if(h.row==13){if(h.mode!=unsigned(v))h.wav_key_valid=false;h.mode=uint8_t(v);}if(h.row==14)h.bars=v?8:4;
    if((h.row==15&&h.wav_track!=unsigned(v))||(h.row==16&&h.wav_pad!=unsigned(v)))h.wav_key_valid=false;
    if(h.row==15){h.wav_track=uint8_t(v);h.wav_pad=c.project.tracks[h.wav_track].last_pad%128;}if(h.row==16)h.wav_pad=uint8_t(v);
    if(h.row==17)h.parts.output=v;if(h.row==18)h.parts.bass.style=uint8_t(v);if(h.row==19)h.parts.bass.octave=uint8_t(v);if(h.row==20)h.parts.bass.gate=uint8_t(std::max(10,v));
    if(h.parts.output>=2){auto& phrase=harmony_phrase(h);if(h.row==21)phrase.rate=uint8_t(v);if(h.row==22)phrase.octave=uint8_t(v);if(h.row==23)phrase.gate=uint8_t(std::max(10,v));if(h.row==24)phrase.variation=uint16_t(v);if(h.row==25)phrase.pattern=uint8_t(v);}
    h.parts=harmony::normalize_output(h.parts);
    if(h.row==26){h.midi_track=uint8_t(v);const auto& from=c.project.tracks[h.midi_track];h.midi_pattern=from.pattern;h.midi_read=from.engine==EngineKind::Sampler&&from.levels?1:0;h.midi_pad=from.levels?from.levels_pad:from.last_pad;}
    if(h.row==27)h.midi_pattern=uint8_t(v);if(h.row==28)h.midi_read=uint8_t(v);if(h.row==29)h.midi_pad=uint8_t(v);if(h.mode==1&&h.row>=26)h.stale=true;
    if(h.row<=6||(h.row>=11&&h.row<=16))h.stale=true;
    if(h.result.count){auto& ch=h.result.chords[h.selected%h.result.count];if(h.row==7)harmony::invert(ch,v-int(ch.inversion));
        if(h.row==8||h.row==9){unsigned at=h.selected%h.result.count;uint32_t hi=at+1<h.result.count?h.result.chords[at+1].tick:h.result_length;hi=std::max(1u,hi);
            if(h.row==8){uint32_t lo=at?h.result.chords[at-1].tick+h.result.chords[at-1].length:0;ch.tick=std::clamp<uint32_t>(v,std::min(lo,hi-1),hi-1);ch.length=std::min(ch.length,hi-ch.tick);}
            else ch.length=std::clamp<uint32_t>(v,1,hi-ch.tick);
        }
    }
}
bool melodic(const Track& t){return t.used()&&t.engine!=EngineKind::Sampler&&t.engine!=EngineKind::Audio;}
void apply_roll(Context& c,bool keep){
    auto& h=c.ui.harmony;const unsigned target=h.confirm?h.confirm_track:h.target;const unsigned pattern=h.confirm?h.confirm_pattern:c.project.tracks[target].pattern;
    if(!melodic(c.project.tracks[target])||c.project.tracks[target].pattern!=pattern||c.frozen(target)||c.ui.recording){h.confirm=false;c.notice("Choose an unfrozen melodic Roll; stop recording");return;}
    auto& dest=c.patterns.at[target][pattern%pattern_count];Pattern made;std::string error;
    const uint32_t bar=c.project.bar_ticks(),length=uint32_t((uint64_t(h.result_length)+bar-1)/bar*bar);
    std::vector<harmony::Note> generated;
    if(!harmony::output_notes(h.result,h.parts,c.project.beat_ticks(),generated,error,c.project.bar_ticks())||!harmony::roll_notes_edit(dest,generated,length,bar*max_bars,keep,made,error)){h.confirm=false;c.notice(error);return;}
    c.edit_pattern(target,pattern,keep?"Add generated MIDI":"Replace with generated MIDI",[&]{dest=std::move(made);c.sync_pattern(target,pattern);});
    remember(c);h.confirm=false;h.open=false;end_preview(c);c.dirty=true;c.select_track(target);c.open(PageId::Roll);
    for(auto& pot:c.ui.pots){pot.bound=0;pot.engaged=false;}c.notice(keep?"Generated notes added; existing kept":"Roll replaced with generated notes");
}
}
void harmony_stop_preview(Context& c){end_preview(c);}
void harmony_open(Context& c){
    if(c.ui.recording){c.notice("Stop recording before analysis");return;}if(!melodic(c.track())){c.notice("Choose a melodic instrument track");return;}
    const bool importing=c.ui.harmony.midi_import_ready;
    auto& h=c.ui.harmony;h.open=true;h.wav_key_valid=false;h.confirm=false;h.source=c.ui.track;h.pattern=c.track().pattern;h.target=h.source;h.selected=0;c.ui.sheet_open=false;
    for(unsigned t=0;t<track_count;++t){const auto& tr=c.project.tracks[t];auto pad=tr.last_pad%sampler_pads;if(tr.used()&&tr.pads[pad].used()){h.wav_track=t;h.wav_pad=pad;break;}}
    if(!h.midi_source_set){h.midi_source_set=true;h.midi_track=c.ui.track;h.midi_pattern=c.track().pattern;h.midi_read=0;h.midi_pad=0;}
    if(h.midi_import_ready){h.midi_import_ready=false;h.mode=1;h.row=26;h.stale=true;if(h.busy&&c.library)c.library->cancel_chord_audio(h.job);}
    if(!importing&&c.ui.harmony_setups[c.ui.track].used)restore_harmony_setup(h,c.ui.harmony_setups[c.ui.track]);
    h.field_changed_at=c.now;h.parts.genre=h.settings.genre;h.options=false;harmony_step_begin(h,0);h.stale=true;
}
void harmony_generate(Context& c){
    auto& h=c.ui.harmony;h.parts.genre=h.settings.genre;end_preview(c);if(c.ui.recording||h.busy){c.notice(h.busy?"WAV analysis in progress":"Stop recording before analysis");return;}
    h.settings.strict_scale=h.scale_choice!=0;
    if(h.scale_choice){h.settings.root=int8_t(h.mode==2&&h.wav_key_valid?h.wav_key_root:c.project.root%12);h.settings.scale_mask=scales()[(h.scale_choice-1)%scales().size()].mask;}
    if(h.mode==2){
        if(!c.library){c.notice("Sample library unavailable");return;}
        const auto& tr=c.project.tracks[h.wav_track];const auto& pad=tr.pads[h.wav_pad];
        if(!pad.used()||pad.sample>=sample_slots){c.notice("Select a loaded WAV track and pad");return;}
        const auto& sample=c.project.samples[pad.sample];std::string path=sample.path;std::string extension=std::filesystem::path(path).extension().string();std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char ch){return char(std::tolower(ch));});
        if(!sample.used()||sample.instrument()||path.empty()||path[0]!='/'||extension!=".wav"){c.notice("Choose a loaded WAV file");return;}
        h.audio_tempo=c.project.tempo_of(h.pattern);h.audio_bar=c.project.bar_ticks();
        auto analysis=h.settings;
        if(h.auto_key_job||!h.scale_choice){analysis.root=-1;analysis.scale_mask=0;analysis.strict_scale=false;}
        // Key detection is independent of the user's enabled chord types.
        if(h.auto_key_job){analysis.complexity=7;h.key_job_track=h.wav_track;h.key_job_pad=h.wav_pad;}
        h.job=c.library->analyse_chord_audio(path,analysis,h.audio_tempo,h.settings.follow==harmony::Follow::Bar?h.audio_bar:h.settings.follow==harmony::Follow::Note?std::max(1u,c.project.beat_ticks()/4):c.project.beat_ticks(),h.audio_bar*max_bars);
        if(!h.job){c.notice("Previous WAV analysis is finishing");return;}
        h.busy=true;h.percent=0;h.stale=true;h.audio_name=std::filesystem::path(path).filename().string();c.notice("Analysing whole WAV in background");return;
    }
    std::vector<harmony::Note> melody;const Pattern* sourcePattern=&c.pattern();
    if(h.mode==1){
        h.midi_notes=0;const auto& sourceTrack=c.project.tracks[h.midi_track];if(sourceTrack.engine==EngineKind::Audio||(h.midi_read&&sourceTrack.engine!=EngineKind::Sampler)){c.notice("Choose a MIDI/keyed Roll or a pitched sampler pad");return;}
        sourcePattern=&c.patterns.at[h.midi_track][h.midi_pattern%pattern_count];
        Pattern chosen;
        bool selected=h.midi_track==c.ui.track&&h.midi_pattern==c.track().pattern&&roll_selected(c.ui,c.track().pattern);
        if(selected){chosen.length=sourcePattern->length;for(size_t n=0;n<sourcePattern->events.size();++n)if(roll_is_marked(*sourcePattern,c.ui,n))chosen.events.push_back(sourcePattern->events[n]);}
        std::string error;if(!harmony::midi_source(selected?chosen:*sourcePattern,h.midi_read!=0,h.midi_pad,melody,error)){h.stale=true;c.notice(error);return;}h.midi_notes=uint16_t(melody.size());
    }
    if(!h.scale_choice){h.settings.root=(h.follow_project||h.mode==0)?int8_t(c.project.root%12):-1;h.settings.scale_mask=(h.follow_project||h.mode==0)?scales()[c.project.scale%scales().size()].mask:0;}
    h.result_length=h.mode==0?h.bars*c.project.bar_ticks():sourcePattern->length;
    h.result=h.mode==0?harmony::progression(h.bars,c.project.bar_ticks(),h.settings):harmony::generate(melody,sourcePattern->length,c.project.beat_ticks(),c.project.bar_ticks(),h.settings);
    h.stale=false;h.selected=0;remember(c);c.notice(h.result.count?"Generated "+std::to_string(h.result.count)+" chords":h.scale_choice?"No matching chords; enable Triads or change scale":"No melody / enable a chord type");
}
void harmony_again(Context& c,bool from_roll){
    if(c.ui.harmony.busy){c.notice("Previous analysis is still finishing");return;}
    const bool saved=c.ui.harmony_setups[c.ui.track].used;
    harmony_open(c);auto& h=c.ui.harmony;if(!h.open)return;
    if(!saved&&!from_roll){c.notice("Choose a setup once, then Generate Again");return;}
    if(from_roll){h.mode=1;h.midi_track=c.ui.track;h.midi_pattern=c.track().pattern;h.midi_read=0;h.midi_source_set=true;}
    vary_harmony_setup(h);harmony_step_begin(h,3);harmony_generate(c);
    if(!h.busy&&!h.stale&&h.result.count)h.choice=1;
}
void harmony_pot(Context& c,unsigned knob,int v){auto& h=c.ui.harmony;h.field_changed_at=c.now;if(h.confirm||h.busy)return;if(knob==0){auto rows=h.options?harmony_rows(h):harmony_step_rows(h);h.row=rows.at[std::clamp(v,0,127)*rows.count/128];}else if(knob==1)set(c,int((uint64_t(v)*limit(h.row)+63)/127));else if(h.result.count)h.selected=uint16_t(v*h.result.count/128);}
void harmony_wheel(Context& c,int delta){auto& h=c.ui.harmony;if(h.confirm){h.confirm_pick=std::clamp<int>(h.confirm_pick+delta,0,2);return;}if(!h.busy)set(c,value(h)+delta);}
void audition_region(Context& c){
    auto& h=c.ui.harmony;if(h.confirm){if(h.confirm_pick==0)h.confirm=false;else apply_roll(c,h.confirm_pick==1);return;}if(h.busy||h.stale)return;end_preview(c);
    if(c.host.playing()){c.notice("Stop playback to preview this region");return;}if(!h.result.count)return;
    if(!melodic(c.project.tracks[h.target])){c.notice("Choose an instrument target track");return;}
    const auto& chord=h.result.chords[h.selected%h.result.count];
    harmony::Result region;
    const uint32_t preview_end=chord.tick+c.project.bar_ticks()*2;
    for(unsigned n=h.selected%h.result.count;n<h.result.count&&h.result.chords[n].tick<preview_end;++n){auto part=h.result.chords[n];part.length=std::min(part.length,preview_end-part.tick);region.chords[region.count++]=part;}
    std::vector<harmony::Note> notes;std::string error;
    if(!harmony::output_notes(region,h.parts,harmony::ppq,notes,error,c.project.bar_ticks())){c.notice(error);return;}
    if(notes.size()>h.rhythm_preview.size()){c.notice("Preview too dense; choose a slower rhythm");return;}
    for(unsigned n=0;n<notes.size();++n){h.rhythm_preview[n]=notes[n];h.rhythm_preview[n].tick-=chord.tick;}
    h.preview_track=h.target;h.rhythm_preview_count=uint16_t(notes.size());h.rhythm_preview_next=0;h.rhythm_preview_active.fill(false);
    h.rhythm_preview_tempo=c.project.tempo_of(h.pattern);h.rhythm_preview_start=c.now;
    c.notice("Playing generated rhythm / up to two bars");
}
namespace {
void open_options(HarmonyTool& h) {
    h.options = true;
    h.row = harmony_rows(h).at[0];
}
void next_step(Context& c) {
    auto& h = c.ui.harmony;
    if (h.step == 2 && (!melodic(c.project.tracks[h.target]) || c.frozen(h.target))) {
        c.notice("Choose an unfrozen instrument track"); return;
    }
    if (h.step == 3 && (h.stale || !h.result.count)) {
        c.notice("Push GENERATE first; review the result"); return;
    }
    const bool detect_wav = harmony_detect_on_confirm(h);
    if (detect_wav) {
        h.auto_key_job = true;
        harmony_generate(c);
        if (!h.busy) { h.auto_key_job = false; return; }
    }
    harmony_step_forward(h);
    for (auto& pot : c.ui.pots) { pot.bound = 0; pot.engaged = false; }
}
}
void harmony_export(Context& c);
void harmony_add(Context& c);
void harmony_preview(Context& c) {
    c.ui.harmony.field_changed_at=c.now;
    auto& h = c.ui.harmony;
    if (h.confirm || h.options) { audition_region(c); return; }
    if (h.busy) return;
    if (c.ui.shift) { open_options(h); return; }
    if (h.step < 3) { next_step(c); return; }
    if (h.choice == 3) { open_options(h); return; }
    if (h.step == 3) {
        if (h.choice == 1) {
            if (h.stale || !h.result.count) c.notice("Generate first");
            else audition_region(c);
        } else {
            if (h.choice == 2 && h.mode != 2) h.settings.variation = (h.settings.variation + 1) % 1000;
            if (h.choice == 2 && h.parts.output >= 2) {
                auto& phrase = harmony_phrase(h); phrase.variation = (phrase.variation + 1) % 1000;
            }
            // WAV analysis is deterministic; new top phrases reuse its result.
            if (h.choice == 2 && h.mode == 2 && !h.stale && h.result.count && h.parts.output >= 2)
                c.notice("New phrase ready; choose LISTEN or NEXT");
            else harmony_generate(c);
            if (!h.stale && h.result.count) h.choice = 1;
        }
    } else if (h.choice == 1) audition_region(c);
    else if (h.choice == 2) harmony_export(c);
    else harmony_add(c);
}
namespace {
void tick_rhythm_preview(Context& c){auto& h=c.ui.harmony;advance_harmony_preview(h,c.now-h.rhythm_preview_start,[&](unsigned key,unsigned velocity){c.note(h.preview_track,key,velocity);},[&](unsigned key){c.note(h.preview_track,key,0);});
}
}
void harmony_tick(Context& c){
    auto& h=c.ui.harmony;
    if(h.busy&&c.library){h.percent=c.library->chord_audio_progress();std::string error;uint32_t ticks;harmony::Result result;
        if(c.library->take_chord_audio(h.job,result,ticks,error)){h.busy=false;
            if(h.auto_key_job){
                h.auto_key_job=false;h.stale=true;h.wav_key_valid=error.empty()&&result.count&&h.open&&h.mode==2&&h.wav_track==h.key_job_track&&h.wav_pad==h.key_job_pad;
                if(h.wav_key_valid){h.wav_key_root=result.tonic;h.wav_key_minor=result.minor;c.notice("WAV key: "+std::string(note_letter(result.tonic))+(result.minor?" minor":" major"));}
                else c.notice(error.empty()?"No clear WAV key; choose another sample":error);
            }else if(h.mode!=2){h.stale=true;c.notice("Preview step: Push GENERATE for MIDI");}else if(error.empty()){h.result=result;h.result_length=ticks;h.selected=0;h.stale=false;remember(c);if(!h.options&&h.step==3)h.choice=1;c.notice("Whole WAV: "+std::to_string(result.count)+" estimated chords");}else {h.stale=true;c.notice(error);}}
    }
    if(h.open&&(h.source!=c.ui.track||h.pattern!=c.track().pattern||c.ui.recording||c.ui.page!=PageId::Roll)){h.open=false;h.confirm=false;}
    if(h.open&&(h.scale_choice||(h.mode!=2&&(h.follow_project||h.mode==0)))&&(h.settings.root!=int8_t(h.mode==2&&h.wav_key_valid?h.wav_key_root:c.project.root%12)||h.settings.scale_mask!=scales()[h.scale_choice?(h.scale_choice-1)%scales().size():c.project.scale%scales().size()].mask))h.stale=true;
    if(h.open&&h.mode==2&&(h.audio_tempo!=c.project.tempo_of(h.pattern)||h.audio_bar!=c.project.bar_ticks()))h.stale=true;
    if(h.preview_count&&int32_t(c.now-h.preview_until)>=0)end_preview(c);
    if((h.preview_count||h.rhythm_preview_count)&&(c.ui.page!=PageId::Roll||!h.open||c.ui.recording||c.host.playing()))end_preview(c);
    tick_rhythm_preview(c);
}
void harmony_export(Context& c) {
    auto& h=c.ui.harmony;
    if(h.busy||h.stale||!h.result.count){c.notice("Generate before saving MIDI");return;}
    if(!c.library||c.library->card().empty()){c.notice("Chord MIDI: no SD card");return;}auto ts=c.project.time_signature();std::vector<uint8_t> bytes;
        std::vector<harmony::Note> notes;std::string error;if(!harmony::output_notes(h.result,h.parts,c.project.beat_ticks(),notes,error,c.project.bar_ticks())){c.notice(error);return;}bytes=harmony::midi_notes(notes,c.project.tempo_of(h.pattern),ts.beats,ts.unit);
        if(bytes.empty()){c.notice("No valid generated MIDI");return;}
        if(!c.library->write_chord_midi(std::string(std::array<const char*,5>{"TW_CHORDS_","TW_BASS_","TW_PLUCK_","TW_MELODY_","TW_HYBRID_"}[std::min(4u,unsigned(h.parts.output))])+std::to_string(c.project.tempo_of(h.pattern)/10)+"BPM",std::move(bytes)))c.notice("MIDI export already pending");else c.notice("Saving optional MIDI copy");return;
}
void harmony_add(Context& c) {
    auto& h=c.ui.harmony;
    if(h.busy||h.stale||!h.result.count){c.notice("Generate before adding notes");return;}

        if(!melodic(c.project.tracks[h.target])){c.notice("Target must be a melodic instrument track");return;}
        auto pattern=c.project.tracks[h.target].pattern;
        if(!c.patterns.at[h.target][pattern%pattern_count].events.empty()){h.confirm=true;h.confirm_track=h.target;h.confirm_pattern=pattern;h.confirm_pick=0;end_preview(c);return;}
        apply_roll(c,false);
}
void harmony_soft(Context& c,int key) {
    c.ui.harmony.field_changed_at=c.now;
    auto& h=c.ui.harmony;
    if(h.confirm){if(key==0)h.confirm=false;else apply_roll(c,key==1);return;}
    if(h.busy){if(key==0&&c.library)c.library->cancel_chord_audio(h.job);return;}
    if(h.options){
        if(key==0){h.options=false;h.row=harmony_step_rows(h).at[0];}
        else if(key==1)harmony_generate(c);
        else if(c.ui.shift)harmony_export(c);
        else audition_region(c);
        return;
    }
    if(key==0){
        end_preview(c);
        if(h.step==0&&h.row==13){remember(c);h.open=false;}
        else harmony_step_back(h);
    } else if(key==1){if(h.step==4)harmony_add(c);else next_step(c);}
    else if(c.ui.shift&&h.step==4)harmony_export(c);
    else open_options(h);
    for(auto& pot:c.ui.pots){pot.bound=0;pot.engaged=false;}
}
}
