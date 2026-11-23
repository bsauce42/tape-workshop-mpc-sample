// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cstdint>

namespace melodee::ui {

// Five columns by seven rows. Each byte holds one row, most significant row first.
inline constexpr std::array<uint64_t, 95> lcd_ascii {
    0x00000000000000ull, // space
    0x04040404040004ull, // !
    0x0a0a0a00000000ull, // "
    0x0a0a1f0a1f0a0aull, // #
    0x040f140e051e04ull, // $
    0x18190204081303ull, // %
    0x0c12140c15120dull, // &
    0x04040800000000ull, // '
    0x02040808080402ull, // (
    0x08040202020408ull, // )
    0x00150e1f0e1500ull, // *
    0x0004041f040400ull, // +
    0x00000000040408ull, // ,
    0x0000001f000000ull, // -
    0x00000000000c0cull, // .
    0x01010204081010ull, // /
    0x0e11131519110eull, // 0
    0x040c040404040eull, // 1
    0x0e11010204081full, // 2
    0x1e01010e01011eull, // 3
    0x02060a121f0202ull, // 4
    0x1f10101e01011eull, // 5
    0x0e10101e11110eull, // 6
    0x1f010204080808ull, // 7
    0x0e11110e11110eull, // 8
    0x0e11110f01010eull, // 9
    0x000c0c000c0c00ull, // :
    0x00040400040408ull, // ;
    0x02040810080402ull, // <
    0x00001f001f0000ull, // =
    0x08040201020408ull, // >
    0x0e110102040004ull, // ?
    0x0e11171517100eull, // @
    0x0e11111f111111ull, // A
    0x1e11111e11111eull, // B
    0x0f10101010100full, // C
    0x1e11111111111eull, // D
    0x1f10101e10101full, // E
    0x1f10101e101010ull, // F
    0x0e11101711110full, // G
    0x1111111f111111ull, // H
    0x0e04040404040eull, // I
    0x0702020212120cull, // J
    0x11121418141211ull, // K
    0x1010101010101full, // L
    0x111b1515111111ull, // M
    0x11191513111111ull, // N
    0x0e11111111110eull, // O
    0x1e11111e101010ull, // P
    0x0e11111115120dull, // Q
    0x1e11111e141211ull, // R
    0x0f10100e01011eull, // S
    0x1f040404040404ull, // T
    0x1111111111110eull, // U
    0x11111111110a04ull, // V
    0x1111111515150aull, // W
    0x11110a040a1111ull, // X
    0x11110a04040404ull, // Y
    0x1f01020408101full, // Z
    0x0e08080808080eull, // [
    0x10100804020101ull, // backslash
    0x0e02020202020eull, // ]
    0x040a1100000000ull, // ^
    0x0000000000001full, // _
    0x08040200000000ull, // `
    0x00000e010f110full, // a
    0x10101e1111111eull, // b
    0x00000f1010100full, // c
    0x01010f1111110full, // d
    0x00000e111f100eull, // e
    0x0609040e040404ull, // f
    0x00000f110f010eull, // g
    0x10101e11111111ull, // h
    0x04000c0404040eull, // i
    0x0200060202120cull, // j
    0x10101214181412ull, // k
    0x0c04040404040eull, // l
    0x00001a15151515ull, // m
    0x00001e11111111ull, // n
    0x00000e1111110eull, // o
    0x00001e111e1010ull, // p
    0x00000f110f0101ull, // q
    0x00001619101010ull, // r
    0x00000f100e011eull, // s
    0x04041f04040403ull, // t
    0x0000111111110full, // u
    0x00001111110a04ull, // v
    0x0000111115150aull, // w
    0x0000110a040a11ull, // x
    0x000011110f010eull, // y
    0x00001f0204081full, // z
    0x02040408040402ull, // {
    0x04040404040404ull, // |
    0x08040402040408ull, // }
    0x00000916000000ull, // ~
};

inline uint64_t lcd_glyph(int c) {
    if (c >= 32 && c <= 126) return lcd_ascii[unsigned(c - 32)];
    switch (c) {
    case 0x00b0: return 0x06090906000000ull; // degree
    case 0x00b7: return 0x0000000c0c0000ull; // middle dot
    case 0x00d7: return 0x0000110a040a11ull; // multiply
    case 0x2013: case 0x2014: case 0x2212: return lcd_ascii['-' - 32];
    case 0x2026: return 0x00000000000015ull; // ellipsis
    default: return 0;
    }
}

}
