#ifndef UTILS_MATH_HH
#define UTILS_MATH_HH

#include <cstddef>
#include <cstdint>

#include <cmath>

namespace Utils {
namespace Math {

// ============================================================================

/// Signum
inline float sign (const float& x) {
    if ((*((uint32_t*)&x)) & 0x80000000) {
        return -1.0f;
    } else {
        return +1.0f;
    }
}

/// Linear interpolation between two values
inline float lerp (const float& x1, const float& x2, const float& w) {
    return (w - 1.0f) * x1 + w * x2;
}

/// Decibel gain to scaling factor
inline float log2lin (const float& gain) {
    return powf(10.0f, gain / 20.0f);
}

/// Scaling factor to decibel gain
inline float lin2log (const float& k) {
    return 20.0f * log10f(k);
}

// ============================================================================

}; // Math
}; // Utils

#endif // UTILS_MATH_HH

