#ifndef AUDIO_RECORDER_HH
#define AUDIO_RECORDER_HH

#include <utils/worker.hh>
#include "buffer.hh"

#include <string>

#include <queue>
#include <mutex>

#include <cstdio>

namespace Audio {

// ============================================================================

class Recorder : public Worker {
public:

    /// Maximum file index
    static constexpr size_t MAX_FILE_INDEX = 9999;

    /// Constructor
    Recorder (const std::string& a_Path = ".");
    /// Destructor
    virtual ~Recorder () override;

    /// Starts recording to a new file
    bool start () override;
    /// Stops recording
    void stop  () override;

    /// Returns true when recording
    bool isRecording () const;
    /// Returns current file name
    std::string getFileName () const;

    /// Puts buffer into the queue
    void push (const Buffer<float>& a_Buffer);

protected:

    /// Worker loop
    int loop () override;

    /// Pops and writes buffer
    void popAndWrite ();
    /// Returns next free file index
    static size_t getNextFileIndex (const std::string& a_Format, size_t a_Begin = 0);

    /// Input audio buffer queue
    std::queue<Buffer<float>> m_InputQueue;
    /// Write audio buffer queue
    std::queue<Buffer<float>> m_WriteQueue;
    /// Synchronization mutex
    std::mutex m_QueueLock;

    /// Temporary audio data buffer
    std::vector<float> m_AudioData;

    /// Record path
    const std::string m_Path;
    /// Current file name
    std::string m_FileName;
    /// Current file index
    size_t m_FileIndex = 0;
    /// Open file object
    FILE*  m_File = nullptr;
};

// ============================================================================

}; // Audio

#endif // RECORDER_HH
