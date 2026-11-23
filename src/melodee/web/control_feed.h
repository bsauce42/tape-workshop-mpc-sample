// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "surface/controls.h"
#include "melodee/web/screen_feed.h"
#include <array>
#include <atomic>
#include <string>

namespace melodee::web {
// MIDI thread only writes fixed atomics. HTTP builds the presentation snapshot.
class ControlFeed {
public:
    ControlFeed() { for (auto& value : positions) value.store(-1); }
    void record(const SurfaceInput& input, int pad, bool pad_down, bool syncing, uint64_t now = ScreenFeed::clock()) {
        if (input.kind == SurfaceInput::Kind::Button && input.index < buttons.size()) {
            buttons[input.index].store(input.pressed);
            if (input.pressed && !syncing) button_at[input.index].store(now);
        }
        if (input.kind == SurfaceInput::Kind::Control && input.index < positions.size()) {
            if (input.index == unsigned(SurfaceControl::Wheel)) {
                wheel.fetch_add(input.delta);
            } else positions[input.index].store(input.value);
            if (!syncing) control_at[input.index].store(now);
        }
        if (pad >= 0 && unsigned(pad) < pads.size()) {
            pads[unsigned(pad)].store(pad_down);
            if (pad_down && !syncing) {
                pad_at[unsigned(pad)].store(now);
                pad_hits[unsigned(pad)].fetch_add(1);
            }
        }
    }
    // MIDI-thread live-note telemetry. Ring storage is fixed; HTTP reads it
    // independently, so UI feedback never waits for a project snapshot.
    void live_note(unsigned track, unsigned key, unsigned velocity) {
        if(track>=16||key>255||velocity<1)return;
        const uint32_t seq=note_count.load(std::memory_order_relaxed)+1;
        note_time[seq%64].store(ScreenFeed::clock(),std::memory_order_relaxed);
        note_events[seq%64].store((uint64_t(seq)<<32)|(uint64_t(track)<<16)|(uint64_t(key)<<8)|std::min(velocity,127u),std::memory_order_release);
        note_count.store(seq,std::memory_order_release);
    }
    void pattern_changed(){pattern_count.fetch_add(1,std::memory_order_relaxed);}
    uint32_t pattern_version() const{return pattern_count.load(std::memory_order_relaxed);}
    std::string notes_json(uint64_t now = ScreenFeed::clock()) const {
        const uint32_t end=note_count.load(std::memory_order_acquire),count=std::min(end,64u);
        std::string out="{\"sequence\":"+std::to_string(end)+",\"events\":[";bool comma=false;
        for(uint32_t offset=count;offset>0;--offset){const uint32_t seq=end-offset+1;const auto word=note_events[seq%64].load(std::memory_order_acquire);if(uint32_t(word>>32)!=seq)continue;const auto at=note_time[seq%64].load(std::memory_order_relaxed);if(comma)out+=',';comma=true;out+="["+std::to_string(seq)+","+std::to_string((word>>16)&15)+","+std::to_string((word>>8)&255)+","+std::to_string(now>=at?std::min<uint64_t>(now-at,60000):0)+"]";}
        return out+"]}";
    }
    std::string json(uint64_t now = ScreenFeed::clock()) const {
        auto age = [now](uint64_t at) { return at && now >= at ? std::min<uint64_t>(now - at, 60000) : 60000; };
        std::string out = "{\"buttons\":[";
        for (unsigned n=0; n<buttons.size(); ++n) {
            if (n) out += ',';
            out += "[" + std::to_string(buttons[n].load()) + "," + std::to_string(age(button_at[n].load())) + "]";
        }
        out += "],\"pads\":[";
        for (unsigned n=0; n<pads.size(); ++n) {
            if (n) out += ',';
            out += "[" + std::to_string(pads[n].load()) + "," + std::to_string(age(pad_at[n].load())) + "]";
        }
        out += "],\"pad_hits\":[";
        for (unsigned n=0; n<pads.size(); ++n) {
            if (n) out += ',';
            out += std::to_string(pad_hits[n].load());
        }
        out += "],\"controls\":[";
        for (unsigned n=0; n<positions.size(); ++n) {
            if (n) out += ',';
            out += "[" + std::to_string(positions[n].load()) + "," + std::to_string(age(control_at[n].load())) + "]";
        }
        return out + "],\"wheel\":" + std::to_string(wheel.load()) + "}";
    }
private:
    std::array<std::atomic<bool>, size_t(SurfaceButton::Count)> buttons {};
    std::array<std::atomic<bool>, 16> pads {};
    std::array<std::atomic<int>, size_t(SurfaceControl::Count)> positions;
    std::array<std::atomic<uint64_t>, size_t(SurfaceButton::Count)> button_at {};
    std::array<std::atomic<uint64_t>, 16> pad_at {};
    std::array<std::atomic<uint32_t>, 16> pad_hits {};
    std::array<std::atomic<uint64_t>, size_t(SurfaceControl::Count)> control_at {};
    std::atomic<int64_t> wheel {0};
    std::array<std::atomic<uint64_t>,64> note_events {},note_time {};
    std::atomic<uint32_t> note_count {0},pattern_count {0};
};
}
