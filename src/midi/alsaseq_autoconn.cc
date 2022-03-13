#include "alsaseq_autoconn.hh"

#include <utils/utils.hh>
#include <utils/logging.hh>

#include <stringf.hh>

#include <thread>
#include <chrono>
#include <cstdint>

namespace MIDI {

// ============================================================================

AlsaSeqAutoConnector::AlsaSeqAutoConnector (uint8_t a_ClientId, uint8_t a_PortId) :
    Worker   (),
    m_Target ({a_ClientId, a_PortId})
{}

// ============================================================================

bool AlsaSeqAutoConnector::start () {

    auto logger = getLogger("alsaseq");

    // Already started
    if (m_Seq != nullptr) {
        return false;
    }

    // Open the sequencer & set its name
    int res = snd_seq_open(&m_Seq, "default", SND_SEQ_OPEN_INPUT, 0);
    if (res < 0) {
        logger->error("snd_seq_open() Failed! {}", snd_strerror(res));
        return false;
    }

    snd_seq_set_client_name(m_Seq, "alsaconnector");

    // Start the worker
    return Worker::start();
}

void AlsaSeqAutoConnector::stop () {

    // Stop the worker
    Worker::stop();

    // Close the sequencer
    if (m_Seq != nullptr) {
        snd_seq_close(m_Seq);
        m_Seq = nullptr;
    }
}

// ============================================================================

int AlsaSeqAutoConnector::connect (snd_seq_addr_t a_Src, snd_seq_addr_t a_Dst)
{
    int res;

    // Allocate the subscription info
    snd_seq_port_subscribe_t* subs = nullptr;
    snd_seq_port_subscribe_alloca(&subs);

    // Fill it in
    snd_seq_port_subscribe_set_sender(subs, &a_Src);
    snd_seq_port_subscribe_set_dest(subs, &a_Dst);
    snd_seq_port_subscribe_set_exclusive(subs, 0);
    snd_seq_port_subscribe_set_queue(subs, 1);
    snd_seq_port_subscribe_set_time_update(subs, 1);
    snd_seq_port_subscribe_set_time_real(subs, 1);

    // Check if not already subscribed
    res = snd_seq_get_port_subscription(m_Seq, subs);
    if (!res) {
        return 1;
    }

    // Make the connection
    res = snd_seq_subscribe_port(m_Seq, subs);
    return res;
}

int AlsaSeqAutoConnector::loop () {

    // Wait
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (m_Ticks > 0) {
        m_Ticks--;
        return 0;
    }
    m_Ticks = 10;

    int  res;
    auto logger = getLogger("alsaseq");

    SPDLOG_LOGGER_TRACE(logger, "Polling for input MIDI devices...");

    // Allocate client and port info data
    snd_seq_port_info_t*   pinfo = nullptr;
    snd_seq_client_info_t* cinfo = nullptr;

    res = snd_seq_client_info_malloc(&cinfo);
    if (res != 0) {
        logger->error("snd_seq_client_info_malloc() Failed! {}", snd_strerror(res));
        return 0;
    }

    res = snd_seq_port_info_malloc(&pinfo);
    if (res != 0) {
        logger->error("snd_seq_client_info_malloc() Failed! {}", snd_strerror(res));
        return 0;
    }

    // Look for ALSA sequencer clients
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(m_Seq, cinfo) >= 0) {

        uint8_t cid = snd_seq_client_info_get_client(cinfo);
        if (cid == snd_seq_client_id(m_Seq)) {
            continue;
        }

        // Get client info
        const char* cname = snd_seq_client_info_get_name(cinfo);
        //bool isCard = snd_seq_client_info_get_card(cinfo) != -1;

        // Check every port
        snd_seq_port_info_set_client(pinfo, cid);
        snd_seq_port_info_set_port(pinfo, -1);

        while (snd_seq_query_next_port(m_Seq, pinfo) >= 0) {
            uint8_t pid = snd_seq_port_info_get_port(pinfo);

            // Get port info
            const char* pname = snd_seq_port_info_get_name(pinfo);
            uint32_t type = snd_seq_port_info_get_type(pinfo);
            uint32_t caps = snd_seq_port_info_get_capability(pinfo);

            // Check if the port is a hardware MIDI input
            bool canConnect = ((caps & SND_SEQ_PORT_CAP_READ) != 0) &&
                              ((type & SND_SEQ_PORT_TYPE_PORT) != 0) &&
                              ((type & SND_SEQ_PORT_TYPE_HARDWARE) != 0);

            // DEBUG
            SPDLOG_LOGGER_TRACE(logger, stringf("%c%d:%d '%s':'%s'", 
                canConnect ? '*' : ' ',
                cid, pid, cname, pname));

            // Connect
            if (canConnect) {

                res = connect({cid, pid}, m_Target);
                if (res == 0) {
                    logger->info("Connected to {}:{} '{}':'{}'",
                        cid, pid, cname, pname);
                }
                else if (res < 0) {
                    logger->error("Failed attempt to connect to {}:{} '{}':'{}'",
                        cid, pid, cname, pname, snd_strerror(res));
                }
            }
        }
    }

    // Free client and port info
    snd_seq_port_info_free(pinfo);
    snd_seq_client_info_free(cinfo);

    return 0;
}

// ============================================================================
}; // MIDI

