#include "exception.hh"
#include "alsa_sink.hh"

#include <utils/exception.hh>
#include <utils/utils.hh>
#include <utils/logging.hh>

#include <stringf.hh>

#include <stdexcept>
#include <cstring>

namespace Audio {

// ============================================================================

AlsaSink::~AlsaSink()
{
    // Close the device
    close();
}

// ============================================================================

const std::vector<std::string> AlsaSink::listDevices () {

    std::vector<std::string> devices;
    
    int  res;
    auto logger = getLogger("alsa");

    // List devices
    char** hints = nullptr;
    res = snd_device_name_hint(-1, "pcm", (void***)&hints);
    if (res) {
        logger->error("snd_device_name_hint() Failed! {}", snd_strerror(res));
        return devices;
    }

    // Get information
    for (char** h = hints; *h != nullptr; h++) {
        char* name = snd_device_name_get_hint(*h, "NAME");
        if (name != nullptr) {
            devices.push_back(std::string(name));
            free(name);
        }
    }

    snd_device_name_free_hint((void**)hints);
    return devices;
}

// ============================================================================

int AlsaSink::open (const std::string& a_DeviceName,
                    size_t a_SampleRate,
                    size_t a_Channels,
                    size_t a_FramesPerBuffer)
{
    // Already open
    if (m_Stream != nullptr) {
        return false;
    }

    int  res;
    auto logger = getLogger("alsa");

    // Open the device
    res = snd_pcm_open(&m_Stream, a_DeviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (res) {
        logger->error("snd_pcm_open() Failed! {}", snd_strerror(res));
        return (res == -ENOENT) ? 1 : -1;
    }

    // Initialize status
    res = snd_pcm_status_malloc(&m_Status);
    if (res) {
        logger->error("snd_pcm_status_malloc() Failed! {}", snd_strerror(res));
        snd_pcm_close(m_Stream);
        return -1;
    }

    // Initialize default parameters
    snd_pcm_hw_params_t* params = nullptr;

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(m_Stream, params);

    // Set the parameters
    res = snd_pcm_hw_params_set_access(m_Stream, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (res) {
        logger->error("snd_pcm_hw_set_params_set_access() Failed! {}", snd_strerror(res));
        snd_pcm_close(m_Stream);
        return -1;
    }

    res = snd_pcm_hw_params_set_format(m_Stream, params, SND_PCM_FORMAT_S16_LE);
    if (res) {
        logger->error("snd_pcm_hw_set_params_set_format() Failed! {}", snd_strerror(res));
        snd_pcm_close(m_Stream);
        return -1;
    }

    res = snd_pcm_hw_params_set_channels(m_Stream, params, a_Channels);
    if (res) {
        logger->error("snd_pcm_hw_set_params_set_channels() Failed! {}", snd_strerror(res));
        snd_pcm_close(m_Stream);
        return -1;
    }

    unsigned int actualSampleRate = (unsigned int)a_SampleRate;
    res = snd_pcm_hw_params_set_rate_near(m_Stream, params, &actualSampleRate, 0);
    if (res) {
        logger->error("snd_pcm_hw_set_params_set_rate_near() Failed! {}", snd_strerror(res));
        snd_pcm_close(m_Stream);
        return -1;
    }

    // Set requested max buffer and period time
    unsigned int maxBufferTime = (int)(0.015f * 1e6f);
    unsigned int minPeriods = 2;
    unsigned int maxPeriods = 3;

    res = snd_pcm_hw_params_set_buffer_time_max(m_Stream, params, &maxBufferTime, 0);
    if (res) {
        logger->error("snd_pcm_hw_params_set_buffer_time_max() Failed! {}", snd_strerror(res));
        snd_pcm_close(m_Stream);
        return -1;
    }

    res = snd_pcm_hw_params_set_periods_minmax(m_Stream, params, &minPeriods, 0, &maxPeriods, 0);
    if (res) {
        logger->error("snd_pcm_hw_params_set_periods_minmax() Failed! {}", snd_strerror(res));
        snd_pcm_close(m_Stream);
        return -1;
    }

    // Send the parameters
    res = snd_pcm_hw_params(m_Stream, params);
    if (res) {
        logger->error("snd_pcm_hw__paramsr() Failed! {}", snd_strerror(res));
        snd_pcm_close(m_Stream);
        return -1;
    }


    // Get channel count
    unsigned int actualChannels;
    snd_pcm_hw_params_get_channels(params, &actualChannels);

    // Get audio period size
    snd_pcm_uframes_t actualFramesPerBuffer;
    snd_pcm_hw_params_get_period_size(params, &actualFramesPerBuffer, 0);
    unsigned int actualPeriods;
    snd_pcm_hw_params_get_periods(params, &actualPeriods, 0);

    // Check the parameters
    logger->debug("Device       : {}", snd_pcm_name(m_Stream));
    logger->debug("Sample rate  : {}", actualSampleRate);
    logger->debug("Channels     : {}", actualChannels);
    logger->debug("Buffers      : {}", actualPeriods);
    logger->debug("Frames/buffer: {}", actualFramesPerBuffer);

    if (a_Channels != (size_t)actualChannels) {
        logger->error("Requested {} playback channels, got {}", a_Channels, actualChannels);
        snd_pcm_close(m_Stream);
        return -1;
    }
    if (a_FramesPerBuffer > (size_t)actualFramesPerBuffer) {
        logger->error("Requested {} frames per buffer, got {}", a_FramesPerBuffer, actualFramesPerBuffer);
        snd_pcm_close(m_Stream);
        return -1;
    }

    // Create the buffer
    size_t size = actualChannels * actualFramesPerBuffer;
    m_CurrBuffer.reset(new int16_t[size]);
    m_NextBuffer.reset(new int16_t[size]);

    m_BufferValid = false;

    // Set parameters
    m_Format            = SND_PCM_FORMAT_S16_LE;
    m_SampleRate        = (size_t)actualSampleRate;
    m_Channels          = (size_t)actualChannels;
    m_FramesPerBuffer   = (size_t)actualFramesPerBuffer;

    return 0;
}

void AlsaSink::close ()
{
    // Not open
    if (m_Stream == nullptr) {
        return;
    }

    // Stop
    stop();

    // Close the device
    snd_pcm_close(m_Stream);
    m_Stream = nullptr;

    // Cleanup
    if (m_Status != nullptr) {
        snd_pcm_status_free(m_Status);
        m_Status = nullptr;
    }
}

bool AlsaSink::start ()
{
    // Not open
    if (m_Stream == nullptr) {
        return false;
    }

    int  res;
    auto logger = getLogger("alsa");

    // Prepare
    if (snd_pcm_state(m_Stream) == SND_PCM_STATE_SETUP) {
        res = snd_pcm_prepare(m_Stream);
        if (res < 0) {
            logger->error("snd_pcm_prepare() Failed! {}", snd_strerror(res));
            return false;
        }
    }

    // Check
    if (!checkState(SND_PCM_STATE_PREPARED)) {
        return false;
    }

    // Start the worker
    return Worker::start();
}

void AlsaSink::stop ()
{
    // Not open
    if (m_Stream == nullptr) {
        return;
    }

    // Stop the worker
    Worker::stop();

    int  res;
    auto logger = getLogger("alsa");

    // Stop the stream
    if (snd_pcm_state(m_Stream) == SND_PCM_STATE_RUNNING) {
        res = snd_pcm_drop(m_Stream);
        if (res < 0) {
            logger->warn("snd_pcm_drop() Failed! {}", snd_strerror(res));
        }
    }
}

// ============================================================================

bool AlsaSink::checkState (snd_pcm_state_t a_State) {

    // Check
    auto state = snd_pcm_state(m_Stream);
    if (state == a_State) {
        return true;
    }

    // Log the error
    auto logger = getLogger("alsa");
    logger->error("Incorrect PCM state {}, should be {}",
        snd_pcm_state_name(state),
        snd_pcm_state_name(a_State)
    );

    return false;
}

// ============================================================================

bool AlsaSink::isReady (int64_t* a_Time) {
    std::lock_guard<std::mutex> lock(m_BufferLock);

    // Return current audio buffer time
    if (a_Time) {
        *a_Time = m_BufferTime;
    }

    // Poll the stream status
    // FIXME: This has to be called here to make audio run smoothly. Not sure
    // why.
    int res = snd_pcm_status(m_Stream, m_Status);
    if (res) {
        return false;
    }

    return !m_BufferValid;
}

int AlsaSink::loop () {

    // Write the current buffer to the device in blocking mode
    int res = snd_pcm_writei(m_Stream, m_CurrBuffer.get(), m_FramesPerBuffer);
    if (res < 0) {

        // Log the error
        auto logger = getLogger("alsa");
        logger->error("snd_pcm_writei() Failed! {}", snd_strerror(res));

        // Restart the stream
        if (res == -EPIPE) {
            snd_pcm_prepare(m_Stream);
            checkState(SND_PCM_STATE_PREPARED);
        }
    }

    // Take timestamp
    int64_t now = Utils::makeTimestamp();        

    // Copy data, update timestamp
    size_t  size = sizeof(int16_t) * m_Channels * m_FramesPerBuffer;
    int16_t *src = m_NextBuffer.get();
    int16_t *dst = m_CurrBuffer.get();

    std::lock_guard<std::mutex> lock(m_BufferLock);

    // The buffer is valid
    if (m_BufferValid) {
        memcpy(dst, src, size);

        m_BufferValid = false;
        m_BufferTime  = now;
    }

    // The buffer is not valid, send all zeros
    else {
        memset(dst, 0, size);
    }

    return 0;
}

void AlsaSink::writeBuffer (const float* a_Data) {
    std::lock_guard<std::mutex> lock(m_BufferLock);

    // Buffer already valid, do not overwrite
    if (m_BufferValid) {
        return;
    }

    // Convert the data to signed 16-bit
    const float* src = a_Data;
    int16_t* dst = m_NextBuffer.get();

    size_t size = m_Channels * m_FramesPerBuffer;
    for (size_t i=0; i<size; ++i) {
        float f = *src++;

        if (f >  1.0f) f =  1.0f;
        if (f < -1.0f) f = -1.0f;

        *dst++ = (int16_t)(f * 32767.0f);
    }

    // Set the validity flag
    m_BufferValid = true;
}

// ============================================================================

}; // Audio

