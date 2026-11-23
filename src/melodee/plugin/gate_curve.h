#pragma once

enum { GATE_CLASSIC, GATE_CLEAN, GATE_VOCAL, GATE_GUITAR, GATE_UPWARD, GATE_DUCKING };

static inline float gate_gain_db(float level, float threshold, float ratio, float range, float knee, int style)
{
    const float difference = style >= GATE_UPWARD ? level - threshold : threshold - level;
    float amount = difference > 0 ? difference : 0;
    if (knee > 0 && difference > -.5f * knee && difference < .5f * knee) {
        const float edge = difference + .5f * knee;
        amount = edge * edge / (2 * knee);
    }
    amount *= ratio - 1;
    if (amount > range) amount = range;
    return style == GATE_UPWARD ? amount : -amount;
}
