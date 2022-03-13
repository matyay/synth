#ifndef AUDIO_ALSA_SINK_HH
#define AUDIO_ALSA_SINK_HH

#include "audio_sink.hh"
#include <utils/worker.hh>

#include <alsa/asoundlib.h>

namespace Audio {

// ============================================================================

class AlsaSink : public AudioSink, public Worker {
public:

    ~AlsaSink ();

    /// List devices
    const std::vector<std::string> listDevices () override;

    /// Open stream
    int open   (const std::string& a_DeviceName, 
                size_t a_SampleRate,
                size_t a_Channels,
                size_t a_FramesPerBuffer) override;
    /// Close stream
    void close () override;

    /// Start streaming
    bool start () override;
    /// Stop streaming
    void stop  () override;

    /// Returns true if the buffer is free
    bool isReady     (int64_t* a_Time) override;
    /// Copies data to the buffer
    void writeBuffer (const float* a_Data) override;

protected:

    /// The work loop function
    int loop ();
    /// Checks the stream state
    bool checkState (snd_pcm_state_t a_State);

    /// ALSA stream handle
    snd_pcm_t* m_Stream = nullptr;
    /// ALSA stream status
    snd_pcm_status_t* m_Status = nullptr;

    /// Audio buffers
    std::unique_ptr<int16_t> m_CurrBuffer;
    std::unique_ptr<int16_t> m_NextBuffer;

    /// Audio sample format
    snd_pcm_format_t m_Format = SND_PCM_FORMAT_UNKNOWN;
};

// ============================================================================
}; // Audio

#endif // AUDIO_ALSA_SINK_HH

