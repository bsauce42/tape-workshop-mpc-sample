// SPDX-License-Identifier: AGPL-3.0-only
// Live browser editor for Melodee by Kerem Kiliç (Ellic Studio).
#include "melodee/shell/studio.h"
#include "melodee/shell/studio_transport.h"
#include "melodee/shell/studio_model.h"
#include "melodee/shell/context.h"
#include "melodee/shell/new_track.h"
#include "melodee/shell/sample_mode.h"
#include "melodee/web/control_feed.h"
#include "melodee/engine/engine_host.h"
#include "melodee/engine/engine_info.h"
#include <juce_core/juce_core.h>
#include <cmath>

namespace melodee {
namespace {
using juce::var;
var object() { return var(new juce::DynamicObject); }
void put(var& o, const char* key, const var& value) { o.getDynamicObject()->setProperty(key, value); }
void put(var& o, const char* key, const std::string& value) { put(o,key,var(juce::String(value))); }
int number(const var& o, const char* key, int low, int high) {
    auto value = o.getProperty(key, var());
    if (!(value.isInt() || value.isInt64() || value.isDouble()) || !std::isfinite(double(value)) || std::floor(double(value)) != double(value) || double(value) < low || double(value) > high) throw std::runtime_error(std::string("Invalid ") + key);
    return int(value);
}
std::string text(const var& o, const char* key) { return o.getProperty(key, var()).toString().toStdString(); }
void editable(Context& c, unsigned t) {
    if (!c.project.tracks[t].used()) throw std::runtime_error("Choose an existing track, or create a Drum track first");
    if (c.ui.recording || c.ui.rec_armed || c.in_take(t, c.project.tracks[t].pattern)) throw std::runtime_error("Stop recording and disarm record before browser editing");
    if (c.frozen_edit(t)) throw std::runtime_error("Unfreeze this track on the MPC first");
}
var snapshot(Context& c, unsigned t, unsigned p, unsigned from, unsigned to) {
    var state = object();
    put(state,"notesVersion",int(c.web_feedback?c.web_feedback->pattern_version():0));
    put(state,"generation",std::to_string(c.project_generation));put(state,"name",juce::String(c.project.name));
    put(state,"playing",c.host.playing());put(state,"songPlaying",c.host.song_playing());put(state,"tempo",int(c.host.tempo_now()));put(state,"tick",int(c.host.ticks()));put(state,"position",c.host.position_of(t));
    put(state,"barTicks",int(c.project.bar_ticks()));put(state,"songBar",int(c.host.song_bar()));put(state,"dirty",c.dirty);put(state,"recording",c.ui.recording);
    put(state,"transport",studio_transport(c,t));
    put(state,"track",int(t));put(state,"pattern",int(p));
    var tracks(juce::Array<var>{});
    for(unsigned i=0;i<track_count;++i){const auto& track=c.project.tracks[i];var tr=object();put(tr,"index",int(i));put(tr,"name",juce::String(track.name));put(tr,"used",track.used());put(tr,"engine",int(track.engine));put(tr,"type",juce::String(engine_info(track.engine).name));put(tr,"drum",track.engine==EngineKind::Sampler);put(tr,"audio",track.engine==EngineKind::Audio);put(tr,"mute",track.mix.mute);put(tr,"solo",track.mix.solo);put(tr,"pattern",int(track.pattern));put(tr,"level",int(track.mix.level));put(tr,"pan",int(track.mix.pan));tracks.append(tr);}
    put(state,"tracks",tracks);
    const auto& pattern=c.patterns.at[t][p];put(state,"revision",studio_revision(pattern));put(state,"length",int(pattern.length));put(state,"totalNotes",int(pattern.events.size()));put(state,"grid",int(pattern.q()));
    var notes(juce::Array<var>{});unsigned count=0;
    for(unsigned i=0;i<pattern.events.size();++i){const auto& e=pattern.events[i];if(e.tick>=to)break;if(e.tick+e.length<=from)continue;if(++count>2048)break;var n(juce::Array<var>{int(i),int(e.tick),int(e.length),int(e.key),int(e.velocity),int(e.chance),int(e.flags),int(e.pitch),int(e.ratchet),int(e.ramp),int(e.pitch_ramp),int(e.condition),int(e.cutoff)});notes.append(n);}
    put(state,"notes",notes);put(state,"truncated",count>2048);
    var sequences(juce::Array<var>{});for(unsigned i=0;i<pattern_count;++i){var seq=object();put(seq,"index",int(i));put(seq,"notes",int(c.patterns.at[t][i].events.size()));put(seq,"length",int(c.patterns.at[t][i].length));sequences.append(seq);}put(state,"patterns",sequences);
    var lanes(juce::Array<var>{});for(unsigned i=0;i<track_count;++i){var lane(juce::Array<var>{});for(unsigned j=0;j<pattern_count;++j)if(!c.patterns.at[i][j].empty())lane.append(int(j));lanes.append(lane);}put(state,"lanes",lanes);
    var clips(juce::Array<var>{});for(unsigned i=0;i<c.project.song.count;++i){const auto& clip=c.project.song.clips[i];var v(juce::Array<var>{int(i),int(clip.track),int(clip.pattern),int(clip.bar),int(clip.bars),clip.muted});clips.append(v);}put(state,"clips",clips);put(state,"songRevision",studio_revision(c.project.song));put(state,"songLength",int(c.project.song.length()));
    put(state,"loopStart",int(c.project.song.loop_start));put(state,"loopEnd",int(c.project.song.loop_end));
    var pads(juce::Array<var>{});const auto& track=c.project.tracks[t];if(track.engine==EngineKind::Sampler){for(unsigned i=0;i<sampler_pads;++i){const auto& pad=track.pads[i];var v=object();put(v,"key",int(sampler_first_note+i));put(v,"name",juce::String(pad.used() && pad.sample < sample_slots ? c.project.samples[pad.sample].name : ""));pads.append(v);}}put(state,"pads",pads);
    return state;
}
}
void studio_tick(Context& c, uint32_t now) {
    for(unsigned t=0;t<track_count;++t){auto& note=c.studio_held[t];if(note.key>=0&&int32_t(now-note.until)>=0){c.note(t,note.key,0);note.key=-1;}}
}
std::string studio_command(Context& c, const std::string& request) {
    const var ask=juce::JSON::parse(juce::String(request));const auto action=text(ask,"action");
    const unsigned t=unsigned(number(ask,"track",0,track_count-1)),p=unsigned(number(ask,"pattern",0,pattern_count-1));
    const unsigned from=unsigned(number(ask,"from",0,128*1536)),to=unsigned(number(ask,"to",1,128*1536));
    if(to<=from || to-from>16*1536)throw std::runtime_error("Editor view must be 1–16 bars");
    if(action!="studio_state" && action!="studio_stop" && text(ask,"generation")!=std::to_string(c.project_generation))throw std::runtime_error("Project changed on the MPC. Refresh the editor.");
    std::string message;
    if(action=="studio_audition"){
        editable(c,t);if(c.project.tracks[t].engine==EngineKind::Audio)throw std::runtime_error("Audio tracks do not play MIDI notes");
        const bool drum=c.project.tracks[t].engine==EngineKind::Sampler;
        const int key=number(ask,"key",drum?int(sampler_first_note):0,drum?int(sampler_last_key):127),velocity=number(ask,"velocity",1,127);
        auto& held=c.studio_held[t];if(held.key>=0)c.note(t,held.key,0);c.note(t,key,velocity);held.key=key;held.until=c.now+220;message="Preview played on MPC";
    }else if(action=="studio_notes"){
        editable(c,t);if(c.project.tracks[t].engine==EngineKind::Audio)throw std::runtime_error("Audio tracks have no editable MIDI notes");
        if(number(ask,"engine",0,int(EngineKind::Count)-1)!=int(c.project.tracks[t].engine))throw std::runtime_error("Track type changed on the MPC");
        const auto list=ask.getProperty("changes",var());if(!list.isArray()||list.size()<1||list.size()>128)throw std::runtime_error("Select 1–128 notes per edit");
        std::vector<StudioNoteChange> changes;
        for(const auto& item:*list.getArray()){
            StudioNoteChange change;const auto kind=text(item,"kind");
            if(kind=="add")change.kind=StudioNoteChange::Kind::Add;
            else if(kind=="update")change.kind=StudioNoteChange::Kind::Update;
            else if(kind=="delete")change.kind=StudioNoteChange::Kind::Delete;
            else throw std::runtime_error("Unknown note edit");
            if(kind!="add")change.index=unsigned(number(item,"index",0,max_pattern_events-1));
            if(kind!="delete"){change.tick=unsigned(number(item,"tick",0,128*1536));change.length=unsigned(number(item,"length",1,65535));change.key=unsigned(number(item,"key",0,255));change.velocity=unsigned(number(item,"velocity",1,127));}
            if(kind!="delete" && item.hasProperty("play")) {
                const auto play=item.getProperty("play",var());
                if(!play.isArray()||play.size()!=8)throw std::runtime_error("Invalid note playback settings");
                var values=object();const char* keys[]={"chance","flags","pitch","ratchet","ramp","pitch_ramp","condition","cutoff"};
                for(int i=0;i<8;++i)put(values,keys[i],play[i]);
                change.carry_play=true;auto& e=change.play;
                e.chance=uint8_t(number(values,"chance",0,100));e.flags=uint8_t(number(values,"flags",0,15));e.pitch=int8_t(number(values,"pitch",-48,48));e.ratchet=uint8_t(number(values,"ratchet",1,8));e.ramp=int8_t(number(values,"ramp",-100,100));e.pitch_ramp=int8_t(number(values,"pitch_ramp",-24,24));e.condition=uint8_t(number(values,"condition",0,43));e.cutoff=uint8_t(number(values,"cutoff",0,101));
            }
            changes.push_back(change);
        }
        auto next=studio_notes(c.patterns.at[t][p],text(ask,"revision"),changes,c.project.tracks[t].engine==EngineKind::Sampler?sampler_last_key:127);
        c.edit_pattern(t,p,"Browser MIDI edit",[&]{c.patterns.at[t][p]=std::move(next);c.sync_pattern(t,p);});
        const auto audition=ask.getProperty("audition",var(false));
        if(audition.isBool()&&bool(audition)&&!c.host.playing())for(auto i=changes.rbegin();i!=changes.rend();++i)if(i->kind!=StudioNoteChange::Kind::Delete){auto& held=c.studio_held[t];if(held.key>=0)c.note(t,held.key,0);c.note(t,int(i->key),int(i->velocity));held.key=int(i->key);held.until=c.now+220;break;}
        message="MIDI updated on MPC";
    }else if(action=="studio_device"){
        editable(c,t);if(c.ui.shift)throw std::runtime_error("Release Shift on the MPC first");c.select_track(t);const auto page=text(ask,"page");
        if(page=="chop"||page=="stems"||page=="trim") {
            if(c.project.tracks[t].engine!=EngineKind::Sampler)throw std::runtime_error("Choose a drum/sample track for this tool");
            const unsigned pad=unsigned(number(ask,"pad",0,sampler_pads-1));
            if(!c.project.tracks[t].pads[pad].used())throw std::runtime_error("Choose a pad containing a sample");
            c.track().last_pad=uint8_t(pad);c.track().bank=uint8_t(pad/16);
            if(page=="chop"){if(c.ui.page==PageId::Chop)c.open(PageId::Trim);chop_button(c);if(c.ui.page!=PageId::Chop)throw std::runtime_error("Chop unavailable for this pad; check the MPC notice");}
            else if(page=="stems"){c.open_stems_sheet();if(c.ui.page!=PageId::Stems)throw std::runtime_error("Stem splitter unavailable for this sample; check the MPC notice");}
            else c.open(PageId::Trim);
        } else if(page=="instrument")c.open(PageId::Edit);
        else if(page=="effects"){c.ui.fx_loop=-1;c.open(PageId::Rack);}
        else throw std::runtime_error("Unknown MPC tool");
        message="MPC tool requested. Use its live screen and controls; confirm processing on the MPC.";
    }else if(action=="studio_length"){
        editable(c,t);if(c.project.tracks[t].engine==EngineKind::Audio)throw std::runtime_error("Set audio-track length on MPC");
        if(text(ask,"revision")!=studio_revision(c.patterns.at[t][p]))throw std::runtime_error("Pattern changed. Refresh before resizing.");
        const unsigned length=unsigned(number(ask,"bars",1,max_bars))*c.project.bar_ticks();
        c.edit_pattern(t,p,"Browser pattern length",[&]{auto& pattern=c.patterns.at[t][p];pattern.length=length;pattern.fit_lanes();c.sync_pattern(t,p);});message="Pattern length updated on MPC";
    }else if(action=="studio_clip"){
        if(c.ui.recording || c.ui.rec_armed)throw std::runtime_error("Stop recording before arrangement edits");
        if(!c.project.tracks[t].used())throw std::runtime_error("Choose an existing track");int index=number(ask,"index",-1,song_clips-1);Clip clip;const auto mode=text(ask,"mode");
        clip.track=uint8_t(t);clip.pattern=uint8_t(p);clip.bar=uint16_t(number(ask,"bar",0,song_bars-1));clip.bars=uint16_t(number(ask,"bars",1,song_bars));clip.muted=number(ask,"muted",0,1)!=0;
        if(mode!="put"&&mode!="delete")throw std::runtime_error("Unknown clip edit");
        auto next=studio_clip(c.project.song,text(ask,"songRevision"),index,mode=="delete"?nullptr:&clip);
        auto edit=c.begin_edit("Browser arrangement",t,-1,true);c.project.song=std::move(next);c.sync_song();c.dirty=true;c.commit_edit(std::move(edit));message="Arrangement updated on MPC";
    }else if(action=="studio_launch"){if(!c.project.tracks[t].used()||c.ui.recording||c.ui.rec_armed)throw std::runtime_error("Choose a used track and stop recording first");c.launch(t,p);message="Pattern launch sent to MPC";
    }else if(action=="studio_scene"){if(c.ui.recording||c.ui.rec_armed)throw std::runtime_error("Disarm recording first");c.launch_scene(p);if(!c.host.playing()){c.open(PageId::Stage);c.play();}message="Sequence started or queued on MPC";
    }else if(action=="studio_play"){
        const bool song=number(ask,"song",0,1)!=0;if(c.host.playing()&&c.host.song_playing()!=song)throw std::runtime_error("Stop playback before switching Song/Pattern transport");
        if(c.ui.recording||c.ui.rec_armed)throw std::runtime_error("Disarm recording on the MPC first");
        if(!c.host.playing()){c.open(song?PageId::Song:PageId::Stage);if(song)c.ui.song_cursor=uint16_t(number(ask,"bar",0,song_bars-1));c.play();}message="Playback on MPC";
    }else if(action=="studio_stop"){c.stop();message="Stopped on MPC";
    }else if(action=="studio_tempo"){c.set_tempo(number(ask,"tempo",300,3000));message="Tempo updated on MPC";
    }else if(action=="studio_mix"){
        if(!c.project.tracks[t].used()||c.ui.recording||c.ui.rec_armed)throw std::runtime_error("Choose an existing track and stop recording first");
        const auto field=text(ask,"field");if(field!="level"&&field!="pan")throw std::runtime_error("Unknown mixer control");
        const int value=number(ask,"value",field=="level"?0:-50,field=="level"?127:50);
        c.touch_scope({UndoScope::Kind::Mix,0,0},0x570000u+t*2+(field=="pan"?1:0));
        auto& mix=c.project.tracks[t].mix;
        if(field=="level"){mix.level=uint8_t(value);c.send({Command::Type::Level,uint8_t(t),0,0,int16_t(value)});}
        else{mix.pan=int8_t(value);c.send({Command::Type::Pan,uint8_t(t),0,0,int16_t(value)});}
        c.dirty=true;message="Mixer updated on MPC";
    }else if(action=="studio_mute"){c.set_mute(t,number(ask,"value",0,1)!=0);c.dirty=true;
    }else if(action=="studio_solo"){c.set_solo(t,number(ask,"value",0,1)!=0);c.dirty=true;
    }else if(action=="studio_undo"||action=="studio_redo"){if(c.ui.recording)throw std::runtime_error("Stop recording first");if(action=="studio_undo")c.undo();else c.redo();message="MPC undo history updated";
    }else if(action=="studio_save"){c.save_project();message="Save requested on MPC. Confirm the filename there if asked.";
    }else if(action=="studio_export"){if(c.host.playing())throw std::runtime_error("Stop playback before exporting");c.open(PageId::Song);c.ui.song_export=true;message="Export opened on MPC. Choose and confirm the export on the unit.";
    }else if(action=="studio_new_drum"){if(c.project.tracks[t].used())throw std::runtime_error("Choose an empty track");if(c.ui.recording)throw std::runtime_error("Stop recording first");c.select_track(t);make_new_track(c,NewChoice::Kit);message="Empty drum track created on MPC";
    }else if(action!="studio_state")throw std::runtime_error("Unknown studio command");
    if(action=="studio_audition"){auto reply=object();put(reply,"ack",true);put(reply,"generation",std::to_string(c.project_generation));return juce::JSON::toString(reply,true).toStdString();}
    auto state=snapshot(c,t,p,from,to);put(state,"message",juce::String(message));return juce::JSON::toString(state,true).toStdString();
}
}
