// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Tape Workshop contributors
#pragma once
#include "melodee/plugin/plugin_panel.h"
#include <string_view>
namespace melodee {
inline bool workshop_meter_supported(std::string_view name) {
    if (name.size() > 5 && name.substr(name.size()-5) == " (TW)") name.remove_suffix(5);
    constexpr std::string_view names[] {"Afterglow Piano","Astral Loom","Dex FM","Layer House","Room Grand",
        "Sloop FM6","Stillwater","Velvet Choir","Break 26","Gate Keeper","Rack 76","Second Spin","Vocal Tune",
        "Aphex Move","Braids Move","Dexed Move","Hera Move","Plaits Move"};
    for (auto candidate : names) if (candidate == name) return true;
    return false;
}
inline void configure_workshop_pages(PluginPanel& panel, unsigned parameters, unsigned requested) {
    const unsigned banks = (parameters + 2) / 3;
    const bool meter = workshop_meter_supported(panel.name);
    panel.prism=(std::string_view(panel.name)=="Prism FM"||std::string_view(panel.name)=="Prism FM (TW)")&&parameters==180;
    panel.rows = uint16_t(panel.prism?prism_ui::rows:banks + (meter ? 1 : 0));
    panel.row = uint16_t(panel.rows ? std::min<unsigned>(requested,panel.rows-1) : 0);
    panel.meter_page = meter && panel.row == banks;
    panel.meter_indices = {0,1,2};
    panel.bloom_ir_first = (std::string_view(panel.name)=="Bloom Reverb" || std::string_view(panel.name)=="Bloom Reverb (TW)") && parameters==24;
}
inline void configure_workshop_meter_aliases(PluginPanel& panel, const std::vector<int>& shown) {
    if(!panel.meter_page)return;
    const std::string_view name(panel.name);
    if((name == "Second Spin" || name == "Second Spin (TW)") && shown.size() == 21) {
        constexpr int raw[] {9,10,4}; // input gain, output trim, wet/dry mix
        for(unsigned k=0;k<3;++k) {
            const auto at=std::find(shown.begin(),shown.end(),raw[k]);
            if(at!=shown.end())panel.meter_indices[k]=uint16_t(at-shown.begin());
        }
    }
}

}
