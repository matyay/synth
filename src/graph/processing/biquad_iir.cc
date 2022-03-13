#include "biquad_iir.hh"
#include <utils/math.hh>

#include <cmath>

namespace Graph {
namespace Processing {

using namespace Utils;

// ============================================================================

BiquadIIR::BiquadIIR (const Coeffs& a_Coeffs) {
    setCoeffs(a_Coeffs);
}

// ============================================================================

void BiquadIIR::setCoeffs (const Coeffs& a_Coeffs) {

    // Store
    m_Coeffs = a_Coeffs;

    // Normalize
    m_Coeffs.b2 /= m_Coeffs.a0;
    m_Coeffs.b1 /= m_Coeffs.a0;
    m_Coeffs.b0 /= m_Coeffs.a0;
    m_Coeffs.a2 /= m_Coeffs.a0;
    m_Coeffs.a1 /= m_Coeffs.a0;
    m_Coeffs.a0  = 1.0f;
}

void BiquadIIR::reset () {

    // Reset the state vector
    m_State.w1 = 0.0f;
    m_State.w2 = 0.0f;
}

// ============================================================================

float BiquadIIR::process (float a_Sample) {

    // Recursive part
    float w =  (a_Sample - m_Coeffs.a1 * m_State.w1 - m_Coeffs.a2 * m_State.w2);

    // FIR part
    float y = \
        (m_Coeffs.b0 * w + m_Coeffs.b1 * m_State.w1 + m_Coeffs.b2 * m_State.w2);

    // Update the state vector
    m_State.w2 = m_State.w1;
    m_State.w1 = w;

    return y;
}

void BiquadIIR::process (float* a_Out, const float* a_In, size_t a_Length) {

    // Process a whole buffer
    for (size_t i=0; i<a_Length; ++i) {
        *a_Out++ = process(*a_In++);
    }
}

// ============================================================================

// https://github.com/libaudioverse/libaudioverse/blob/master/audio%20eq%20cookbook.txt

#define COMPUTE_A \
    float A     = Math::log2lin(gain * 0.5f);

#define COMPUTE_SIN_COS_W \
    float w     = 6.2831f * f0 / fs; \
    float cosw  = cosf(w); \
    float sinw  = sinf(w);

#define COMPUTE_ALPHA_Q \
    float alpha = sinw / (2.0f * Q);

#define COMPUTE_BETA_Q \
    float beta  = sqrtf(A) / Q;

// ============================================================================

const BiquadIIR::Coeffs BiquadIIR::computeLPF(float f0, float gain, float Q, float fs) {
    COMPUTE_SIN_COS_W
    COMPUTE_ALPHA_Q

    (void)gain;

    Coeffs cf;
    cf.b0 = (1.0f - cosw) / 2.0f;
    cf.b1 =  1.0f - cosw;
    cf.b2 = (1.0f - cosw) / 2.0f;
    cf.a0 =  1.0f + alpha;
    cf.a1 = -2.0f * cosw;
    cf.a2 =  1.0f - alpha;

    return cf;
}

const BiquadIIR::Coeffs BiquadIIR::computeHPF(float f0, float gain, float Q, float fs) {
    COMPUTE_SIN_COS_W
    COMPUTE_ALPHA_Q

    (void)gain;

    // FIXME: The -3db frequency seems to be off!
    Coeffs cf;
    cf.b0 =  (1.0f + cosw) / 2.0f;
    cf.b1 = -(1.0f + cosw);
    cf.b2 =  (1.0f + cosw) / 2.0f;
    cf.a0 =   1.0f + alpha;
    cf.a1 =  -2.0f * cosw;
    cf.a2 =   1.0f - alpha;

    return cf;
}

const BiquadIIR::Coeffs BiquadIIR::computeBPF(float f0, float gain, float Q, float fs) {
    COMPUTE_SIN_COS_W
    COMPUTE_ALPHA_Q

    (void)gain;

    Coeffs cf;
    cf.b0 =  alpha;
    cf.b1 =  0.0f;
    cf.b2 = -alpha;
    cf.a0 =  1.0f + alpha;
    cf.a1 = -2.0f * cosw;
    cf.a2 =  1.0f - alpha;

    return cf;
}

const BiquadIIR::Coeffs BiquadIIR::computeNotch(float f0, float gain, float Q, float fs) {
    COMPUTE_SIN_COS_W
    COMPUTE_ALPHA_Q

    (void)gain;

    Coeffs cf;
    cf.b0 =  1.0f;
    cf.b1 = -2.0f * cosw;
    cf.b2 =  1.0f;
    cf.a0 =  1.0f + alpha;
    cf.a1 = -2.0f * cosw;
    cf.a2 =  1.0f - alpha;

    return cf;
}

const BiquadIIR::Coeffs BiquadIIR::computeAPF(float f0, float gain, float Q, float fs) {
    COMPUTE_SIN_COS_W
    COMPUTE_ALPHA_Q

    (void)gain;

    Coeffs cf;
    cf.b0 =  1.0f - alpha;
    cf.b1 = -2.0f * cosw;
    cf.b2 =  1.0f + alpha;
    cf.a0 =  1.0f + alpha;
    cf.a1 = -2.0f * cosw;
    cf.a2 =  1.0f - alpha;

    return cf;
}

const BiquadIIR::Coeffs BiquadIIR::computePeak(float f0, float gain, float Q, float fs) {
    COMPUTE_A
    COMPUTE_SIN_COS_W
    COMPUTE_ALPHA_Q

    Coeffs cf;
    cf.b0 =  1.0f + alpha * A;
    cf.b1 = -2.0f * cosw;
    cf.b2 =  1.0f - alpha * A;
    cf.a0 =  1.0f + alpha / A;
    cf.a1 = -2.0f * cosw;
    cf.a2 =  1.0f - alpha / A;

    return cf;
}

const BiquadIIR::Coeffs BiquadIIR::computeLowShelf(float f0, float gain, float Q, float fs) {
    COMPUTE_A
    COMPUTE_SIN_COS_W
    COMPUTE_BETA_Q

    Coeffs cf;
    cf.b0 =         A * ((A + 1.0f) - (A - 1.0f) * cosw + beta * sinw);
    cf.b1 =  2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw);
    cf.b2 =         A * ((A + 1.0f) - (A - 1.0f) * cosw - beta * sinw);
    cf.a0 =              (A + 1.0f) + (A - 1.0f) * cosw + beta * sinw;
    cf.a1 =     -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw);
    cf.a2 =              (A + 1.0f) + (A - 1.0f) * cosw - beta * sinw;

    return cf;
}

const BiquadIIR::Coeffs BiquadIIR::computeHighShelf(float f0, float gain, float Q, float fs) {
    COMPUTE_A
    COMPUTE_SIN_COS_W
    COMPUTE_BETA_Q

    Coeffs cf;
    cf.b0 =         A * ((A + 1.0f) + (A - 1.0f) * cosw + beta * sinw);
    cf.b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw);
    cf.b2 =         A * ((A + 1.0f) + (A - 1.0f) * cosw - beta * sinw);
    cf.a0 =              (A + 1.0f) - (A - 1.0f) * cosw + beta * sinw;
    cf.a1 =      2.0f * ((A - 1.0f) - (A + 1.0f) * cosw);
    cf.a2 =              (A + 1.0f) - (A - 1.0f) * cosw - beta * sinw;

    return cf;
}

// ============================================================================

}; // Processing
}; // Graph

