#include "alsaseq_source.hh"

#include <utils/utils.hh>
#include <utils/logging.hh>

#include <stringf.hh>

namespace MIDI {

// ============================================================================

AlsaSeqSource::AlsaSeqSource () :
    Worker ()
{
}

AlsaSeqSource::~AlsaSeqSource () {
    close();
}

// ============================================================================

bool AlsaSeqSource::open (const std::string& a_Name) {
    int res;

    // Create the logger
    m_Logger = getLogger(stringf("alsaseq [%s]", a_Name.c_str()));

    // Open the sequencer & set its name
    res = snd_seq_open(&m_Seq, "hw", SND_SEQ_OPEN_DUPLEX, 0);
    if (res < 0) {
        m_Logger->error("snd_seq_open() Failed! %s", snd_strerror(res));
        return false;
    }

    snd_seq_set_client_name(m_Seq, a_Name.c_str());
    m_SeqId = snd_seq_client_id(m_Seq);

    // Allocate an event queue
    res = snd_seq_alloc_queue(m_Seq);
    if (res < 0) {
        m_Logger->error("snd_seq_alloc_queue() Failed! %s", snd_strerror(res));
        close();
        return false;
    }
    m_SeqQueue = res;

    // Create port
    snd_seq_port_info_t* portInfo = nullptr;
    snd_seq_port_info_alloca(&portInfo);

    snd_seq_port_info_set_name(portInfo, a_Name.c_str());
    snd_seq_port_info_set_type(portInfo, SND_SEQ_PORT_TYPE_APPLICATION);
    snd_seq_port_info_set_capability(portInfo, SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE);
    snd_seq_port_info_set_timestamping(portInfo, 1);
    snd_seq_port_info_set_timestamp_real(portInfo, 1);
    snd_seq_port_info_set_timestamp_queue(portInfo, m_SeqQueue);
    
    res = snd_seq_create_port(m_Seq, portInfo);
    if (res < 0) {
        m_Logger->error("snd_seq_create_port() Failed! %s", snd_strerror(res));
        close();
        return false;
    }

    // Get the port id & store the name
    m_SeqName = a_Name;
    m_SeqPort = snd_seq_port_info_get_port(portInfo);

    // Set the sequencer to non-blocking mode
    snd_seq_nonblock(m_Seq, 1);

    m_Logger->info("ALSA sequencer client '{}:{}' ready", m_SeqName, m_SeqPort);
    return true;
}

void AlsaSeqSource::close () {

    // Stop worker
    stop();

    // Close
    if (m_Seq != nullptr) {
        snd_seq_close(m_Seq);
        m_Seq = nullptr;
    }

    // Free poll descriptors
    if (m_PollFd) {
        free(m_PollFd);
        m_PollFd = nullptr;
    }
}

// ============================================================================

bool AlsaSeqSource::start () {

    if (m_Seq == nullptr) {
        return false;
    }

    // Start the ALSA queue
    int res = snd_seq_start_queue(m_Seq, m_SeqQueue, nullptr);
    if (res < 0) {
        m_Logger->error("snd_seq_start_queue() Failed! %s", snd_strerror(res));
        return false;
    }

    // Clear the event queue
    m_Queue = std::queue<Event>();

    return Worker::start();
}


int AlsaSeqSource::init () {

    // Allocate poll descriptors
    m_PollCount = snd_seq_poll_descriptors_count(m_Seq, POLLIN);
    m_PollFd = (struct pollfd*)malloc(m_PollCount * sizeof(struct pollfd));
    snd_seq_poll_descriptors(m_Seq, m_PollFd, m_PollCount, POLLIN);

    return 0;
}

int AlsaSeqSource::loop () {

    // Poll for events
    if (::poll(m_PollFd, m_PollCount, 1) > 0) {

        // Get timestamp
        auto timestamp = Utils::makeTimestamp();

        // Lock        
        std::lock_guard<std::mutex> lock(m_QueueLock);

        // Process events
        snd_seq_event_t* seqEvent;
        do {

            // Check if there is an event
            int res = snd_seq_event_input(m_Seq, &seqEvent);
            if (res <= 0) {
                if (res == -ENOSPC) {
                    m_Logger->warn("snd_seq_event_input(): ENOSPC");
                }
                break;
            }

            // Process the event
            Event event;
            event.time = timestamp;

            switch (seqEvent->type)
            {
            case SND_SEQ_EVENT_RESET: {
                event.type = Event::Type::RESET;

                m_Queue.push(event);
                break;
                }

            case SND_SEQ_EVENT_NOTEON: {
                event.type = Event::Type::NOTE_ON;
                event.data.note.channel     = seqEvent->data.note.channel;
                event.data.note.note        = seqEvent->data.note.note;
                event.data.note.velocity[0] = seqEvent->data.note.velocity;
                event.data.note.velocity[1] = seqEvent->data.note.velocity;
                event.data.note.duration    = 0;

                // For note on with velocity = 0 make it a note off.
                if (event.data.note.velocity[0] == 0) {
                    event.type = Event::Type::NOTE_OFF;
                }

                m_Queue.push(event);
                break;
                }

            case SND_SEQ_EVENT_NOTEOFF: {
                event.type = Event::Type::NOTE_OFF;
                event.data.note.channel     = seqEvent->data.note.channel;
                event.data.note.note        = seqEvent->data.note.note;
                event.data.note.velocity[0] = seqEvent->data.note.velocity;
                event.data.note.velocity[1] = seqEvent->data.note.velocity;
                event.data.note.duration    = 0;

                m_Queue.push(event);
                break;
                }

            case SND_SEQ_EVENT_CONTROLLER: {
                event.type = Event::Type::CONTROLLER;
                event.data.ctrl.channel     = seqEvent->data.control.channel;
                event.data.ctrl.param       = seqEvent->data.control.param;
                event.data.ctrl.value       = seqEvent->data.control.value;

                m_Queue.push(event);
                break;
                }
            }

            // Free the event
            snd_seq_free_event(seqEvent);

        } while (snd_seq_event_input_pending(m_Seq, 0) > 0);

    }

    return 0;
}

// ============================================================================

const std::vector<Event> AlsaSeqSource::getEvents (size_t a_MaxCount) {
    std::lock_guard<std::mutex> lock(m_QueueLock);

    // Get number of events to fetch
    size_t count = m_Queue.size();
    if (a_MaxCount > 0 && count > a_MaxCount) {
        count = a_MaxCount;
    }

    // Pop events
    std::vector<Event> events(count);
    for (size_t i=0; i<count; ++i) {
        events[i] = m_Queue.front();
        m_Queue.pop();
    }

    return events;
}

const std::vector<Event> AlsaSeqSource::getEventsBefore (uint64_t a_Time) {
    std::lock_guard<std::mutex> lock(m_QueueLock);

    // Queue empty
    if (m_Queue.empty()) {
        return std::vector<Event>();
    }

    // Reserver a vector
    std::vector<Event> events;
    events.reserve(m_Queue.size());

    // Pop events
    while (!m_Queue.empty() && (uint64_t)m_Queue.front().time < a_Time) {
        events.push_back(m_Queue.front());
        m_Queue.pop();
    }

    return events;
}

// ============================================================================
}; // MIDI
