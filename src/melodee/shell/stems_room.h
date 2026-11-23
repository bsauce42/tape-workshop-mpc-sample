// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include "melodee/model/project.h"
#include "melodee/shell/stems.h"

// What a STEMS job (shell/stems_jobs.cpp) needs of the project, and what there is: a job lands four new samples on
// four empty pads of the track. Pure, for the shell, the sheet and the tests.
namespace melodee {

struct StemsRoom {
    unsigned pads = 0;    // empty pads of the track after the pad, going round: as many as a job needs at most
    bool enough() const { return pads >= stem_count; }
};

inline StemsRoom stems_room(const Project& project, unsigned track, unsigned pad) {
    StemsRoom room;
    const auto& t = project.tracks[track % track_count];
    for (unsigned k = 1; k < sampler_pads && room.pads < stem_count; ++k) room.pads += !t.pads[(pad + k) % sampler_pads].used();
    return room;
}

} // namespace melodee
