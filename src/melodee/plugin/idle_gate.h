// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

// When a plug-in may sleep: its processBlock skipped, at no cost, until
// something could make it sound again. An instrument sleeps once it holds no
// key (sustain pedal included), has had nothing in its output above -100 dB
// for half a second, and does not hear the song playing (a plug-in with a
// sequencer of its own plays from Play alone; TRANSPORT off, it never hears
// it). An effect sleeps once its input and output have been as quiet for as
// long; the song playing does not matter. Any MIDI for it, or sound in an
// effect's input, or a parameter change wakes it. Gate state is audio-thread-only;
// request_wake() is lock-free across threads. No allocation.
namespace melodee {

class IdleGate {
public:
    static constexpr float quiet_level = 1e-5f; // -100 dB
    static constexpr double quiet_seconds = 0.5;

    // `rate`: the frames the plug-in's output is counted in (its own rate).
    void reset(bool is_instrument, double rate) {
        instrument = is_instrument;
        quiet_needed = uint32_t(std::lround(quiet_seconds * rate));
        quiet = 0;
        wake_requested.store(false,std::memory_order_relaxed);
        held.fill(0);
        held_count = 0;
        sustain = 0;
    }
    // Control/loader threads request work without touching audio-owned gate state.
    // Noise generators and self-oscillating effects must wake after a quiet preset.
    void request_wake() { wake_requested.store(true,std::memory_order_release); }
    // Every MIDI message sent to the plug-in, as sent: keys held and the sustain pedal followed.
    void midi(uint8_t status, uint8_t data1, uint8_t data2) {
        const uint8_t kind = status & 0xf0, channel = status & 0x0f;
        const unsigned key = data1 & 0x7f;
        const uint8_t bit = uint8_t(1u << (key & 7));
        uint8_t& byte = held[size_t(channel) * 16 + key / 8];
        if (kind == 0x90 && data2 > 0) {
            if (!(byte & bit)) { byte |= bit; ++held_count; }
        } else if (kind == 0x80 || kind == 0x90) {
            if (byte & bit) { byte &= uint8_t(~bit); --held_count; }
        } else if (kind == 0xb0 && data1 == 64) {
            sustain = data2 >= 64 ? uint16_t(sustain | (1u << channel)) : uint16_t(sustain & ~(1u << channel));
        } else if (kind == 0xb0 && (data1 == 120 || data1 == 123)) {
            for (unsigned k = 0; k < 16; ++k) {
                held_count -= unsigned(__builtin_popcount(held[size_t(channel) * 16 + k]));
                held[size_t(channel) * 16 + k] = 0;
            }
        }
    }
    // What it played: `frames` of its output, at its own rate.
    void heard(const float* left, const float* right, unsigned frames) {
        if (loud(left, right, frames)) quiet = 0;
        else quiet = std::min<uint32_t>(quiet + frames, quiet_needed);
    }
    // Before a render: whether to skip it. `midi_waiting`: messages for it since its last render; `playing`: the song
    // as it hears it; `input`: an effect's, for this render (device frames).
    bool asleep(bool midi_waiting, bool playing, const float* left, const float* right, unsigned frames) {
        const bool changed=wake_requested.exchange(false,std::memory_order_acquire);
        if (midi_waiting || changed) { quiet = 0; return false; }
        if (instrument) {
            if (held_count || sustain || playing) return false;
        } else if (left && right && loud(left, right, frames)) {
            quiet = 0;
            return false;
        }
        return quiet >= quiet_needed;
    }
    unsigned keys_held() const { return held_count; }

private:
    static bool loud(const float* left, const float* right, unsigned frames) {
        for (unsigned f = 0; f < frames; ++f)
            if (std::fabs(left[f]) > quiet_level || std::fabs(right[f]) > quiet_level) return true;
        return false;
    }
    static_assert(std::atomic<bool>::is_always_lock_free,"Audio wake requests must be lock-free");
    std::atomic<bool> wake_requested {false};
    bool instrument = true;
    uint32_t quiet_needed = 22050, quiet = 0;
    std::array<uint8_t, 16 * 16> held {}; // a bit a channel's key
    unsigned held_count = 0;
    uint16_t sustain = 0;                 // a bit a channel: its pedal down
};

} // namespace melodee
