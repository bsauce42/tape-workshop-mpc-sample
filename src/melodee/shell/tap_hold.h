// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include <cstdint>

// Tap Tempo held. As on the stock app, holding it and turning the encoder sets the tempo: the Tempo page shows
// while it is held and goes back when it is let go. Held tap_hold_ms with nothing turned, it opens the Tempo page
// to stay; held again on that page, it closes it (a tool's opener). Pure, for the shell and the tests.
namespace melodee {

constexpr uint32_t tap_hold_ms = 500;

struct TapHold {
    bool down = false;   // Tap Tempo is down
    bool opened = false; // this hold opened the Tempo page
    bool turned = false; // the encoder was turned during this hold
    bool held = false;   // the hold's own job (open or close) is done
    uint32_t at = 0;     // ms: when it went down
};

enum class TapDo : uint8_t { Nothing, Open, Close };

inline void tap_press(TapHold& t, uint32_t now) { t = TapHold {true, false, false, false, now}; }

// Each pass while it is down: once held long enough, open the page, or close it when it was open already.
inline TapDo tap_tick(TapHold& t, uint32_t now, bool on_tempo) {
    if (!t.down || t.held || t.turned || now - t.at <= tap_hold_ms) return TapDo::Nothing;
    t.held = true;
    if (on_tempo) return TapDo::Close;
    t.opened = true;
    return TapDo::Open;
}

// The encoder turned while it is down: the tempo, on the Tempo page (opened when another page is up).
inline TapDo tap_turn(TapHold& t, bool on_tempo) {
    if (!t.down) return TapDo::Nothing;
    t.turned = true;
    if (on_tempo) return TapDo::Nothing;
    t.opened = true;
    return TapDo::Open;
}

// Let go: a page this hold opened for a turn of the encoder goes back; one opened by holding alone stays.
inline TapDo tap_release(TapHold& t, bool on_tempo) {
    bool close = t.down && t.opened && t.turned && on_tempo;
    t.down = false;
    return close ? TapDo::Close : TapDo::Nothing;
}

} // namespace melodee
