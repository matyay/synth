#ifndef AUDIO_PORTAUDIO_SINK_HH
#define AUDIO_PORTAUDIO_SINK_HH

#include "audio_sink.hh"

#include <portaudio.h>

namespace Audio {

// ============================================================================

class PortAudioSink : public AudioSink {
public:

    PortAudioSink ();
   ~PortAudioSink ();

    /// List devices
    const std::vector<std::string> listDevices () override;

    /// Open stream
    int  open  (const std::string& a_DeviceName, 
                size_t a_SampleRate,
                size_t a_Channels,
                size_t a_FramesPerBuffer) override;
    /// Close stream
    void close () override;

    /// Start streaming
    bool start () override;
    /// Stop streaming
    void stop  () override;

    /// Copies data to the buffer
    void writeBuffer (const float* a_Data) override;

protected:

    /// Audio stream
    PaStream* m_Stream = nullptr;

    /// Next buffer to be played
    std::unique_ptr<float> m_Buffer;

    /// Portaudio callback
    int         paCallback (const void* inputBuffer, void* outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags);

    static int _paCallback (const void* inputBuffer, void* outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void* userData);
};

// ============================================================================
}; // Audio
#endif // AUDIO_PORTAUDIO_SINK_HH

