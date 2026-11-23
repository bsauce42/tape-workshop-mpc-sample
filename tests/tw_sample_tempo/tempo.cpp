#include "melodee/engine/filename_tempo.h"
#include "melodee/engine/onsets.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
    using namespace melodee;
    assert(filename_tempo("Should I @thenickmeek - 93 BPM D Min.wav") == 930);
    assert(filename_tempo("loop_128bpm.wav") == 1280);
    assert(filename_tempo("BPM: 87.5 - loop.wav") == 875);
    assert(filename_tempo("/93 BPM/loop 2026 take 93.wav") == 0);
    assert(filename_tempo("808 kick 16.wav") == 0);
    assert(filename_tempo("x93bpm.wav") == 0);
    assert(filename_tempo("93 bpms.wav") == 0);
    assert(filename_tempo("120 BPM 93 BPM.wav") == 0);
    assert(filename_tempo("12 BPM.wav") == 0);
    for (double bpm : {60., 93., 120., 155.}) for (double tail : {0., .95}) {
        const double duration = 32 * 60 / bpm + tail;
        std::vector<float> mono(size_t(duration * 44100));
        for (int beat = 0; beat < 32; ++beat) {
            size_t at = size_t(beat * 60 / bpm * 44100);
            for (size_t k = 0; k < 1800 && at+k < mono.size(); ++k)
                mono[at+k] += float(std::exp(-double(k)/260) * std::sin(k*.37));
        }
        std::array<Hit,max_hits> hits; BeatGrid grid;
        find_hits_in(mono,hits,grid);
        std::cout << bpm << " tail " << tail << " -> " << grid.tempo/10. << '\n';
        assert(grid.found() && std::abs(grid.tempo / 10. - bpm) <= .3);
    }
}
