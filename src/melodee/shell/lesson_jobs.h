// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include "melodee/shell/lesson.h"

// A lesson's jobs on the MIDI thread (shell/lesson.h has its rules).
namespace melodee {

class Context;

// LEARN's START: the open project is saved to its file (a project never saved and unchanged is just left), then
// a new project with the factory kit takes its place and the lesson begins. Says why not when the card is away,
// a take runs or an MPC project just opened has no file of its own.
void lesson_start(Context& c, unsigned lesson);
// The lesson ends: LEAVE in the MENU, or its last step done (`done`: it gets its check, kept in the settings). The
// project open before it opens again; the lesson's own is dropped.
void lesson_leave(Context& c, bool done);
// Often: a waiting step ends when the kit has loaded (or the lesson, when it never comes), and a finished lesson
// leaves after showing DONE for a moment.
void lesson_tick(Context& c, uint32_t now);

} // namespace melodee
