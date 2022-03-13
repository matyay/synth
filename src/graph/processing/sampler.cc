#include "sampler.hh"

#include <utils/exception.hh>
#include <utils/math.hh>

#include <stringf.hh>

#include <sndfile.h>

#include <cmath>
#include <cassert>

namespace Graph {
namespace Processing {

// ============================================================================

void Sampler::load (const std::string& a_FileName) {

    // Open the audio file
    SF_INFO info;
    memset(&info, 0, sizeof(SF_INFO));

    SNDFILE* sf = sf_open(a_FileName.c_str(), SFM_READ, &info);
    if (sf == nullptr) {
        THROW(std::runtime_error, "Error opening audio file '%s'", a_FileName.c_str());
    }

    if (info.channels < 1 || info.channels > 2) {
        sf_close(sf);
        THROW(std::runtime_error, "The audio file '%s' is neither mono nor stereo", a_FileName.c_str());
    }

    // FIXME: Allow stereo samples
    if (info.channels != 1) {
        sf_close(sf);
        THROW(std::runtime_error, "The audio file '%s' is stereo which is not supported yet", a_FileName.c_str());
    }

    // Get the file size
    sf_count_t numFrames = sf_seek(sf, 0, SEEK_END);

    // Allocate a temporary buffer
    size_t bufferSize = numFrames * info.channels * sizeof(float);
    std::unique_ptr<float> buffer(new float[bufferSize]);

    sf_command(sf, SFC_SET_NORM_FLOAT, NULL, SF_TRUE);

    // Read data and close the file
    sf_seek(sf, 0, SEEK_SET);
    sf_count_t read = sf_readf_float(sf, buffer.get(), numFrames);
    sf_close(sf);

    if (read < numFrames) {
        THROW(std::runtime_error, "Error reading audio file '%s'", a_FileName.c_str());
    }

    // Create the buffer with magins
    m_Waveform.create(numFrames + 2*MARGIN, info.channels);

    // Copy data. Deinterleave samples and create margins for regular sample
    // lookup during interpolation.
    for (size_t c=0; c<m_Waveform.getChannels(); ++c) {
        const float* src = buffer.get() + c;
        float*       dst = m_Waveform.data(c);
        size_t       dj  = m_Waveform.getChannels();

        for (int32_t i=0; i<(int32_t)m_Waveform.getSize(); ++i) {
            int32_t j = i - MARGIN;

            // Wrap around
            if (j < 0) {
                j += numFrames;
            }
            if (j >= numFrames) {
                j -= numFrames;
            }

            *dst++ = src[j * dj];
        }
    }

    // Store info
    m_SampleRate = info.samplerate;
}

float Sampler::getSample (const float& phi) {
    assert(phi >= 0.0f);
    assert(phi <= 1.0f);

    // Actual audio length
    size_t length = m_Waveform.getSize() - 2 * MARGIN;

    // Break into integer and fraction
    double  i_f;
    float   f = modf(phi * length, &i_f);
    size_t  i = (size_t)i_f;

    // Actual audio pointer
    const float* ptr = m_Waveform.data() + MARGIN + i;

    // Compute coeffs
    float fp2 = f * f;
    float fd6 = f / 6.0f;
    float fd2 = f / 2.0f;

    // Compute the sample
    float sample = ptr[0] +
        ptr[-1] * fd6 * (-fp2 + 3.0f * f - 2.0f) +
        ptr[ 0] * fd2 * ( fp2 - 2.0f * f - 1.0f) +
        ptr[ 1] * fd2 * (-fp2 + 1.0f * f + 2.0f) +
        ptr[ 2] * fd6 * ( fp2            - 1.0f);

    return sample;
}

// ============================================================================

}; // Processing
}; // Graph

