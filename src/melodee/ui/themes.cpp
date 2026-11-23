// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/ui/themes.h"
#include "melodee/ui/tokens.h"

namespace melodee::ui {
namespace {
thread_local unsigned active = 0;
}

bool themed() { return active != 0; }
bool bold_theme_text() { return themes[active].bold_text; }
bool pixel_theme() { return themes[active].pixel_title; }
bool lcd_ink_theme() { return themes[active].lcd_ink; }
bool boxed_theme_controls() {
    const auto controls = themes[active].controls;
    return controls == ThemeControls::Boxed || controls == ThemeControls::Reversed;
}
bool sample_theme_controls() { return themes[active].controls == ThemeControls::Sample; }
bool reversed_theme_controls() { return themes[active].controls == ThemeControls::Reversed; }
bool filled_theme_tabs() { return themed() && themes[active].filled_tabs; }
juce::Colour theme_accent() { return hex(themes[active].accent); }

juce::Colour knob_shade(unsigned track, unsigned knob) {
    constexpr std::array<std::array<uint32_t, 3>, 4> xl {{
        {0x548BFF, 0x79A6FF, 0x5EB8FF},
        {0xFA4778, 0xFF729B, 0xFF6671},
        {0xF0CD4C, 0xFFE07D, 0xFFB464},
        {0x2EE994, 0x65F7AF, 0x58DCB4},
    }};
    if (themes[active].saved_id == 33) return hex(workshop_track_shades[track % 4][knob % 3]);
    if (sample_theme_controls()) {
        constexpr uint32_t shades[] {0xE3E3E7, 0xF8F8FA, 0x49B9FF};
        return hex(shades[knob % 3]);
    }
    track %= track_shades.size();
    knob %= track_shades.front().size();
    if (themes[active].xl_track_colours && track < xl.size()) return hex(xl[track][knob]);
    return active ? pad_track_ink(track_shades[track][knob]) : hex(palette().tracks[track][knob]);
}

void apply_theme(unsigned index) {
    if (index >= themes.size()) index = 0;
    if (index == active && !index) return;
    active = index;
    if (!active) {
        const auto previous = palette_in_use;
        palette_in_use = ~0u;
        use_palette(previous);
        return;
    }
    using namespace colour;
    const auto& theme = themes[index];
    const bool two_colour = theme.background == theme.surface;
    bg = hex(theme.background);
    surf = hex(theme.surface);
    text = hex(theme.ink);
    const auto accent = theme_accent();
    panel = index == 0 ? hex(0x1D1926) : bg.interpolatedWith(surf, 0.55f);
    quiet = index == 0 ? hex(0x1A1722) : bg.interpolatedWith(surf, 0.25f);
    raised = index == 0 ? hex(0x262033) : two_colour ? bg : surf.interpolatedWith(text, 0.10f);
    line = index == 0 ? hex(0x2E2939) : two_colour ? text : surf.interpolatedWith(text, 0.32f);
    sec = index == 0 ? hex(0xC9C2D6) : two_colour ? text : bg.interpolatedWith(text, 0.88f);
    mid = index == 0 ? hex(0x9A93A8) : two_colour ? text : bg.interpolatedWith(text, 0.75f);
    dim = index == 0 ? hex(0x5A5468) : two_colour ? text : bg.interpolatedWith(text, 0.49f);
    rec = index == 0 ? hex(0xFF4848) : accent;
    amber = index == 0 ? hex(0xFFC145) : accent;
    play = index == 0 ? hex(0x5FD97A) : accent;
    ice = index == 0 ? hex(0xBFE3FF) : text;
    if(theme.saved_id==33){
        panel=quiet=raised=bg;line=hex(0x402956);
        sec=hex(0xF3E9FF);mid=hex(0xB8DEFF);dim=hex(0xA48BC6);
        rec=hex(0xFF457D);play=hex(0xAE71FF);amber=hex(0xED38FF);ice=text;
    }
}
}
