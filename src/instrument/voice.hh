#ifndef INSTRUMENT_VOICE_HH
#define INSTRUMENT_VOICE_HH

#include <audio/buffer.hh>
#include <midi/event.hh>

#include <graph/module.hh>
#include <graph/port.hh>
#include <graph/iface/midi_listener.hh>

#include <vector>
#include <memory>
#include <limits>

namespace Instrument {

// ============================================================================

class Voice
{
public:

    /// Constructor
    Voice (const Graph::Module* a_Module, float a_MinLevel = -96.0f);

    /// Returns true when stereo
    bool isStereo   () const;

    /// Returns true when active
    bool isActive   () const;
    /// Activates the voice
    void activate   ();
    /// Deactivates the voice
    void deactivate ();

    /// Returns activity time in ms
    int64_t getActiveTime () const;
    /// Returns silence time in ms
    int64_t getSilentTime () const;

    /// Pushes a single MIDI event on the queue
    void pushEvent (const MIDI::Event& a_Event);

    /// Processes audio
    void process ();
    /// Returns the audio buffer
    const Audio::Buffer<float> getBuffer () const;
    /// Returns the peak audio level in dB
    float getPeakLevel () const;

    /// Returns the top-level module
    Graph::Module* getModule ();

protected:

    /// The top-level module
    std::shared_ptr<Graph::Module> m_Module;
    /// Output audio port of the top-level module
    Graph::Port* m_AudioPort[2];
    /// Peak audio level [dB]
    float m_PeakLevel = -std::numeric_limits<float>::infinity();

    /// MIDI events
    std::vector<MIDI::Event> m_MidiEvents;
    /// MIDI listeners
    std::vector<Graph::Modules::IMidiListener*> m_MidiListeners;

    /// Active flag
    bool    m_Active  = false;
    /// Playing (making actual sound) flag
    bool    m_Playing = false;
    /// Minimal peak signal level
    float   m_MinLevel;
    /// Active time
    int64_t m_ActiveTime = 0;
    /// Silent time
    int64_t m_SilentTime = 0;

    /// Audio buffer
    Audio::Buffer<float> m_Buffer;
};


// ============================================================================

}; // Instrument

#endif // INSTRUMENT_VOICE_HH
