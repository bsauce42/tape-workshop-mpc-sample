// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "surface/leds.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace melodee::ui {
// All sixteen pads sweep from the upper row down during the splash.
// Existing splash timing, skip and brightness settings still own its lifetime.
inline std::array<Rgb,16> boot_pad_lights(uint32_t age,uint32_t fade_at,uint32_t black_ms,
                                         uint32_t logo_ms,uint32_t logo_hold_ms,uint32_t backdrop_ms,
                                         uint32_t out_ms){
    std::array<Rgb,16> pads{};
    const auto smooth=[](float x){x=std::clamp(x,0.f,1.f);return x*x*(3.f-2.f*x);};
    if(age<black_ms||age>=fade_at+out_ms)return pads;
    const float time=float(std::min(age,fade_at)-black_ms),sweep=time/std::max(logo_ms,1u)*7.f;
    const float hold=smooth((time-logo_ms-logo_hold_ms)/std::max(backdrop_ms,1u));
    const float opacity=age<fade_at?1.f:1.f-smooth(float(age-fade_at)/std::max(out_ms,1u));
    constexpr Rgb colours[]={{0xA4,0x65,0xFF},{0xEF,0x38,0xFF},{0xCE,0x70,0xFF},{0x87,0x55,0xFF}};
    for (unsigned pad = 0; pad < 16; ++pad) {
        const unsigned row = 3 - pad / 4, col = pad % 4;
        const float arrival = smooth((sweep - row - col * .5f) * 1.5f);
        const float wave = .5f + .5f * std::sin(time * .006f - row * .8f - col * .6f);
        const float light = opacity * arrival * (.20f + .22f * wave + .06f * hold);
        const auto c = colours[(row + col) % 4];
        pads[pad] = {uint8_t(c.r * light), uint8_t(c.g * light), uint8_t(c.b * light)};
    }
    return pads;
}
}
