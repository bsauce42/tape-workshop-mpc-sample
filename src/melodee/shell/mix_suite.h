// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/engine/effect_info.h"
#include "melodee/shell/bindings.h"
#include "melodee/shell/features.h"
#include "melodee/shell/stage_map.h"
#include <string_view>

namespace melodee {

inline const char* mix_suite_name(const Project& project, const EffectSlot& slot, bool short_name = false) {
    switch (EffectKind(slot.kind)) {
    case EffectKind::Plugin: {
        const unsigned entry = unsigned(slot.params[0]);
        if (entry >= max_plugins) return nullptr;
        const std::string_view name(project.plugins[entry].name);
        if (name == "Prism EQ") return short_name ? "Prism" : "Prism EQ";
        if (name == "Contour Comp") return short_name ? "Contour" : "Contour Comp";
        if (name == "Gate Keeper") return short_name ? "Gate" : "Gate Keeper";
        if (name == "Rack 76") return short_name ? "76" : "Rack 76";
        if (name == "Second Spin" || name == "Second Spin 2") return short_name ? "Spin" : "Second Spin";
        return nullptr;
    }
    case EffectKind::DeEsser: return short_name ? "Silk" : "Silk De-esser";
    case EffectKind::BandCompressor: return short_name ? "Triad" : "Triad Dynamics";
    case EffectKind::Limiter: return short_name ? "Crown" : "Crown Limiter";
    case EffectKind::Reverb: return short_name ? "Halo" : "Halo Reverb";
    case EffectKind::TapeEcho: return short_name ? "Delay" : "Afterimage Delay";
    case EffectKind::Filter: return short_name ? "Ember" : "Ember Filter";
    case EffectKind::TubeDrive: return short_name ? "Copper" : "Copper Drive";
    default: return nullptr;
    }
}

inline unsigned mix_suite_chain(const Project& project, const UiState& ui) {
    if (ui.page == PageId::Rack || ui.page == PageId::Bus || ui.page == PageId::Master || ui.page == PageId::PadFx) return page_chain(project, ui);
    if (ui.page == PageId::Stage) {
        const auto& track = project.tracks[ui.track % track_count];
        if (stage_page_now(ui, track, stage_skip(project, ui, track)).kind == StageKind::Fx) return ui.track;
    }
    return chain_count;
}

inline const EffectSlot* selected_mix_suite_effect(const Project& project, const UiState& ui) {
    if (ui.picker_open || ui.sheet_open) return nullptr;
    const unsigned n = mix_suite_chain(project, ui);
    if (n >= chain_count) return nullptr;
    const auto& chain = project.chain(n);
    const unsigned k = ui.fx_cursor[UiState::cursor_of(n)];
    return k < chain.count() && mix_suite_name(project, chain.slots[k]) ? &chain.slots[k] : nullptr;
}

inline int next_mix_suite_slot(const Project& project, const Chain& chain, unsigned cursor, int direction) {
    const int count = int(chain.count());
    if (count < 2 || cursor >= unsigned(count)) return -1;
    for (int step = 1; step < count; ++step) {
        const unsigned k = unsigned((int(cursor) + direction * step + count) % count);
        if (mix_suite_name(project, chain.slots[k])) return int(k);
    }
    return -1;
}

} // namespace melodee
