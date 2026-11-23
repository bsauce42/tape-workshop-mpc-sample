// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include "melodee/model/limits.h"
#include "melodee/model/sampler.h"
#include "melodee/plugin/prism_spectrum.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>

namespace melodee {

// One audio writer and one screen reader. The extra window keeps the reader
// away from samples being overwritten even if another audio block begins.
class MixSpectrum {
public:
    static constexpr unsigned pad_count = sampler_pads / sampler_banks, prism_pre = track_count + pad_count,
                              prism_post = prism_pre + 1, sources = prism_post + 1;
    static constexpr unsigned focus_count = track_count * sampler_banks;
    static constexpr unsigned capacity = 2 * prism_fft_size;
    using Samples = std::array<std::array<float, prism_fft_size>, 2>;
    static int selection(unsigned track, unsigned bank, int prism_slot = -1) {
        return track < track_count ? int(track * sampler_banks + bank % sampler_banks) +
                                    (prism_slot + 1) * int(focus_count) : -1;
    }
    void watch(int selection) { requested.store(selection, std::memory_order_relaxed); }
    void watch(unsigned track, unsigned bank) { watch(selection(track, bank)); }
    int prism_slot() const { return focus < 0 ? -1 : focus / int(focus_count) - 1; }
    int prepare(unsigned frames) {
        const int next = requested.load(std::memory_order_relaxed);
        if (next != focus) {
            focus = next;
            valid = 0;
            status.store(0, std::memory_order_release);
        }
        if (focus >= 0)
            for (auto& pad : pads) { std::fill_n(pad[0].begin(), frames, 0.f); std::fill_n(pad[1].begin(), frames, 0.f); }
        prism_frames = 0;
        return focus < 0 ? -1 : focus % int(focus_count);
    }
    void prism(bool after, const float* left, const float* right, unsigned frames) {
        auto& tap = prism_taps[after ? 1 : 0];
        std::copy_n(left, frames, tap[0].begin());
        std::copy_n(right, frames, tap[1].begin());
        if (after) prism_frames = frames;
    }
    void pad(unsigned slot, unsigned frame, float l, float r) {
        pads[slot][0][frame] += l; pads[slot][1][frame] += r;
    }
    void sample(unsigned source, unsigned frame, float l, float r) {
        const auto quantize = [](float v) {
            return uint16_t(int16_t(std::isfinite(v) ? std::clamp(v, -1.f, 1.f) * 32767.f : 0.f));
        };
        ring[source][(head + frame) % capacity].store(uint32_t(quantize(l)) | uint32_t(quantize(r)) << 16, std::memory_order_relaxed);
    }
    void finish(unsigned frames, float gain, float track_audible = 1.f) {
        for (unsigned pad = 0; pad < pad_count; ++pad)
            for (unsigned f = 0; f < frames; ++f)
                sample(track_count + pad, f, pads[pad][0][f] * gain, pads[pad][1][f] * gain);
        if (prism_slot() >= 0)
            for (unsigned tap = 0; tap < 2; ++tap)
                for (unsigned f = 0; f < frames; ++f) {
                    const auto& stereo = prism_taps[tap];
                    sample(prism_pre + tap, f, f < prism_frames ? stereo[0][f] * track_audible : 0.f,
                                               f < prism_frames ? stereo[1][f] * track_audible : 0.f);
                }
        head += frames;
        valid = std::min(valid + frames, prism_fft_size);
        if (valid == prism_fft_size)
            status.store((uint64_t(head) << 16) | unsigned(focus + 1), std::memory_order_release);
    }
    uint64_t window(int expected) const {
        const uint64_t ticket = status.load(std::memory_order_acquire);
        return expected >= 0 && uint16_t(ticket) == expected + 1 ? ticket : 0;
    }
    uint64_t window(unsigned track, unsigned bank) const { return window(selection(track, bank)); }
    bool read(unsigned source, uint64_t first, Samples& out) const {
        if (!first || source >= sources) return false;
        const uint32_t end = uint32_t(first >> 16);
        bool audible = false;
        for (unsigned i = 0; i < prism_fft_size; ++i) {
            const auto pair = ring[source][(end - prism_fft_size + i) % capacity].load(std::memory_order_relaxed);
            out[0][i] = int16_t(pair) / 32768.f;
            out[1][i] = int16_t(pair >> 16) / 32768.f;
            audible |= std::max(std::abs(out[0][i]), std::abs(out[1][i])) > .003f;
        }
        const uint64_t last = status.load(std::memory_order_acquire);
        return audible && uint16_t(first) == uint16_t(last) && uint32_t(uint32_t(last >> 16) - end) < prism_fft_size - 128;
    }
private:
    std::atomic<int> requested {-1};
    int focus = -1;
    unsigned valid = 0;
    uint32_t head = 0;
    std::atomic<uint64_t> status {0};
    std::array<std::array<std::atomic<uint32_t>, capacity>, sources> ring {};
    std::array<std::array<std::array<float, 128>, 2>, pad_count> pads {};
    std::array<std::array<std::array<float, 128>, 2>, 2> prism_taps {};
    unsigned prism_frames = 0;
};

struct SpectrumOverlap {
    std::array<float, prism_spectrum_bins> glow {};
    int source = -1, reference_pad = -1;
    unsigned bin = 0;
    float strength = 0;
    float relief = 0;
    bool quiet = false;
};

inline float overlap_at(float a, float b) {
    return std::max(0.f, std::min(a, b) - .38f) * std::max(0.f, 1.f - 3.f * std::abs(a - b));
}

inline void update_overlap(SpectrumOverlap& result, const std::array<float, prism_spectrum_bins>& reference,
                            const std::array<float, prism_spectrum_bins>& candidate, unsigned source) {
    for (unsigned i = 0; i < result.glow.size(); ++i) {
        const float a = reference[i], b = candidate[i];
        // Both signals must be present and reasonably close in level.
        const float overlap = overlap_at(a, b);
        result.glow[i] = std::max(result.glow[i], overlap);
        if (overlap > result.strength) {
            result.reference_pad = -1;
            result.strength = overlap;
            result.bin = i;
            result.source = int(source);
        }
    }
}
// Balance means the measured EQ cut eased a clash at the same frequency.
inline float overlap_relief(float before, float after, float other) {
    const float initial = overlap_at(before, other);
    if (initial < .09f) return 0.f;
    const float remaining = overlap_at(after, other);
    const float reduced = std::clamp((initial - remaining) / initial, 0.f, 1.f);
    const float cut = std::clamp((before - after) * 78.f / 8.f, 0.f, 1.f);
    return std::min(cut, reduced);
}
struct PrismOverlapRow {
    int source = -1, reference_pad = -1;
    unsigned bin = 0;
    float strength = 0.f, relief = 0.f;
};

struct PrismOverlapList {
    std::array<std::array<PrismOverlapRow, 2>, 3> bands {};

