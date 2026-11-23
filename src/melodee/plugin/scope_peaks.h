// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace melodee {

// One scope column per channel. Keep the signed endpoints so opposite-phase
// stereo signals stay visible and invalid audio cannot enter the display.
inline uint32_t scope_column(const float* samples, unsigned count) {
    float low = 1.f, high = -1.f;
    for (unsigned i = 0; i < count; ++i) {
        const float sample = samples[i];
        if (!std::isfinite(sample)) continue;
        low = std::min(low, sample);
        high = std::max(high, sample);
    }
    if (low > high) return 0;
    auto encode = [](float value) {
        return uint16_t(int16_t(std::lround(32767 * std::clamp(value, -1.f, 1.f))));
    };
    return uint32_t(encode(low)) | (uint32_t(encode(high)) << 16);
}

} // namespace melodee
