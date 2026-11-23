// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/shell/ui_state.h"
#include "melodee/shell/typed_sample_search.h"
#include "melodee/plugin/preset_rows.h"
namespace melodee {
inline unsigned browser_query_slot(PageId page) { return page == PageId::Project ? 0 : page == PageId::Plugins ? 1 : page == PageId::PluginPresets ? 3 : 2; }
inline std::string browser_query(const UiState& ui) {
    return ui.name.target == NameTarget::BrowserSearch ? std::string(ui.name.text, ui.name.length) : ui.browser_queries[browser_query_slot(ui.page)];
}
inline std::vector<PresetRow> browser_preset_rows(const PluginProgramList& list, std::string_view inside, int current, const UiState& ui) {
    auto rows = preset_rows(list, inside, current);
    filter_browser_files(rows, browser_query(ui));
    return rows;
}
}