    static unsigned band(unsigned bin) {
        const float position = (bin + .5f) / prism_spectrum_bins;
        return position < .366f ? 0u : position < .667f ? 1u : 2u;
    }

    void add(const SpectrumOverlap& candidate, const float* pre = nullptr,
             const float* post = nullptr, const float* other = nullptr) {
        std::array<PrismOverlapRow, 3> peaks {};
        for (unsigned bin = 0; bin < candidate.glow.size(); ++bin) {
            auto& peak = peaks[band(bin)];
            const float strength = candidate.glow[bin];
            if (strength <= .09f || strength <= peak.strength) continue;
            const float relief = pre && post && other ? overlap_relief(pre[bin], post[bin], other[bin]) : 0.f;
            peak = {candidate.source, candidate.reference_pad, bin, strength, relief};
        }
        for (unsigned i = 0; i < peaks.size(); ++i) {
            const auto& peak = peaks[i];
            auto& rows = bands[i];
            if (peak.strength > rows[0].strength) { rows[1] = rows[0]; rows[0] = peak; }
            else if (peak.strength > rows[1].strength) rows[1] = peak;
        }
    }

    bool empty() const {
        return std::all_of(bands.begin(), bands.end(), [](const auto& rows) { return rows[0].source < 0; });
    }
};

struct PrismOverlapHold {
    PrismOverlapList last;
    bool was_playing = false, stopped = false;

    void transport(bool playing) {
        if (playing) stopped = false;
        if (!playing && was_playing) stopped = !last.empty();
        was_playing = playing;
    }

    void observe(const PrismOverlapList& live) {
        if (!stopped && !live.empty()) last = live;
    }
};
} // namespace melodee
