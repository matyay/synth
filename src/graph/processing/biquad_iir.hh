#ifndef GRAPH_PROCESSING_BIQUAD_IIR_HH
#define GRAPH_PROCESSING_BIQUAD_IIR_HH

#include <cstddef>
#include <cstdint>

namespace Graph {
namespace Processing {

// ============================================================================

class BiquadIIR {
public:

    /// Filter coefficients
    struct Coeffs {
        float a0, a1, a2;
        float b0, b1, b2;
    };

    /// Default constructor
    BiquadIIR () = default;
    /// Constructs using given coefficients
    BiquadIIR (const Coeffs& a_Coeffs);

    /// Sets new coefficients
    void setCoeffs (const Coeffs& a_Coeffs);
    /// Resets the filter state
    void reset ();

    /// Process an audio buffer
    void  process (float* a_Out, const float* a_In, size_t a_Length);
    /// Processes a single sample
    float process (float a_Sample);

    /// Compute a lowpass filter
    static const Coeffs computeLPF   (float f0, float gain, float Q, float fs);
    /// Compute a highpass filter
    static const Coeffs computeHPF   (float f0, float gain, float Q, float fs);
    /// Compute a bandpass filter
    static const Coeffs computeBPF   (float f0, float gain, float Q, float fs);
    /// Compute a bandstop (notch) filter
    static const Coeffs computeNotch (float f0, float gain, float Q, float fs);
    /// Compute an allpas filter
    static const Coeffs computeAPF   (float f0, float gain, float Q, float fs);

    /// Compute a peaking filter
    static const Coeffs computePeak      (float f0, float gain, float Q, float fs);
    /// Compute a low shelving filter
    static const Coeffs computeLowShelf  (float f0, float gain, float Q, float fs);
    /// Compute a high shelving filter
    static const Coeffs computeHighShelf (float f0, float gain, float Q, float fs);

private:

    /// Coefficients
    Coeffs m_Coeffs;

    /// State vector
    struct {
        float w1 = 0.0f;
        float w2 = 0.0f;
    } m_State;
};

// ============================================================================

}; // Processing
}; // Graph

#endif // GRAPH_PROCESSING_BIQUAD_IIR_HH

