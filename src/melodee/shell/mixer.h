// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/shell/context.h"
#include "melodee/shell/mixer_state.h"
#include "melodee/shell/mute_mode.h"
#include <algorithm>

namespace melodee {
inline bool mixer_bank(const UiState& ui) { return ui.mixer_view == MixerView::Banks || ui.mixer_view == MixerView::Pad; }
inline unsigned mixer_pick(const Track& t, const UiState& ui) { return ui.mixer_master ? 16 : mixer_bank(ui) ? t.last_pad % 16 : ui.track; }
inline void mixer_select(Context& c, unsigned pick) {
    pick = std::min(pick, 16u); c.ui.mixer_master = pick == 16;
    if (pick == 16) return;
    if (mixer_bank(c.ui)) c.track().last_pad = uint8_t(c.track().bank % sampler_banks * 16 + pick);
    else c.select_track(pick);
}
inline void mixer_pad_solo(Context& c, unsigned pad, bool on) {
    pad %= sampler_pads; auto& bits = c.track().pad_solos[pad / 64];
    const auto bit = uint64_t(1) << (pad % 64); bits = on ? bits | bit : bits & ~bit;
    c.send({Command::Type::PadSolo, c.ui.track, 0, uint16_t(pad), int16_t(on)});
}
inline void mixer_action(Context& c, unsigned row) {
    auto& ui = c.ui; const bool pad = mixer_bank(ui); auto& t = c.track();
    const unsigned at = t.last_pad % sampler_pads;
    if (row == 0) { if (pad) set_pad_mute(c, ui.track, at, !pad_muted_now(c, ui.track, at)); else c.set_mute(ui.track, !t.mix.mute); }
    else if (row == 1) { if (pad) mixer_pad_solo(c, at, !t.pad_soloed(at)); else c.set_solo(ui.track, !t.mix.solo); }
    else for (unsigned track = 0; track < track_count; ++track) {
        c.set_solo(track, false); c.project.tracks[track].pad_solos = {};
        c.send({Command::Type::PadSolo, uint8_t(track), 0, 0xffff, 0});
    }
}
inline bool mixer_back(Context& c) { return mixer_go_back(c.ui); }
inline void mixer_open_fx(Context& c) {
    PageId page = c.ui.mixer_master || c.ui.mixer_view == MixerView::Master ? PageId::Master : mixer_bank(c.ui) ? PageId::PadFx : PageId::Rack;
    c.ui.mixer_fx = true; c.ui.mixer_fx_edit = false; c.open(page);
    if (page == PageId::PadFx) c.ui.picker_family = uint8_t(std::min<unsigned>(c.ui.picker_family, unsigned(Family::Count) - 2));
}
inline std::array<Binding, 3> mixer_bindings(const Project& project, const UiState& ui) {
    using K = Binding::Kind; const auto& t = project.tracks[ui.track % track_count];
    if (ui.mixer_menu) return {Binding {K::MixerMenu, 0, 0, 2, ui.mixer_menu_row}, Binding {}, Binding {}};
    if (ui.mixer_view == MixerView::Master) return {Binding {K::MasterLevel, 0, 0, 127, project.master_level}, Binding {K::MasterPan, 0, -50, 50, project.master_pan}, Binding {K::MasterMono, 0, 0, 1, int(project.master_mono)}};
    if (ui.mixer_view == MixerView::Pad) {
        const auto& p = t.pads[t.last_pad % sampler_pads];
        return {Binding {K::PadLevel, 0, 0, 127, p.level}, Binding {K::PadPan, 0, -50, 50, p.pan}, Binding {K::PadSend, 0, 0, 100, p.sends[0]}};
    }
    if (ui.mixer_view == MixerView::Track) return {Binding {K::Level, 0, 0, 127, t.mix.level}, Binding {K::Pan, 0, -50, 50, t.mix.pan}, Binding {K::Send, 0, 0, 100, t.mix.sends[0]}};
    const bool bank = mixer_bank(ui); const auto& p = t.pads[t.last_pad % sampler_pads];
    Binding pick {K::MixerPick, 0, 0, 16, int(mixer_pick(t, ui))};
    if (ui.mixer_master) return {pick, Binding {K::MasterLevel, 0, 0, 127, project.master_level}, Binding {K::MasterPan, 0, -50, 50, project.master_pan}};
    return {pick, Binding {bank ? K::PadLevel : K::Level, 0, 0, 127, bank ? p.level : t.mix.level}, Binding {bank ? K::PadPan : K::Pan, 0, -50, 50, bank ? p.pan : t.mix.pan}};
}
}
