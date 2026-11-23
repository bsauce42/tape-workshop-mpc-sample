// SPDX-License-Identifier: AGPL-3.0-only
// Optional bounded, UI-only metadata extension for Tape Workshop Bloom Reverb.
#pragma once
#include <cstdint>
namespace melodee::bloom_ir {
constexpr int32_t effect_id=0x54574252, opcode=50, command=0x54574952;
enum Status : int32_t { Algorithmic=0, Loading=1, Ready=2, Bad=3, Missing=4 };
struct Info {
    uint32_t version=1;
    int32_t slot=0,status=Algorithmic;
    char filename[256] {};
};
constexpr unsigned parameter(unsigned row,unsigned knob) {
    // IR first, then original banks 0–5, then modulation. The plug-in's ABI is unchanged.
    return (row==0?6u:row<=6?row-1u:7u)*3u+knob;
}
}
