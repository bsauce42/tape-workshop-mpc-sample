// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <future>
#include <optional>
#include <string>

namespace melodee {

class VoiceSearch {
public:
    bool available() const { return installed.load(std::memory_order_relaxed); }
    bool begin();
    void release() { held.store(false, std::memory_order_release); }
    void cancel();
    bool busy() const { return phase.load(std::memory_order_acquire) != Phase::Idle; }
    bool route_wanted() const;
    void routed(bool attempted, bool ok);
    void capture(const int32_t* stereo, unsigned frames, unsigned rate);
    struct Result { int status; std::string query; };
    std::optional<Result> service();

private:
    enum class Phase { Idle, Route, Start, Capture, Ready, Decode };
    static Result decode(const std::string& folder, const float* samples, unsigned count, unsigned rate);
    std::atomic<Phase> phase {Phase::Idle};
    std::atomic<bool> held {false}, cancelled {false}, installed {false};
    std::array<float, 192000> samples {}; // up to two seconds, including at 96 kHz
    unsigned count = 0, rate = 0, settle = 0; // audio owns these until Ready
    std::future<Result> pending;
    std::string folder;
    bool checked = false;
};

} // namespace melodee
