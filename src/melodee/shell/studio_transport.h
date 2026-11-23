// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/shell/context.h"
#include "melodee/engine/engine_host.h"
#include <juce_core/juce_core.h>
namespace melodee {
// Project metadata on the MIDI thread. Fast positions remain in /studio/meters.
inline juce::var studio_transport(Context& c,unsigned track){
    juce::var value(new juce::DynamicObject);auto* out=value.getDynamicObject();
    const unsigned pattern=std::min<unsigned>(c.host.pattern_of(track),pattern_count-1);
    out->setProperty("track",int(track));out->setProperty("pattern",int(pattern));
    out->setProperty("length",int(c.patterns.at[track][pattern].length));
    out->setProperty("barTicks",int(c.project.bar_ticks()));out->setProperty("beatTicks",int(c.project.beat_ticks()));
    out->setProperty("tempo",int(c.host.tempo_now()));
    out->setProperty("songLength",int(c.project.song.length()));
    return value;
}
}
