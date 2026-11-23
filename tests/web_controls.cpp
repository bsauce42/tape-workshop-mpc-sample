// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/web/control_feed.h"
#include <cassert>
#include <iostream>
int main() {
    melodee::web::ControlFeed feed;
    SurfaceInput input;
    input.kind = SurfaceInput::Kind::Button;
    input.index = uint8_t(SurfaceButton::Shift);
    input.pressed = true;
    feed.record(input, -1, false, false, 1000);
    input.kind = SurfaceInput::Kind::Control;
    input.index = uint8_t(SurfaceControl::K3); input.value = 91;
    feed.record(input, -1, false, false, 1010);
    input.index = uint8_t(SurfaceControl::Wheel); input.delta = -2;
    feed.record(input, -1, false, false, 1020);
    input = {};
    feed.record(input, 15, true, false, 1030);
    feed.record(input, 15, false, false, 1040);
    std::cout << feed.json(1050) << '\n';
    input.kind = SurfaceInput::Kind::Control; input.index = 250;
    feed.record(input, 500, true, false, 1050); // invalid indices are ignored
    input.index = uint8_t(SurfaceControl::K1); input.value = 42;
    feed.record(input, -1, false, true, 1050); // startup positions do not flash
    feed.record({}, 15, true, false, 1055); // second hit after release is counted
    feed.record({}, 15, false, false, 1056);
    feed.record({}, 0, true, true, 1057); // startup synchronization is not a hit
    std::cout << feed.json(1060) << '\n';
}
