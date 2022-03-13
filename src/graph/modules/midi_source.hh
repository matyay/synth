#ifndef GRAPH_MODULES_MIDI_SOURCE_HH
#define GRAPH_MODULES_MIDI_SOURCE_HH

#include "../module.hh"
#include "../iface/midi_listener.hh"

#include <utils/base_interface.hh>
#include <audio/buffer.hh>

#include <vector>
#include <string>

#include <cstdint>

namespace Graph {
namespace Modules {

// ============================================================================

class MidiSource : public Module, public IMidiListener {
public:

    /// Constructor
    MidiSource (
        const std::string& a_Name,
        const Module::Attributes& a_Attributes = Module::Attributes()
    );

    /// Creates an instance
    static Module* create (
        const std::string& a_Type,
        const std::string& a_Name,
        const Module::Attributes& a_Attributes = Module::Attributes()
    );

    /// Query for a given interface id.
    IBaseInterface* queryInterface (iid_t a_Id) override;

    /// Called on processing start
    void start () override;

    /// Processes a single audio buffer
    void process () override;

    /// Pushes a single event on the event list
    void pushEvent (const MIDI::Event& a_Event) override;

protected:

    /// Resets state
    void reset ();

    /// Minimum MIDI note index
    size_t m_MinNote = 0;
    /// Maximum MIDI note index
    size_t m_MaxNote = 127;

    /// "Control voltage" output
    Port* m_Note;
    /// Note velocity output
    Port* m_Velocity;
    /// Gate output
    Port* m_Gate;

    /// Output state
    struct {
        float cv;
        float velocity;
        float gate;
    } m_State;

    /// Event list
    std::vector<MIDI::Event> m_Events;
};

// ============================================================================

}; // Modules
}; // Graph

#endif // GRAPH_MODULES_MIDI_SOURCE_HH

