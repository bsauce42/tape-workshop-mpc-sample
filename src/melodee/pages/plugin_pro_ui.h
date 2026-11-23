// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Tape Workshop contributors
#pragma once
#include "melodee/pages/common.h"
namespace melodee {
bool draw_plugin_pro_ui(ui::Paint&,const Snapshot&,float x,float y,float w,float h,bool listening=false,bool bypass=false);
}
