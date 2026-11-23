// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include <array>

// The typefaces Settings > SCREEN > Font offers, in ui/font_data.h's order (no
// font data here, so Settings builds in the host tests too). A pixel face is
// drawn only at whole multiples of its grid: `em` screen pixels to the em at
// 1x, its capitals `cap` pixels tall (ui/paint.cpp picks the multiple whose
// capitals come nearest Rubik's at the size asked for). Outline faces have 0.
// A pixel face can name a narrower companion (`narrow`, an index here, -1
// none) that a string too wide for its box is set in instead; companions sit
// after the choices and are not offered themselves.
namespace melodee::ui {
struct Face { const char* name; float em, cap; int narrow = -1; float scale = 1; }; // scale: an outline face's size against Rubik's, so its capitals match
constexpr unsigned face_choices = 8; // the first eight: Settings' Font
constexpr std::array<Face, 9> faces {{
    {"Rubik", 0, 0},
    {"Inter Tight", 0, 0},
    {"Atkinson", 0, 0},
    {"Space Grotesk", 0, 0},
    {"IBM Plex", 0, 0},
    {"Manrope", 0, 0},
    {"Melodee Pixel", 10, 7, 8},       // tools/gen_pixel_font.py: 100 units a pixel, 1000 to the em
    {"VT323", 0, 0, -1, 1.25f},        // a terminal's pixel letters, narrow; capitals 0.56 em
    {"Melodee Pixel Narrow", 10, 7},   // FM-1's narrow letters (tools/gen_pixel_font.py)
}};
}
