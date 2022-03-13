#include "../graph.hh"
#include "../exception.hh"
#include "midi_source.hh"

#include <utils/utils.hh>
#include <stringf.hh>

#include <algorithm>
#include <cassert>

namespace Graph {
namespace Modules {

// ============================================================================

MidiSource::MidiSource (const std::string& a_Name,
                        const Module::Attributes& a_Attributes) :
    Module ("midiSource", a_Name, a_Attributes)
{
    // Output ports
    m_Note      = addPort(new Port(this, "cv",       Port::Direction::OUTPUT));
    m_Velocity  = addPort(new Port(this, "velocity", Port::Direction::OUTPUT));
    m_Gate      = addPort(new Port(this, "gate",     Port::Direction::OUTPUT));

    // Decode attributes
    if (a_Attributes.has("minNote")) {
        auto str  = a_Attributes.get("minNote");
        int  note = Utils::noteToIndex(str);
        if (note < 0) {
            THROW(ModuleError, "Invalid node specification '%s'",
                str.c_str()
            );
        }

        m_MinNote = (size_t)note;
    }

    if (a_Attributes.has("maxNote")) {
        auto str  = a_Attributes.get("maxNote");
        int  note = Utils::noteToIndex(str);
        if (note < 0) {
            THROW(ModuleError, "Invalid node specification '%s'",
                str.c_str()
            );
        }

        m_MaxNote = (size_t)note;
    }

    // Reset state
    reset();
}

Module* MidiSource::create (
    const std::string& a_Type,
    const std::string& a_Name,
    const Module::Attributes& a_Attributes)
{
    (void)a_Type;
    return new MidiSource(a_Name, a_Attributes);
}

// ============================================================================

IBaseInterface* MidiSource::queryInterface (iid_t a_Id) {

    // We have IMidiListener
    if (a_Id == IMidiListener::ID) {
        return static_cast<IMidiListener*>(this);
    }

    return Module::queryInterface(a_Id);
}

// ============================================================================

void MidiSource::reset() {

    // Clear the event queue
    m_Events.clear();

    // Update state
    m_State.cv       = 0.0f;
    m_State.velocity = 0.0f;
    m_State.gate     = 0.0f;
}

void MidiSource::pushEvent (const MIDI::Event& a_Event) {

    // Filter the event
    if (a_Event.type == MIDI::Event::Type::NOTE_ON ||
        a_Event.type == MIDI::Event::Type::NOTE_OFF)
    {
        if (a_Event.data.note.note < m_MinNote) {
            return;
        }
        if (a_Event.data.note.note > m_MaxNote) {
            return;
        }
    }

    m_Events.push_back(a_Event);
}

// ============================================================================

void MidiSource::start () {
    reset();
}

void MidiSource::process () {

    // Sort events by their times
    std::sort(m_Events.begin(), m_Events.end(),
        [](MIDI::Event const& a, MIDI::Event const& b) {
            return a.time < b.time;
        });

    // Get data pointers
    float* ptrCv       = m_Note->getBuffer().data();
    float* ptrVelocity = m_Velocity->getBuffer().data();
    float* ptrGate     = m_Gate->getBuffer().data();

    size_t pos   = 0;
    auto   evItr = m_Events.begin();

    while (pos < m_BufferSize) {

        // Get next event, determine current segment length
        size_t length;
        if (evItr != m_Events.end()) {
            MIDI::Event& nextEvent = *evItr;
            size_t sample = nextEvent.time;

            assert(sample >= pos);
            assert(sample < m_BufferSize);

            length = sample - pos;
        }

        // No next event, continue to the end of the buffer
        else {
            length = m_BufferSize - pos;
        }

        // Fill the buffer segment with the current state.
        for (size_t i=0; i<length; ++i) {
            *ptrCv++       = m_State.cv;
            *ptrVelocity++ = m_State.velocity;
            *ptrGate++     = m_State.gate;
        }
        pos += length;

        // Got a next event, update the state
        if (evItr != m_Events.end()) {
            MIDI::Event& event = *evItr++;

            if (event.type == MIDI::Event::Type::NOTE_ON) {
                m_State.cv       = Utils::noteToCv(event.data.note.note);
                m_State.velocity = (float)event.data.note.velocity[0] / 127.0f;
                m_State.gate     = 1.0f;
            }

            if (event.type == MIDI::Event::Type::NOTE_OFF) {
                m_State.gate     = 0.0f;
            }
        }
    }

    // Clear the event list
    m_Events.clear();

    // Clear dirty flags on all output ports
    for (auto itr : getPorts()) {
        auto port = itr.second;
        if (port->getDirection() == Port::Direction::OUTPUT) {
            port->clearDirty();
        }
    }
}


// ============================================================================

}; // Modules
}; // Graph

