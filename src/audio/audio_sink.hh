#ifndef AUDIO_AUDIO_SINK_HH
#define AUDIO_AUDIO_SINK_HH

#include <string>
#include <vector>
#include <memory>
#include <mutex>

#include <cstdint>

namespace Audio {

// ============================================================================

class AudioSink {
public:

    /// Vitual destructor
    virtual ~AudioSink () {};

    /// List available devices
    virtual const std::vector<std::string> listDevices () = 0;

    /// Open stream
    virtual int open  (const std::string& a_DeviceName, 
                       size_t a_SampleRate,
                       size_t a_Channels,
                       size_t a_FramesPerBuffer) = 0;
    /// Close stream
    virtual void close () = 0;

    /// Start streaming
    virtual bool start () = 0;
    /// Stop streaming
    virtual void stop  () = 0;

    /// Returns the channel count
    size_t getChannels () const;
    /// Returns the sample rate
    size_t getSampleRate () const;
    /// Returns number of frames per buffer
    size_t getFramesPerBuffer () const;

    /// Returns true if the buffer is free
    virtual bool isReady     (int64_t* a_Time = nullptr);
    /// Copies data to the buffer. For multiple channels the data has to
    /// be interleaved.
    virtual void writeBuffer (const float* a_Data) = 0;

protected:

    /// Sample rate
    size_t  m_SampleRate = 0;
    /// Channel count
    size_t  m_Channels = 0;
    /// Frames per buffer
    size_t  m_FramesPerBuffer = 0;

    /// Buffer lock
    std::mutex m_BufferLock;
    /// Buffer valid flag
    bool m_BufferValid = false;
    /// Last played buffer timestamp
    int64_t m_BufferTime = 0;
};

// ============================================================================
}; // Audio

#endif // AUDIO_AUDIO_SINK_HH

