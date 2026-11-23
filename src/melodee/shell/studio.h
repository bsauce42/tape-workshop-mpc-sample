// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <cstdint>
namespace melodee {
class Context;
// MIDI-thread project operations, under Shell's normal lock, never the audio callback.
void studio_tick(Context& context, uint32_t now);
std::string studio_command(Context& context, const std::string& request);
}
