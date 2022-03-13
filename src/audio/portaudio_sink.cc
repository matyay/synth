#include "exception.hh"
#include "portaudio_sink.hh"

#include <utils/exception.hh>
#include <utils/utils.hh>
#include <utils/logging.hh>

#include <stringf.hh>

#include <stdexcept>
#include <cstring>

namespace Audio {

// ============================================================================

PortAudioSink::PortAudioSink ()
{
    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        THROW(DeviceError, "Error initializing PortAudio: '%s'",
            Pa_GetErrorText(err));
    }

    auto logger = getLogger("portaudio");
    logger->info("PortAudio initialized.");
}

PortAudioSink::~PortAudioSink()
{
    // Close the device
    close();

    // Shutdown portaudio
    Pa_Terminate();

    auto logger = getLogger("portaudio");
    logger->info("PortAudio shutdown.");
}

// ============================================================================

const std::vector<std::string> PortAudioSink::listDevices () {

    // List devices
    std::vector<std::string> devices;
    for (PaDeviceIndex i=0; i<Pa_GetDeviceCount(); ++i) {

        // Get info
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);

        // Ignore devices that don't have outputs
        if (info->maxOutputChannels <= 0) {
            continue;
        }

        // Add to list
        devices.push_back(std::string(info->name));
    }

    return devices;
}

// ============================================================================

int PortAudioSink::open (const std::string& a_DeviceName,
                         size_t a_SampleRate,
                         size_t a_Channels,
                         size_t a_FramesPerBuffer)
{
    auto logger = getLogger("portaudio");

    PaError             err;
    PaStreamParameters  outputParameters;
    PaDeviceIndex       deviceIndex = -1;

    // Already open
    if (m_Stream != nullptr) {
        return -1;
    }

    // Find device
    for (PaDeviceIndex i=0; i<Pa_GetDeviceCount(); ++i) {

        // Get info
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);

        // Check name
        if (a_DeviceName == std::string(info->name)) {
            deviceIndex = i;
            break;
        }
    }

    // Device not found
    if (deviceIndex == -1) {
        logger->error("Couldn't find device '{}'", a_DeviceName);
        return 1;
    }

    // Stream parameters
    memset(&outputParameters, 0, sizeof(PaStreamParameters));
    outputParameters.device           = deviceIndex;
    outputParameters.channelCount     = a_Channels;
    outputParameters.sampleFormat     = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(deviceIndex)->defaultLowInputLatency;

    // Open audio stream
    err = Pa_OpenStream(
            &m_Stream,
            NULL,
            &outputParameters,
            a_SampleRate,
            a_FramesPerBuffer,
            paNoFlag,
            PortAudioSink::_paCallback,
            (void*)this);

    if (err != paNoError) {
        return -1;
    }

    // Check the parameters
    logger->debug("Device       : {} (index={})", a_DeviceName, deviceIndex);
    logger->debug("Sample rate  : {}", a_SampleRate);
    logger->debug("Channels     : {}", a_Channels);
    logger->debug("Frames/buffer: {}", a_FramesPerBuffer);

    // Create the buffer
    size_t size = sizeof(float) * a_Channels * a_FramesPerBuffer;
    m_Buffer.reset(new float[size]);

    m_BufferValid = false;

    // Set parameters
    m_SampleRate        = a_SampleRate;
    m_Channels          = a_Channels;
    m_FramesPerBuffer   = a_FramesPerBuffer;

    return 0;
}

void PortAudioSink::close ()
{
    // Not open
    if (m_Stream == nullptr) {
        return;
    }

    // Stop
    stop();

    // Close the steam
    Pa_CloseStream(m_Stream);
    m_Stream = nullptr;
}

bool PortAudioSink::start ()
{
    PaError err;

    // Not open
    if (m_Stream == nullptr) {
        return false;
    }
    // Aleready running
    if (Pa_IsStreamStopped(m_Stream) == 0) {
        return false;
    }

    // Start the stream
    err = Pa_StartStream(m_Stream);
    if (err != paNoError) {
        return false;
    }

    return true;
}

void PortAudioSink::stop ()
{
    // Not open
    if (m_Stream == nullptr) {
        return;
    }
    // Not running
    if (Pa_IsStreamStopped(m_Stream) != 0) {
        return;
    }

    // Stop
    Pa_StopStream(m_Stream);

    // Set parameters
    m_FramesPerBuffer = 0;
}

// ============================================================================

void PortAudioSink::writeBuffer (const float* a_Data) {
    size_t size = sizeof(float) * m_Channels * m_FramesPerBuffer;
    std::lock_guard<std::mutex> lock(m_BufferLock);

    // Buffer already valid, do not overwrite
    if (m_BufferValid) {
        return;
    }

    memcpy(m_Buffer.get(), a_Data, size);
    m_BufferValid = true;
}

// ============================================================================

int PortAudioSink::paCallback  (const void* inputBuffer, void* outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags)
{
    (void)inputBuffer;
    (void)framesPerBuffer;
    (void)timeInfo;
    (void)statusFlags;

    size_t  size = sizeof(float) * m_Channels * m_FramesPerBuffer;
    int64_t now  = Utils::makeTimestamp();

    // If the buffer is valid then copy its content to the audio buffer and
    // update the timestamp.
    {
        std::lock_guard<std::mutex> lock(m_BufferLock);


        // Copy if valid
        if (m_BufferValid) {
            memcpy(outputBuffer, m_Buffer.get(), size);

            m_BufferValid = false;
            m_BufferTime  = now;

            return paContinue;
        }
    }

    // The buffer is not valid, send all zeros
    memset(outputBuffer, 0, size);

    auto logger = getLogger("portaudio");
    logger->warn("[{:07.1f}] Dropped {} frames",
        (double)now * 1e-3,
        m_FramesPerBuffer
    );

    return paContinue;
}


int PortAudioSink::_paCallback (const void* inputBuffer, void* outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void* userData)
{
    // No context
    if (!userData) {
        return paAbort;
    }

    PortAudioSink* ctx = (PortAudioSink*)userData;

    return ctx->paCallback(
        inputBuffer,
        outputBuffer,
        framesPerBuffer,
        timeInfo,
        statusFlags
    );
}


// ============================================================================

}; // Audio

