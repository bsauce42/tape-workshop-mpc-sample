#pragma once

static inline float contour_reduction(float level, float threshold, float ratio, float knee,
                                      float range, float scale)
{
    const float slope = 1 - 1 / ratio;
    const float over = level - threshold;
    float amount = over > 0 ? over : 0;
    if (knee > 0 && over > -.5f * knee && over < .5f * knee) {
        const float edge = over + .5f * knee;
        amount = edge * edge / (2 * knee);
    }
    amount *= slope * scale;
    return amount < range ? amount : range;
}
