#ifndef MIDI_ALSASEQ_AUTOCONN_HH
#define MIDI_ALSASEQ_AUTOCONN_HH

#include <utils/worker.hh>

#include <alsa/asoundlib.h>

#include <thread>
#include <cstdint>

namespace MIDI {

// ============================================================================

class AlsaSeqAutoConnector : public Worker {
public:

    /// Constructor
    AlsaSeqAutoConnector (uint8_t a_ClientId, uint8_t a_PortId = 0);

    /// Start connection
    bool start () override;
    /// Stop connecting
    void stop () override;

protected:

    /// Connects two ALSA sequencer ports
    int connect (snd_seq_addr_t a_Src, snd_seq_addr_t a_Dst);

    /// The polling loop
    int loop () override;

    /// Sequencer context
    snd_seq_t* m_Seq = nullptr;
    /// Target client and port address
    const snd_seq_addr_t m_Target;

    int m_Ticks = 0;
};

// ============================================================================

}; // MIDI

#endif // MIDI_ALSASEQ_AUTOCONN_HH

