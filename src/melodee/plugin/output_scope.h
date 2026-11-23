// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Tape Workshop contributors
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace melodee {
// One audio writer; a UI reader. Fixed storage, bounded work, no locks or allocation.
// Packed extrema preserve small transients without copying audio buffers to the UI.
class PluginOutputScope {
public:
    using Columns = std::array<uint32_t, 64>;
    static_assert(std::atomic<uint32_t>::is_always_lock_free, "Scope requires lock-free words");

    void publish(const float* left, const float* right, unsigned frames) noexcept {
        if (!left || !right) return;
        auto at = head.load(std::memory_order_relaxed);
        for (unsigned offset = 0; offset < frames; offset += 96) {
            const auto count = std::min(96u, frames - offset);
            l[at % 64].store(pack(left + offset, count), std::memory_order_relaxed);
            r[at % 64].store(pack(right + offset, count), std::memory_order_relaxed);
            ++at;
        }
        head.store(at, std::memory_order_release);
    }
    uint32_t read(Columns& left, Columns& right) const noexcept {
        const auto at = head.load(std::memory_order_acquire);
        for (unsigned i = 0; i < 64; ++i) {
            left[i] = l[(at + i) % 64].load(std::memory_order_relaxed);
            right[i] = r[(at + i) % 64].load(std::memory_order_relaxed);
        }
        return at;
    }

private:
    static uint32_t pack(const float* samples, unsigned frames) noexcept {
        float low = 0, high = 0;
        for (unsigned i = 0; i < frames; ++i) {
            const auto v = std::isfinite(samples[i]) ? std::clamp(samples[i], -1.f, 1.f) : 0.f;
            low = std::min(low, v);
            high = std::max(high, v);
        }
        return uint32_t(uint16_t(int16_t(low * 32767))) | (uint32_t(uint16_t(high * 32767)) << 16);
    }
    std::array<std::atomic<uint32_t>, 64> l {}, r {};
    std::atomic<uint32_t> head {0};
};
} // namespace melodee
