// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/shell/context.h"
namespace melodee {
void roll_select_all(Context& c);
bool roll_start_split(Context& c);
void roll_split_step(Context& c, int delta);
void roll_finish_split(Context& c);
}
