#ifndef GRAPH_PROCESSING_SAMPLER_HH
#define GRAPH_PROCESSING_SAMPLER_HH

#include <audio/buffer.hh>

#include <string>

#include <cstddef>
#include <cstdint>

namespace Graph {
namespace Processing {

// ============================================================================

class Sampler {
public:

    /// Loads the waveform from an audio file
    void load (const std::string& a_FileName);

    /// Returns sample rate of the waveform in Hz
    float getSampleRate () const {
        return (float)m_SampleRate;
    }

    /// Returns length of the waveform in seconds
    float getLength () const {
        size_t length = m_Waveform.getSize() - 2 * MARGIN;
        return (float)length / (float)m_SampleRate;
    }

    /// Returns a single sample at the given point in time. The "phase" ranges
    /// from 0.0 to 1.0
    float getSample (const float& phi);

private:

    /// Margin size
    static constexpr size_t MARGIN = 2;

    /// Original waveform sample rate
    size_t m_SampleRate = 0;
    /// The waveform
    Audio::Buffer<float> m_Waveform;
};

// ============================================================================

}; // Processing
}; // Graph

#endif // GRAPH_PROCESSING_SAMPLER_HH
