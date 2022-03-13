#include "audio_sink.hh"

#include <cstring>

namespace Audio {

// ============================================================================

size_t AudioSink::getChannels () const {
    return m_Channels;
}

size_t AudioSink::getSampleRate () const {
    return m_SampleRate;
}

size_t AudioSink::getFramesPerBuffer () const {
    return m_FramesPerBuffer;
}

// ============================================================================

bool AudioSink::isReady (int64_t* a_Time) {
    std::lock_guard<std::mutex> lock(m_BufferLock);

    // If the pointer is valid return the timestamp
    if (a_Time != nullptr) {
        *a_Time = m_BufferTime;
    }

    // Return the flag
    return !m_BufferValid;
}

// ============================================================================

}; // Audio

