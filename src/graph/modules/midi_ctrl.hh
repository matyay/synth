#ifndef GRAPH_MODULES_MIDI_CONTROLLER_HH
#define GRAPH_MODULES_MIDI_CONTROLLER_HH

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

class MidiController : public Module, public IMidiListener {
public:

    /// Constructor
    MidiController (
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

    /// Updates state with an event.
    void update  (const MIDI::Event& a_Event);

    /// Called on audio processing start
    void start   () override;
    /// Called on audio processing stop
    void stop    () override;

    /// Processes a single audio buffer
    void process () override;

    /// Pushes a single event on the event list
    void pushEvent (const MIDI::Event& a_Event) override;

protected:

    /// Activity flag
    bool m_Active = false;

    /// Controller id
    uint32_t m_Controller;

    /// Min value
    float m_Min;
    /// Max value
    float m_Max;

    /// Control output
    Port* m_Output;
    /// Output state
    float m_State;

    /// Event list
    std::vector<MIDI::Event> m_Events;
};

// ============================================================================

}; // Modules
}; // Graph

#endif // GRAPH_MODULES_MIDI_CONTROLLER_HH

