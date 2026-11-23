// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/shell/ui_state.h"
namespace melodee {
inline bool mixer_enter(UiState& ui, Track& track) {
    if (ui.mixer_view==MixerView::Master) return false;
    if (ui.mixer_master) { ui.mixer_master_from=ui.mixer_view;ui.mixer_view=MixerView::Master;return true; }
    if (ui.mixer_view==MixerView::Tracks) {
        ui.mixer_view=track.engine==EngineKind::Sampler?MixerView::Banks:MixerView::Track;
        if(ui.mixer_view==MixerView::Banks)track.last_pad=uint8_t(track.bank*16+track.last_pad%16);
        return true;
    }
    if(ui.mixer_view==MixerView::Banks){ui.mixer_view=MixerView::Pad;return true;}
    return false;
}
inline bool mixer_go_back(UiState& ui) {
    if(ui.mixer_menu){ui.mixer_menu=false;return true;}
    if(ui.mixer_view==MixerView::Master){ui.mixer_view=ui.mixer_master_from;ui.mixer_master=true;return true;}
    if(ui.mixer_view==MixerView::Pad){ui.mixer_view=MixerView::Banks;return true;}
    if(ui.mixer_view!=MixerView::Tracks){ui.mixer_view=MixerView::Tracks;ui.mixer_master=false;return true;}
    return false;
}
}
