// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/shell/context.h"
#include "melodee/shell/sample_library.h"
#include "melodee/plugin/melody_pattern.h"
namespace melodee {
inline bool open_melody_sample_key(Context& c,int key){
    const auto& panel=c.ui.plugin_panel;
    const int base=melody::parameter_base(panel.name,panel.visual_count);
    if(key!=2||!c.ui.shift||c.track().engine!=EngineKind::Plugin||panel.entry!=c.track().plugin||base<0||panel.row*3u+2<unsigned(base))return false;
    if(!c.library||c.library->card().empty()){c.notice("No SD card");return true;}
    c.stop_preview();c.library->preview("");const auto back=c.ui.page;c.open(PageId::Samples);
    c.ui.sample_key_pick=true;c.ui.sample_key_back=back;c.ui.files_cursor=0;
    c.ui.midi_import=c.ui.expansions=c.ui.expansion_home=c.ui.sample_folder_pick=false;
    c.library->browse(Drive::Card,"");c.ui.files_drive=uint8_t(Drive::Card);
    c.notice("Choose audio; B3 confirms the estimated key");return true;
}
}
