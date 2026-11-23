// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace melodee {

struct SampleKey {
    std::string path;
    int root = -1; // MIDI note for a stable single pitch
    int tonic = -1; // pitch class for a tonal loop
    bool minor = false;
    bool ready = false;
};

// The caller supplies mono audio at 6 kHz. Use normalized autocorrelation
// for stable pitches and pitch-class energy for multi-note material.
inline SampleKey estimate_sample_key(const std::vector<float>& mono) {
    constexpr int window = 1024, min_lag = 4, max_lag = 190;
    constexpr double pi = 3.141592653589793;
    SampleKey result;
    result.ready = true;
    if (mono.size() < 400) return result;
    const int size = int(std::min<size_t>(window, mono.size()));
    std::array<unsigned, 128> votes {};
    std::array<double, 12> chroma {};
    unsigned voiced = 0, active = 0;
    for (size_t start = 0; start + size <= mono.size(); start += 768) {
        const float* x = mono.data() + start;
        double energy = 0;
        for (int i = 0; i < size; ++i) energy += double(x[i]) * x[i];
        if (energy < 0.018) continue;
        ++active;
        std::array<double, max_lag + 1> correlation {};
        for (int lag = min_lag; lag <= max_lag; ++lag) {
            double cross = 0, a = 0, b = 0;
            for (int i = 0; i < size - max_lag; i += 2) {
                cross += double(x[i]) * x[i + lag];
                a += double(x[i]) * x[i];
                b += double(x[i + lag]) * x[i + lag];
            }
            correlation[lag] = cross / std::sqrt(std::max(1e-15, a*b));
        }
        for (int lag = min_lag + 1; lag < max_lag; ++lag) {
            const auto here = correlation[lag], prev = correlation[lag-1], next = correlation[lag+1];
            if (here < .86 || here < prev || here <= next) continue;
            const double period = lag + .5*(prev-next)/(prev-2*here+next);
            const int note = int(std::lround(69 + 12*std::log2(6000/(440*period))));
            if (note >= 0 && note < int(votes.size())) { ++votes[note]; ++voiced; }
            break;
        }
        std::array<double, window> tapered {};
        for (int i = 0; i < size; ++i) tapered[i] = x[i]*(.5-.5*std::cos(2*pi*i/size));
        // Goertzel bins at note frequencies. Start above the poorly resolved bass bins.
        std::array<double, 12> frame {};
        for (int note = 45; note <= 88; ++note) {
            const double frequency = 440*std::exp2((note-69)/12.0);
            const double coefficient = 2*std::cos(2*pi*frequency/6000);
            double previous = 0, older = 0;
            for (int i = 0; i < size; ++i) {
                const double next = tapered[i] + coefficient*previous - older;
                older = previous;
                previous = next;
            }
            frame[note%12] += std::max(0.0, previous*previous+older*older-coefficient*previous*older);
        }
        double total = 0;
        for (const double bin : frame) total += bin;
        if (total > .001) for (int i = 0; i < 12; ++i) chroma[i] += frame[i]/total;
    }
    const auto root = std::max_element(votes.begin(), votes.end());
    if (active && voiced && *root >= .80*active) {
        result.root = int(root - votes.begin());
        return result;
    }
    if (active < 3 || *std::max_element(chroma.begin(), chroma.end()) < active*.15) return result;
    constexpr std::array<double,12> major {{6.35,2.23,3.48,2.33,4.38,4.09,2.52,5.19,2.39,3.66,2.29,2.88}};
    constexpr std::array<double,12> minor {{6.33,2.68,3.52,5.38,2.60,3.53,2.54,4.75,3.98,2.69,3.34,3.17}};
    double best = -1, second = -1;
    int tonic = -1;
    bool is_minor = false;
    for (int mode = 0; mode < 2; ++mode) {
        const auto& profile = mode ? minor : major;
        double mean = 0;
        for (double x : profile) mean += x/12;
        for (int key = 0; key < 12; ++key) {
            double cross = 0, a = 0, b = 0;
            for (int i = 0; i < 12; ++i) {
                const double x = chroma[(key+i)%12], y = profile[i]-mean;
                cross += x*y;
                a += x*x; // norm is constant across rotations
                b += y*y;
            }
            const double score = cross/std::sqrt(std::max(1e-15, a*b));
            if (score > best) { second = best; best = score; tonic = key; is_minor = mode != 0; }
            else if (score > second) second = score;
        }
    }
    if (best > .37 && best-second > .025) { result.tonic = tonic; result.minor = is_minor; }
    return result;
}
}
