// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/plugin/output_scope.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <thread>

int main() {
    using namespace melodee;
    PluginOutputScope scope;
    PluginOutputScope::Columns left {}, right {};
    assert(scope.read(left, right) == 0);
    for (auto v : left) assert(v == 0);
    std::array<float, 256> l {}, r {};
    for (unsigned i = 0; i < l.size(); ++i) {
        l[i] = i % 2 ? -.5f : .5f;
        r[i] = i % 2 ? -.25f : .25f;
    }
    const auto original_l = l, original_r = r;
    for (int i = 0; i < 30; ++i) scope.publish(l.data(), r.data(), l.size());
    assert(l == original_l && r == original_r); // the tap never modifies audio
    const auto at = scope.read(left, right);
    assert(at == 90);
    for (unsigned i = 0; i < 64; ++i) {
        assert((left[i] >> 16) == 16383);
        assert((right[i] >> 16) == 8191);
        assert(int16_t(left[i] & 65535) == -16383);
    }
    scope.publish(l.data(), r.data(), 0);
    assert(scope.read(left, right) == at);
    l.fill(0); r.fill(0);
    for (int i = 0; i < 22; ++i) scope.publish(l.data(), r.data(), l.size());
    scope.read(left, right);
    for (unsigned i = 0; i < 64; ++i) assert(left[i] == 0 && right[i] == 0);
    l[0] = std::numeric_limits<float>::quiet_NaN();
    l[1] = std::numeric_limits<float>::infinity(); l[2] = -l[1];
    l[3] = 3; l[4] = -3;
    scope.publish(l.data(), r.data(), 24);
    scope.read(left, right);
    assert(left.back() >> 16 == 32767);
    assert(int16_t(left.back() & 65535) == -32767);
    // The UI may read during writes: all shared data are atomic packed words.
    std::atomic<bool> done {false};
    std::thread writer([&] {
        for (int i = 0; i < 20000; ++i) scope.publish(r.data(), r.data(), r.size());
        done.store(true);
    });
    while (!done.load()) scope.read(left, right);
    writer.join();
    std::cout << "PASS: stereo signal capture, audio unchanged, rolling window, silence, nonfinite/clipping safety, concurrent reader\n";
}
