#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <utility>

namespace melodee {

constexpr unsigned prism_fft_size = 2048;
constexpr unsigned prism_spectrum_bins = 76;

// Analyze a window of stereo output. Taking the larger channel magnitude
// preserves stereo material whose channels cancel when summed to mono.
inline std::array<float, prism_spectrum_bins> prism_spectrum(
    const std::array<std::array<float, prism_fft_size>, 2>& samples, double rate)
{
    constexpr float tau = 6.28318530718f;
    std::array<float, prism_spectrum_bins> peaks {};
    std::array<std::complex<float>, prism_fft_size> bins {};
    static const auto window = [] {
        std::array<float, prism_fft_size> result {};
        for (unsigned i = 0; i < result.size(); ++i)
            result[i] = .5f - .5f * std::cos(tau * i / (result.size() - 1));
        return result;
    }();
    for (const auto& channel : samples) {
        for (unsigned i = 0, j = 0; i < bins.size(); ++i) {
            bins[j] = channel[i] * window[i];
            if (i + 1 == bins.size()) break;
            unsigned bit = prism_fft_size >> 1;
            while (j & bit) { j ^= bit; bit >>= 1; }
            j ^= bit;
        }
        for (unsigned size = 2; size <= bins.size(); size *= 2) {
            const std::complex<float> step(std::cos(tau / size), -std::sin(tau / size));
            for (unsigned base = 0; base < bins.size(); base += size) {
                std::complex<float> phase(1, 0);
                for (unsigned j = 0; j < size / 2; ++j) {
                    const auto upper = phase * bins[base + j + size / 2];
                    const auto lower = bins[base + j];
                    bins[base + j] = lower + upper;
                    bins[base + j + size / 2] = lower - upper;
                    phase *= step;
                }
            }
        }
        for (unsigned i = 0; i < peaks.size(); ++i) {
            const double low = 20 * std::pow(1000., double(i) / peaks.size());
            const double high = 20 * std::pow(1000., double(i + 1) / peaks.size());
            const unsigned first = std::clamp(unsigned(std::ceil(low * prism_fft_size / rate)), 1u, prism_fft_size / 2);
            const unsigned last = std::clamp(unsigned(std::ceil(high * prism_fft_size / rate)), first + 1, prism_fft_size / 2 + 1);
            for (unsigned k = first; k < last; ++k)
                peaks[i] = std::max(peaks[i], std::abs(bins[k]) * (4.f / prism_fft_size));
        }
    }
    for (auto& peak : peaks)
        peak = std::clamp((20.f * std::log10(std::max(1e-5f, peak)) + 78.f) / 78.f, 0.f, 1.f);
    return peaks;
}

} // namespace melodee
