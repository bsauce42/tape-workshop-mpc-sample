// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop; host based on Melodee by Kerem Kiliç (Ellic Studio).
#pragma once
#include "melodee/model/harmonizer.h"
#include <complex>
namespace melodee::harmony {
struct Chroma { uint32_t tick=0,length=1;std::array<float,12> tones{},bass{};unsigned frames=0; };
// Streaming mono 6 kHz analysis. A fixed FFT window and at most 256 musical bins;
// never retains the full recording. Non-real-time worker only.
class AudioFeatures {
public:
    AudioFeatures(double seconds,unsigned tempo_tenths,unsigned beat);
    void push(float sample);
    std::vector<Chroma> finish();
    uint32_t ticks() const {return total_ticks;}
private:
    static constexpr unsigned window=4096,hop=1024;
    std::array<float,window> ring{},hann{};
    std::array<std::complex<float>,window> spectrum{},roots{};
    std::array<unsigned,window> reverse{};
    std::vector<Chroma> bins;
    uint64_t samples=0;uint32_t total_ticks=0,bin_ticks=1;double ticks_per_sample=0;
    void frame(bool tail=false);
};
Result recognize(const std::vector<Chroma>& bins,const Settings& settings);
}
