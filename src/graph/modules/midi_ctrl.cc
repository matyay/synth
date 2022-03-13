#include "midi_ctrl.hh"
#include <utils/utils.hh>

#include <algorithm>
#include <cassert>

namespace Graph {
namespace Modules {

// ============================================================================

MidiController::MidiController (const std::string& a_Name,
                                const Module::Attributes& a_Attributes) :
    Module ("midiController", a_Name, a_Attributes)
{
    // Output port
    m_Output = addPort(new Port(this, "out", Port::Direction::OUTPUT));

    // Attributes
    m_Controller = std::stoi(a_Attributes.get("controller", "0"));
    m_State      = Utils::stof(a_Attributes.get("default",    "0.0f"));
    m_Min        = Utils::stof(a_Attributes.get("min",        "0.0f"));
    m_Max        = Utils::stof(a_Attributes.get("max",        "1.0f"));
}

Module* MidiController::create (
    const std::string& a_Type,
    const std::string& a_Name,
    const Module::Attributes& a_Attributes)
{
    (void)a_Type;
    return new MidiController(a_Name, a_Attributes);
}

// ============================================================================

IBaseInterface* MidiController::queryInterface (iid_t a_Id) {

    // We have IMidiListener
    if (a_Id == IMidiListener::ID) {
        return static_cast<IMidiListener*>(this);
    }

    return Module::queryInterface(a_Id);
}

// ============================================================================

void MidiController::pushEvent (const MIDI::Event& a_Event) {

    // Not active, update immediately
    if (!m_Active) {
        update(a_Event);
    }
    // Queue the event
    else {
        m_Events.push_back(a_Event);
    }
}

// ============================================================================

void MidiController::update (const MIDI::Event& a_Event) {

    // A controller event
    if (a_Event.type == MIDI::Event::Type::CONTROLLER) {

        // We are listening for this one
        if (a_Event.data.ctrl.param == m_Controller) {
            float v = (float)a_Event.data.ctrl.value / 127.0f;
            m_State = m_Min + v * (m_Max - m_Min);
        }
    }
}

// ============================================================================

void MidiController::start () {
    m_Events.clear();
    m_Active = true;
}

void MidiController::stop () {
    m_Events.clear();
    m_Active = false;
}

void MidiController::process () {

    // Sort events by their times
    std::sort(m_Events.begin(), m_Events.end(),
        [](MIDI::Event const& a, MIDI::Event const& b) {
            return a.time < b.time;
        });
    
    // Get data pointers
    float* ptr   = m_Output->getBuffer().data();

    size_t pos   = 0;
    auto   evItr = m_Events.begin();

    while (pos < m_BufferSize) {

        // Get next event, determine current segment length
        size_t length;
        if (evItr != m_Events.end()) {
            MIDI::Event& nextEvent = *evItr;
            size_t sample = nextEvent.time;

            assert(sample >= pos);
            assert(sample <  m_BufferSize);

            length = sample - pos;
        }

        // No next event, continue to the end of the buffer
        else {
            length = m_BufferSize - pos;
        }

        // Fill the buffer segment with the current state.
        for (size_t i=0; i<length; ++i) {
            *ptr++ = m_State;
        }
        pos += length;

        // Got a next event, update the state
        if (evItr != m_Events.end()) {
            update(*evItr++);
        }
    }

    // Clear the event list
    m_Events.clear();
}


// ============================================================================

}; // Modules
}; // Graph

