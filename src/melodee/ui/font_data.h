// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include <array>
#include <cstddef>
#include "melodee/ui/faces.h"

// The embedded variable font files (font_data.cpp): Rubik, the design's, and
// the faces Settings > SCREEN > Font offers. Every weight comes from a face's
// wght axis.
extern "C" const unsigned char melodee_rubik_start[], melodee_rubik_end[], melodee_intertight_start[], melodee_intertight_end[],
    melodee_atkinson_start[], melodee_atkinson_end[], melodee_spacegrotesk_start[], melodee_spacegrotesk_end[],
    melodee_plex_start[], melodee_plex_end[], melodee_manrope_start[], melodee_manrope_end[],
    melodee_pixel_start[], melodee_pixel_end[],
    melodee_vt323_start[], melodee_vt323_end[], melodee_pixelnarrow_start[], melodee_pixelnarrow_end[];

namespace melodee::ui {
struct FaceData { const unsigned char* start; const unsigned char* end; };
inline const std::array<FaceData, faces.size()> face_data {{
    {melodee_rubik_start, melodee_rubik_end},
    {melodee_intertight_start, melodee_intertight_end},
    {melodee_atkinson_start, melodee_atkinson_end},
    {melodee_spacegrotesk_start, melodee_spacegrotesk_end},
    {melodee_plex_start, melodee_plex_end},
    {melodee_manrope_start, melodee_manrope_end},
    {melodee_pixel_start, melodee_pixel_end},
    {melodee_vt323_start, melodee_vt323_end},
    {melodee_pixelnarrow_start, melodee_pixelnarrow_end},
}};
}
