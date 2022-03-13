#include "waveform.hh"

#include <cmath>

namespace Graph {
namespace Processing {

// ============================================================================

#define _2PI 6.283185307f

namespace Waveform {

float sine (float x, float arg) {
    (void)arg;
    return sinf(x * _2PI);
}

float half_sine(float x, float arg) {
    (void)arg;
    return (x < 0.5f) ? sinf(x * _2PI) : 0.0f;
}

float abs_sine(float x, float arg) {
    return fabs(sine(x, arg));
}

float pulse_sine(float x, float arg) {
    (void)arg;

    if (x > 0.5f) {
        x -= 0.5f;
    }

    return (x < 0.25f) ? sinf(x * _2PI) : 0.0f;
}

float even_sine(float x, float arg) {
    (void)arg;
    return (x < 0.5f) ? sinf(x * 2.0f * _2PI) : 0.0f;
}

float even_abs_sine(float x, float arg) {
    (void)arg;
    return (x < 0.5f) ? fabs(sinf(x * 2.0f * _2PI)) : 0.0f;
}

float square (float x, float arg) {
    return (x < arg) ? +1.0f : -1.0f;
}

float derived_square (float x, float arg) {
    const float g = 20.0 - 10.0 * arg;
    if (x < 0.5f) {
        float f = 2.0f * x;
        return -expf(-f * g);
    } else {
        float f = 2.0f * (1.0f - x);
        return +expf(-f * g);
    }
}

float triangle (float x, float arg) {
    if (x < arg) {
        float w = x / arg;
        return -1.0f * (1.0f - w) + 1.0f * w;
    } else {
        float w = (x - arg) / (1.0f - arg);
        return +1.0f * (1.0f - w) - 1.0f * w;
    }
}

float sawtooth (float x, float arg) {
    (void)arg;
    return 2.0f * (x - 0.5f);
}

}; // Waveform

// ============================================================================

}; // Processing
}; // Graph

