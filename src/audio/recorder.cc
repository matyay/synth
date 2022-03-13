#include "recorder.hh"

#include <utils/logging.hh>

#include <stringf.hh>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>

#include <string.h>
#include <errno.h>

namespace Audio {

// ============================================================================

Recorder::Recorder (const std::string& a_Path) :
    m_Path (a_Path)
{}

Recorder::~Recorder () {
    stop();
}

// ============================================================================

bool Recorder::isRecording () const {
    return isAlive();
}

std::string Recorder::getFileName () const {
    return m_FileName;
}

// ============================================================================

bool Recorder::start () {

    // Already started
    if (isRecording()) {
        return false;
    }

    auto logger = getLogger("recorder");

    // Get next free file index
    const std::string format = m_Path + "/record_%04d.raw";
    m_FileIndex = getNextFileIndex(format, m_FileIndex);

    // Set file name
    m_FileName = stringf(format, m_FileIndex);
    logger->info("Recording to '{}'...", m_FileName);

    // Open the file
    m_File = fopen(m_FileName.c_str(), "wb");
    if (m_File == nullptr) {
        logger->error("Error opening file! {}", strerror(errno));
        return false;
    }

    // Clear the queue
    auto empty1 = std::queue<Buffer<float>>();
    std::swap(m_InputQueue, empty1);

    auto empty2 = std::queue<Buffer<float>>();
    std::swap(m_WriteQueue, empty2);

    // Start the worker
    return Worker::start();
}

void Recorder::stop () {

    // Not recording
    if (!isRecording()) {
        return;
    }

    auto logger = getLogger("recorder");
    logger->info("Stopping recording...");

    // Stop the worker
    Worker::stop();

    // Flush the input queue
    while (!m_InputQueue.empty()) {
        m_WriteQueue.push(m_InputQueue.front());
        m_InputQueue.pop();
    }

    // Flush the write queue
    while (!m_WriteQueue.empty()) {
        popAndWrite();
    }

    // Close the file
    if (m_File != nullptr) {
        fclose(m_File);
        m_File = nullptr;
    }
}

size_t Recorder::getNextFileIndex (const std::string& a_Format, size_t a_Begin) {

    auto fileExists = [](const std::string& fileName) {
        struct stat buffer;   
        return (stat(fileName.c_str(), &buffer) == 0);
    };

    // Limit
    if (a_Begin > MAX_FILE_INDEX) {
        a_Begin = MAX_FILE_INDEX;
    }

    // Find a first free file name
    for (size_t i=a_Begin; i<MAX_FILE_INDEX; ++i) {
        std::string fileName = stringf(a_Format, i);

        // File does not exist
        if (!fileExists(fileName)) {
            return i;
        }
    }

    // Maximum number of files reached
    return MAX_FILE_INDEX;
}

// ============================================================================

void Recorder::push (const Buffer<float>& a_Buffer) {
    std::lock_guard<std::mutex> lock(m_QueueLock);
    m_InputQueue.push(a_Buffer.copy());
}

void Recorder::popAndWrite () {

    // Empty queue
    if (m_WriteQueue.empty()) {
        return;
    }

    auto logger = getLogger("recorder");

    // Get the buffer and its size
    const auto& buffer = m_WriteQueue.front();
    size_t count = buffer.getSize() * buffer.getChannels();

    // Reorder samples
    m_AudioData.resize(count);

    if (buffer.getChannels() == 2) {
        const float* ptrL = buffer.data(0);
        const float* ptrR = buffer.data(1);
        float*       ptr  = m_AudioData.data();
        
        for (size_t i=0; i<count; i=i+2) {
            *ptr++ = *ptrL++;
            *ptr++ = *ptrR++;
        }
    }
    else if (buffer.getChannels() == 1) {
        memcpy(m_AudioData.data(), buffer.data(), count * sizeof(float));
    }

    // Write it
    size_t res = fwrite(m_AudioData.data(), sizeof(float), m_AudioData.size(), m_File);
    if (res == 0) {
        int err = ferror(m_File);
        logger->error("Write error! {}", strerror(err));
    }

    // Pop
    m_WriteQueue.pop();
}

int Recorder::loop () {

    // Transfer buffers from the input queue to the write queue
    {
        std::lock_guard<std::mutex> lock(m_QueueLock);
        while (!m_InputQueue.empty()) {
            m_WriteQueue.push(m_InputQueue.front());
            m_InputQueue.pop();
        }
    }
    
    // Pop and write data
    while (!m_WriteQueue.empty()) {
        popAndWrite();
    }

    // Sleep
    std::this_thread::sleep_for (std::chrono::milliseconds(10));

    return 0;
}

// ============================================================================

}; // Audio
