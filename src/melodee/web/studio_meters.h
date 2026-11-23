// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/model/limits.h"
#include <algorithm>
#include <cmath>
#include <string>
namespace melodee::web {
// Audio peaks are published by EngineHost already. Conversion/JSON runs on
// the HTTP worker, with no Shell lock and no new audio-thread work.
inline int studio_peak_db10(float amplitude) {
    if (!std::isfinite(amplitude) || amplitude <= 0.00001584893f) return -960;
    return std::clamp(int(std::lround(200.0f * std::log10(amplitude))), -960, 120);
}
template<class Source> std::string studio_meter_json(const Source& host) {
    std::string json="{\"tracks\":[";
    for(unsigned t=0;t<track_count;++t){if(t)json+=',';json+=std::to_string(studio_peak_db10(host.meter(t)));}
    json+="],\"positions\":[";for(unsigned t=0;t<track_count;++t){if(t)json+=',';json+=std::to_string(host.position_of(t));}
    json+="],\"patterns\":[";for(unsigned t=0;t<track_count;++t){if(t)json+=',';json+=std::to_string(host.pattern_of(t));}
    return json+"],\"left\":"+std::to_string(studio_peak_db10(host.out_level(0)))+",\"right\":"+std::to_string(studio_peak_db10(host.out_level(1)))+",\"ticks\":"+std::to_string(host.ticks())+",\"songBar\":"+std::to_string(host.song_bar())+",\"playing\":"+(host.playing()?"true":"false")+",\"songPlaying\":"+(host.song_playing()?"true":"false")+"}";
}
}
