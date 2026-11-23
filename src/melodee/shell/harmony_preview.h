// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/shell/harmony_tool.h"
namespace melodee {
// Fixed-size, control-thread audition clock. All note-offs precede note-ons
// at a shared boundary; expired intervals are skipped after delayed ticks.
template<class State,class On,class Off>
void advance_harmony_preview(State& h,uint32_t elapsed,On&& on,Off&& off){
    if(!h.rhythm_preview_count)return;
    auto ms=[&](uint32_t tick){return uint32_t(uint64_t(tick)*600000/(std::max(1u,unsigned(h.rhythm_preview_tempo))*harmony::ppq));};
    for(unsigned n=0;n<h.rhythm_preview_next;++n)if(h.rhythm_preview_active[n]&&elapsed>=ms(h.rhythm_preview[n].tick+h.rhythm_preview[n].length)){off(h.rhythm_preview[n].key);h.rhythm_preview_active[n]=false;}
    while(h.rhythm_preview_next<h.rhythm_preview_count){unsigned n=h.rhythm_preview_next;const auto note=h.rhythm_preview[n];if(elapsed<ms(note.tick))break;++h.rhythm_preview_next;
        if(elapsed>=ms(note.tick+note.length))continue;on(note.key,note.velocity);h.rhythm_preview_active[n]=true;
    }
    bool active=false;for(unsigned n=0;n<h.rhythm_preview_next;++n)active|=h.rhythm_preview_active[n];
    if(h.rhythm_preview_next==h.rhythm_preview_count&&!active)h.rhythm_preview_count=0;
}
template<class State,class Off>
void stop_harmony_preview(State& h,Off&& off){
    for(unsigned n=0;n<h.rhythm_preview_count;++n)if(h.rhythm_preview_active[n])off(h.rhythm_preview[n].key);
    h.rhythm_preview_active.fill(false);h.rhythm_preview_count=0;h.rhythm_preview_next=0;
}
}
