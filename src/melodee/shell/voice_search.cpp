// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/shell/voice_search.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <dlfcn.h>
#include <filesystem>
#include <vector>

namespace melodee {

bool VoiceSearch::begin() {
    if (!available() || busy()) return false;
    cancelled.store(false, std::memory_order_relaxed);
    held.store(true, std::memory_order_relaxed);
    phase.store(Phase::Route, std::memory_order_release);
    return true;
}

void VoiceSearch::cancel() {
    if (!busy()) return;
    cancelled.store(true, std::memory_order_release);
    release();
}

bool VoiceSearch::route_wanted() const {
    const auto state = phase.load(std::memory_order_acquire);
    return held.load(std::memory_order_acquire)
        && (state == Phase::Route || state == Phase::Start || state == Phase::Capture);
}

void VoiceSearch::routed(bool attempted, bool ok) {
    auto expected = Phase::Route;
    // Route failure or an early release still goes through the audio handoff.
    if (phase.load(std::memory_order_acquire) != Phase::Route) return;
    if (!attempted && held.load(std::memory_order_acquire)) return;
    if (!ok) release();
    phase.compare_exchange_strong(expected, Phase::Start, std::memory_order_acq_rel);
}

void VoiceSearch::capture(const int32_t* stereo, unsigned frames, unsigned device_rate) {
    auto state = phase.load(std::memory_order_acquire);
    if (state == Phase::Start) {
        count = 0;
        rate = device_rate;
        settle = rate / 16; // discard the ADC switch transient
        phase.store(Phase::Capture, std::memory_order_release);
        state = Phase::Capture;
    }
    if (state != Phase::Capture) return;
    if (!held.load(std::memory_order_acquire) || device_rate != rate || !rate) {
        phase.store(Phase::Ready, std::memory_order_release);
        return;
    }
    const unsigned skip = std::min(settle, frames);
    settle -= skip;
    const unsigned limit = std::min(unsigned(samples.size()), 2 * rate);
    for (unsigned n = skip; n < frames && count < limit; ++n)
        samples[count++] = (float(stereo[2 * n]) + float(stereo[2 * n + 1])) * (0.5f / 2147483648.f);
    if (count == limit) phase.store(Phase::Ready, std::memory_order_release);
}

VoiceSearch::Result VoiceSearch::decode(const std::string& folder, const float* samples, unsigned count, unsigned rate) {
    constexpr unsigned speech_rate = 16000;
    float peak = 0;
    for (unsigned n = 0; n < count; ++n) peak = std::max(peak, std::abs(samples[n]));
    if (peak < 0.002f) return {0, {}};
    const float gain = std::min(12.f, 0.8f / peak);
    const unsigned length = unsigned(uint64_t(count) * speech_rate / rate);
    std::vector<int16_t> pcm(length + speech_rate / 5, 0); // silence lets the last consonant finish
    for (unsigned n = 0; n < length; ++n) {
        const unsigned from = unsigned(uint64_t(n) * rate / speech_rate);
        const unsigned to = std::max(from + 1, unsigned(uint64_t(n + 1) * rate / speech_rate));
        float sum = 0;
        for (unsigned frame = from; frame < to; ++frame) sum += samples[frame];
        pcm[n] = int16_t(std::clamp(sum * gain / float(to - from), -1.f, 1.f) * 32767.f);
    }
    void* module = dlopen((folder + "/decoder.so").c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!module) return {-1, {}};
    using Recognize = int (*)(const char*, const int16_t*, size_t, char*, size_t);
    const auto recognize = reinterpret_cast<Recognize>(dlsym(module, "tape_workshop_recognize"));
    char word[32] {};
    const int status = recognize ? recognize(folder.c_str(), pcm.data(), pcm.size(), word, sizeof word) : -1;
    dlclose(module);
    return {status, word};
}

std::optional<VoiceSearch::Result> VoiceSearch::service() {
    if (!checked) {
        checked = true;
        const char* override_folder = std::getenv("MELODEE_VOICE");
        folder = override_folder ? override_folder : "/run/sample-base/voice";
        std::error_code error;
        installed.store(std::filesystem::is_regular_file(folder + "/decoder.so", error), std::memory_order_relaxed);
    }
    const auto state = phase.load(std::memory_order_acquire);
    if (state == Phase::Ready) {
        if (cancelled.load(std::memory_order_acquire) || !rate || count < rate / 8) {
            const bool discard = cancelled.load(std::memory_order_acquire);
            phase.store(Phase::Idle, std::memory_order_release);
            if (!discard) return Result {0, {}};
        } else {
            try {
                pending = std::async(std::launch::async, &VoiceSearch::decode, folder, samples.data(), count, rate);
                phase.store(Phase::Decode, std::memory_order_release);
            } catch (...) {
                phase.store(Phase::Idle, std::memory_order_release);
                return Result {-1, {}};
            }
        }
    } else if (state == Phase::Decode && pending.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        Result result {-1, {}};
        try { result = pending.get(); } catch (...) {}
        const bool discard = cancelled.load(std::memory_order_acquire);
        phase.store(Phase::Idle, std::memory_order_release);
        if (!discard) return result;
    }
    return std::nullopt;
}

} // namespace melodee
