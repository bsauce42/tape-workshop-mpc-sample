// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>

namespace melodee::web {
// A bounded, on-demand screen cache. Only the screen thread renders/encodes;
// HTTP only requests and copies a completed frame. Never used by audio render().
class ScreenFeed {
public:
    static constexpr uint64_t frame_interval_ms = 33; // at most about 30 fps, off the audio thread
    static uint64_t clock() {
        return uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }
    void request(uint64_t now = clock()) { until.store(now + 2500, std::memory_order_relaxed); }
    void request_png() { png_requested.store(true, std::memory_order_relaxed); request(); }
    bool take_png_request() { return png_requested.exchange(false, std::memory_order_relaxed); }
    void publish_rgb565(std::string pixels, uint64_t now, uint64_t sample, unsigned rate) {
        if (pixels.size() != 320 * 240 * 2) return;
        // Run-length encode flat UI regions; keep raw pixels when compression would grow them.
        std::string runs; runs.reserve(pixels.size());
        for (size_t at = 0; at < pixels.size();) {
            size_t count = 1;
            while (count < 65535 && at + count * 2 < pixels.size()
                && pixels[at] == pixels[at + count * 2] && pixels[at + 1] == pixels[at + count * 2 + 1]) ++count;
            runs += char(count & 255); runs += char(count >> 8);
            runs += pixels[at]; runs += pixels[at + 1]; at += count * 2;
        }
        const bool use_runs = runs.size() < pixels.size();
        {
            std::lock_guard<std::mutex> held(lock);
            compressed = use_runs;
            raw_frame = use_runs ? std::move(runs) : std::move(pixels);
            ++sequence; frame_sample = sample; frame_rate = rate;
        }
        captured.store(now, std::memory_order_relaxed);
    }
    bool wanted(uint64_t now = clock()) const {
        return now < until.load(std::memory_order_relaxed) && now >= captured.load(std::memory_order_relaxed) + frame_interval_ms;
    }
    void publish(std::string png, uint64_t now = clock(), uint64_t sample = 0, unsigned rate = 0) {
        if (png.empty() || png.size() > 320 * 240 * 4) return;
        {
            std::lock_guard<std::mutex> held(lock);
            frame = std::move(png);
            frame_sample = sample; frame_rate = rate;
        }
        captured.store(now, std::memory_order_relaxed);
    }
    std::string snapshot() const {
        std::lock_guard<std::mutex> held(lock);
        return frame;
    }
    std::string packet() const {
        std::lock_guard<std::mutex> held(lock);
        if (raw_frame.empty() && frame.empty()) return {};
        std::string out = raw_frame.empty() ? "TWVF" : "TWF2";
        for (unsigned n=0; n<4; ++n) out += char((frame_rate >> (n*8)) & 255);
        for (unsigned n=0; n<8; ++n) out += char((frame_sample >> (n*8)) & 255);
        if (!raw_frame.empty()) {
            for (unsigned n=0; n<4; ++n) out += char(((compressed ? 1u : 0u) >> (n*8)) & 255);
            for (unsigned n=0; n<4; ++n) out += char((sequence >> (n*8)) & 255);
        }
        return out + (raw_frame.empty() ? frame : raw_frame);
    }
private:
    std::atomic<uint64_t> until {0}, captured {0};
    mutable std::mutex lock;
    std::string frame, raw_frame;
    std::atomic<bool> png_requested {false};
    uint64_t frame_sample = 0;
    unsigned frame_rate = 0;
    uint32_t sequence = 0;
    bool compressed = false;
};
}
