#ifndef MIDI_ALSASEQ_SOURCE_HH
#define MIDI_ALSASEQ_SOURCE_HH

#include <utils/worker.hh>
#include "event.hh"

#include <spdlog/spdlog.h>

#include <alsa/asoundlib.h>

#include <vector>
#include <queue>
#include <mutex>

#include <thread>

#include <cstdint>

#include <unistd.h>

namespace MIDI {

// ============================================================================

class AlsaSeqSource : public Worker {
public:

     AlsaSeqSource ();
    ~AlsaSeqSource () override;

    /// Open
    bool open  (const std::string& a_Name);
    /// Close
    void close ();

    /// Start streaming
    bool start () override;

    /// Returns a list of MIDI events. Can limit the retreived count
    const std::vector<Event> getEvents (size_t a_MaxCount = 0);
    /// Returns a list of MIDI events that happened before the given timestamp
    const std::vector<Event> getEventsBefore (uint64_t a_Time);

    /// Returns the event queue
    std::queue<Event>& getQueue ();

    /// Returns the name
    const std::string& getName () const {return m_SeqName;}
    /// Returns the sequencer client id
    int getId () const {return m_SeqId;}
    /// Returns the sequencer client port id
    int getPort () const {return m_SeqPort;}

protected:
   
    /// Thread initialization
    int  init  () override; 
    /// The polling loop
    int  loop  () override;

    /// Name
    std::string m_SeqName;
    /// Logger
    std::shared_ptr<spdlog::logger> m_Logger;

    /// Sequencer context
    snd_seq_t* m_Seq  = nullptr;
    /// Client ID
    int m_SeqId    = 0;
    /// Client port ID
    int m_SeqPort  = 0;
    /// Sequencer queue id
    int m_SeqQueue = 0;

    /// Event queue
    std::queue<Event> m_Queue;
    /// Event queue lock
    std::mutex        m_QueueLock;

    /// Poll descriptors
    struct pollfd*  m_PollFd    = nullptr;
    size_t          m_PollCount = 0;
};

// ============================================================================

}; // MIDI

#endif // MIDI_ALSASEQ_SOURCE_HH

