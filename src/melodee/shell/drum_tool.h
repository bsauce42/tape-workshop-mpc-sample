// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/model/drum_generator.h"
#include "melodee/model/harmonizer.h"
namespace melodee {
class Context;
struct DrumTool {
 bool open=false,stale=true,confirm=false,browsing=false,sound_load_requested=false;
 uint8_t page=0,row=0,role=0,owner=0,pattern=0,confirm_pick=0;
 uint8_t preview_pair=0;std::array<bool,drums::roles> muted{};
 uint8_t sound_pad=0,sound_role=0,previous_pad=0;uint32_t field_changed_at=0;
 uint64_t fingerprint=0;drums::Settings settings;
 std::array<uint8_t,drums::roles> pads{};
 std::vector<drums::Hit> hits;
 std::array<harmony::Note,512> rhythm_preview{};
 uint16_t rhythm_preview_count=0,rhythm_preview_next=0,rhythm_preview_tempo=1200;
 uint32_t rhythm_preview_start=0;std::array<bool,512> rhythm_preview_active{};
};
// Mutes omit a lane from audition and Add; they never erase its generated hits.
inline std::array<uint8_t,drums::roles> drum_active_pads(const DrumTool& d){auto pads=d.pads;for(unsigned n=0;n<drums::roles;++n)if(d.muted[n])pads[n]=0;return pads;}
inline drums::Settings drum_lane_settings(const DrumTool& d){auto settings=d.settings;settings.locked.fill(true);if(d.role<drums::roles)settings.locked[d.role]=d.settings.locked[d.role];return settings;}
template<class Track> inline int drum_sound_destination(const DrumTool& d,const Track& track){if(d.role>=drums::roles)return -1;if(d.pads[d.role])return d.pads[d.role]-1;for(unsigned n=0;n<128;++n){bool mapped=false;for(auto p:d.pads)mapped|=p==n+1;if(!track.pads[n].used()&&!mapped)return int(n);}return -1;}
inline bool drum_sound_matches(const DrumTool& d,unsigned track,unsigned pad){return pad<128&&d.sound_role<drums::roles&&d.open&&d.sound_load_requested&&d.owner==track&&d.sound_pad==pad;}
inline void drum_next(DrumTool& d){if(d.page==0&&d.row<5)++d.row;else if(d.page==1&&d.row==0){if(d.role+1<drums::roles)++d.role;else d.row=1;}else if(d.page<3){++d.page;d.row=0;}}
inline void drum_back(DrumTool& d){if(d.page==1&&d.row==0&&d.role){--d.role;}else if(d.page<2&&d.row){--d.row;}else if(d.page){--d.page;d.row=d.page==0?5:0;}else d.open=false;}
void drum_browse_sound(Context&);void drum_sound_landed(Context&,unsigned,unsigned);
void drum_open(Context&);void drum_stop(Context&);void drum_tick(Context&);
void drum_pot(Context&,unsigned,int);void drum_wheel(Context&,int);void drum_soft(Context&,int);void drum_push(Context&);
}
