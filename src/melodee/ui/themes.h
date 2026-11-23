// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cstdint>

namespace melodee::ui {

enum class ThemeControls { Standard, Boxed, Reversed, Sample };

struct Theme {
    uint8_t saved_id;
    const char* name;
    uint32_t background, surface, ink, accent;
    bool filled_tabs;
    bool pixel_title = false;
    bool lcd_ink = false;
    ThemeControls controls = ThemeControls::Standard;
    bool xl_track_colours = false;
    bool bold_text = false;
};

// Keep saved IDs stable for SD cards from earlier releases.
inline constexpr std::array themes {
    Theme {0, "Melodee Night", 0x17141f, 0x221e2c, 0xf3eef8, 0xffc145, false},
    Theme {1, "MPC60", 0xb7cb36, 0xa4b631, 0x151b05, 0x273108, true, true, true},
    Theme {4, "MPC2000", 0x1536BF, 0x132CAD, 0xECF4FF, 0xDDEFFF, true, true, true, ThemeControls::Boxed},
    Theme {9, "MPC5000", 0xE0E9D5, 0xD6DCCF, 0x452086, 0x5426A3, true, true, true, ThemeControls::Reversed},
    Theme {19, "MPC Sample", 0x000000, 0x101014, 0xF8F8FA, 0xFFBE16, true, false, false, ThemeControls::Sample},
    Theme {22, "MPC XL", 0x242426, 0x303033, 0xF2F1F3, 0xFA478B, false, false, false, ThemeControls::Standard, true},
    Theme {28, "8-Bit", 0x110d34, 0x38215b, 0xfff0c7, 0x55eac1, true, true},
    Theme {29, "Black & Yellow", 0x000000, 0x000000, 0xffd400, 0xffd400, true},
    Theme {30, "Hot Green & Purple", 0x20102F, 0x462369, 0xFCF2FF, 0xB6FF35, true},
    Theme {32, "Hot Green", 0xB6FF35, 0x9CDE27, 0x241030, 0x551B83, true},
    Theme {33, "Tape Workshop Neon", 0x000000, 0x000000, 0xFFFFFF, 0xED38FF, false, false, false, ThemeControls::Standard, false, true},
};

inline unsigned theme_index_from_id(int id) {
    for (unsigned n = 0; n < themes.size(); ++n)
        if (themes[n].saved_id == id) return n;
    return 0;
}
}
