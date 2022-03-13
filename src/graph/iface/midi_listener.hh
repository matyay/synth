#ifndef GRAPH_MODULES_MIDI_LISTENER_HH
#define GRAPH_MODULES_MIDI_LISTENER_HH

#include <utils/base_interface.hh>
#include <midi/event.hh>

#include <vector>
#include <cstdint>

namespace Graph {
namespace Modules {

// ============================================================================

class IMidiListener : public IBaseInterface {
public:

    /// Virtual desctructor
    virtual ~IMidiListener () {};

    /// Interface ID
    static constexpr iid_t ID = INTERFACE_ID("MIDI");

    /// Pushes a single event on the event list
    virtual void pushEvent (const MIDI::Event& a_Event) = 0;
};

// ============================================================================

}; // Modules
}; // Graph

#endif  // GRAPH_MODULES_MIDI_LISTENER_HH
