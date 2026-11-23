// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Tape Workshop contributors
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace melodee {
// UI-only transport state. Audio freshness and level, not Play, determine motion.
struct PluginAudioMotion {
    uint32_t stamp=0,previous_ms=0,fresh_ms=0;
    float phase=0;
    bool initialized=false;
    float update(uint32_t now,uint32_t audio_stamp,float peak,bool enabled,float speed=1.f) {
        if(!initialized){initialized=true;previous_ms=now;fresh_ms=now;stamp=audio_stamp;return phase;}
        if(audio_stamp!=stamp){stamp=audio_stamp;fresh_ms=now;}
        const auto elapsed=std::min<uint32_t>(now-previous_ms,100);previous_ms=now;
        if(enabled && peak>.0003f && now-fresh_ms<100)
            phase=std::fmod(phase+elapsed*.0026f*std::clamp(speed,.25f,4.f),6.283185307f);
        return phase;
    }
};
}
