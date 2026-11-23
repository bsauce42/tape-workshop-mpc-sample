// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
// Rubik and the faces Settings > SCREEN > Font offers (each SIL Open Font
// License 1.1, by its own authors, except Melodee Pixel, Melodee's own,
// drawn into build/deps by tools/gen_pixel_font.py), embedded so the binary carries them to the MPC and
// to host previews. src/melodee/dependencies.py fetches the pinned files; the
// assembler reads them from the build folder (make runs at the repository root).
#include "melodee/ui/font_data.h"

asm(R"(
    .section .rodata
    .balign 16
    .global melodee_rubik_start
melodee_rubik_start:
    .incbin "build/deps/rubik/Rubik-VF.ttf"
    .global melodee_rubik_end
melodee_rubik_end:
    .balign 16
    .global melodee_intertight_start
melodee_intertight_start:
    .incbin "build/deps/fonts/intertight/face.ttf"
    .global melodee_intertight_end
melodee_intertight_end:
    .balign 16
    .global melodee_atkinson_start
melodee_atkinson_start:
    .incbin "build/deps/fonts/atkinsonhyperlegiblenext/face.ttf"
    .global melodee_atkinson_end
melodee_atkinson_end:
    .balign 16
    .global melodee_spacegrotesk_start
melodee_spacegrotesk_start:
    .incbin "build/deps/fonts/spacegrotesk/face.ttf"
    .global melodee_spacegrotesk_end
melodee_spacegrotesk_end:
    .balign 16
    .global melodee_plex_start
melodee_plex_start:
    .incbin "build/deps/fonts/ibmplexsans/face.ttf"
    .global melodee_plex_end
melodee_plex_end:
    .balign 16
    .global melodee_manrope_start
melodee_manrope_start:
    .incbin "build/deps/fonts/manrope/face.ttf"
    .global melodee_manrope_end
melodee_manrope_end:
    .balign 16
    .global melodee_pixel_start
melodee_pixel_start:
    .incbin "build/deps/fonts/melodeepixel/MelodeePixel.ttf"
    .global melodee_pixel_end
melodee_pixel_end:
    .balign 16
    .global melodee_vt323_start
melodee_vt323_start:
    .incbin "build/deps/fonts/vt323/face.ttf"
    .global melodee_vt323_end
melodee_vt323_end:
    .balign 16
    .global melodee_pixelnarrow_start
melodee_pixelnarrow_start:
    .incbin "build/deps/fonts/melodeepixel/MelodeePixelNarrow.ttf"
    .global melodee_pixelnarrow_end
melodee_pixelnarrow_end:
    .previous
)");
