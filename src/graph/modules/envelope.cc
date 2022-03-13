#include "../graph.hh"
#include "../exception.hh"
#include "../parameter.hh"

#include "envelope.hh"

#include <strutils.hh>
#include <stringf.hh>
#include <utils/utils.hh>
#include <utils/exception.hh>

#include <regex>
#include <cmath>

namespace Graph {
namespace Modules {

// ============================================================================

Envelope::Envelope (const std::string& a_Type,
                    const std::string& a_Name,
                    const Module::Attributes& a_Attributes) :
    Module (a_Type, a_Name, a_Attributes)
{
    // Input ports
    m_Gate   = addPort(new Port(this, "gate", Port::Direction::INPUT, 0.0f));
    // Output ports
    m_Output = addPort(new Port(this, "out",  Port::Direction::OUTPUT));

    // Parse attributes to get points
    std::regex expr("point([0-9]+)");
    for (auto it : a_Attributes) {
        auto& name  = it.first;
        auto& value = it.second;

        // Check if it is a point definition
        std::smatch match;
        std::regex_match(name, match, expr);

        if (match.empty()) {
            continue;
        }

        // Decode fields
        const auto fields = strutils::split(value, ",");
        if (fields.size() == 2) {
            m_Points.push_back(Point(
                Utils::stof(fields[0]),
                Utils::stof(fields[1])
            ));
        }
        else if (fields.size() == 3) {
            m_Points.push_back(Point(
                Utils::stof(fields[0]),
                Utils::stof(fields[1]),
                std::stoi(fields[2])
            ));
        }
        else {
            THROW(ModuleError,
                "Incorrect envelope point specification: '%s'", value.c_str());
        }
    }

    // If there are any points
    if (!m_Points.empty()) {

        // Sort points by time
        std::sort(m_Points.begin(), m_Points.end(),
            [](Point const& a, Point const& b) {
                return a.time < b.time;
            });

        // Check
        sanityCheckPoints();
    }
}

Module* Envelope::create (
    const std::string& a_Type,
    const std::string& a_Name,
    const Module::Attributes& a_Attributes)
{
    (void)a_Type;
    return new Envelope("envelope", a_Name, a_Attributes);
}

// ============================================================================

void Envelope::sanityCheckPoints() {

    // There have to be at least 2 points.
    if (m_Points.size() < 2) {
        throw ModuleError("There has to be at least two envelope points");
    }

    // Time must increase monotonically.
    for (size_t i=0; i<m_Points.size() - 1; ++i) {
        if (m_Points[1].time < m_Points[0].time) {
            throw ModuleError("Envelope time must be monotonically increasing!");
        }
    }

    // Only one sustain point allowed.
    size_t sustainCount = 0;
    for (auto& point : m_Points) {
        if (point.isSustain) {
            sustainCount++;
        }
    }

    if (sustainCount > 1) {
        throw ModuleError("There can be at most one sustain point!");
    }
}

// ============================================================================

void Envelope::start () {

    // There have to be at least 2 points.
    if (m_Points.size() < 2) {
        throw ModuleError("There has to be at least two envelope points");
    }

    // Reset
    stop();

    // Set the starting level
    m_CurrLevel = m_Points[0].level;
}

void Envelope::stop () {

    // Reset
    m_IsActive   = false;
    m_LevelDelta = 0.0;
    m_GateState  = 0.0f;
}

// ============================================================================

void Envelope::scheduleEvents (int32_t a_Sample, bool a_Attack) {
    //auto logger = m_Pipeline->getLogger();

    // Remove all events
    m_Events.clear();

    // Schedule attack + sustain
    if (a_Attack) {
        int32_t prevTime = -1;
        for (size_t i=0; i<m_Points.size(); ++i) {
            auto&   point = m_Points[i];
            int32_t time  = (int32_t)(point.time * m_SampleRate + 0.5f) + a_Sample;

            if (time <= prevTime) {
                time  = prevTime + 1;
            }
            prevTime = time;

            m_Events.push_back(Event(time, point));
            //logger->debug("A: t={}, l={}", time, point.level);

            if (point.isSustain) {
                break;
            }
        }

        // Set the start level
        if (!m_IsActive) {
            m_CurrLevel = m_Events.front().level;
        }
    }

    // Schedule release
    else {
        size_t i;

        // Find the sustain point
        for (i=0; i<m_Points.size(); ++i) {
            auto& point = m_Points[i];
            if (point.isSustain) {
                break;
            }
        }

        // No sustain point found, schedule only the last two
        if (i == m_Points.size()) {
            i -= 2;
        }

        // Echedule events
        int32_t prevTime = -1;
        for (; i<m_Points.size(); ++i) {
            auto&   point = m_Points[i];
            int32_t time  = (int32_t)(point.time * m_SampleRate + 0.5f) + a_Sample;

            if (time <= prevTime) {
                time  = prevTime + 1;
            }
            prevTime = time;

            m_Events.push_back(Event(time, point));
            //logger->debug("R: t={}, l={}", time, point.level);
        }
    }
}

void Envelope::nextEvent () {
    //auto logger = m_Pipeline->getLogger();

    // Get pop the current event
    auto currEvent = m_Events.front();
    m_Events.pop_front();

    // No more events
    if (m_Events.empty()) {
        m_LevelDelta = 0.0;

        //logger->debug("No more events, delta=0.0");
    }
    // Compute the delta between the present and the next event
    else {
        auto& nextEvent = m_Events.front();

        float dt = nextEvent.time  - currEvent.time;
        float dl = nextEvent.level - m_CurrLevel;

        m_LevelDelta = (double)dl / (double)dt;

        //logger->debug("Next event, t={}->{}, l={}->{}, delta={}",
        //    currEvent.time, nextEvent.time,
        //    m_CurrLevel, nextEvent.level,
        //    m_LevelDelta
        //);
    }
}

void Envelope::process () {

    // Get buffer pointers
    const float* ptrGate = m_Gate->process().data();
    float*       ptrOut  = m_Output->getBuffer().data();

    // Shift all events back
    for (auto& ev : m_Events) {
        ev.time -= m_BufferSize;
        if (ev.time < 0) {
            THROW(ProcessingError, "Negative envelope point time %d!",
                ev.time
            );
        }
    }

    // Next event time
    int32_t nextTime = -1;
    if (!m_Events.empty()) {
        nextTime = m_Events.front().time;
    }

    // Process the buffer
    for (size_t i=0; i<m_BufferSize; ++i) {
        float gate = *ptrGate++;

        // Detect trigger
        float trigger;
        if (i == 0) {
            trigger = gate - m_GateState;
        } else {
            trigger = gate - *(ptrGate - 2);
        }

        // Got a trigger
        if (trigger > 0.5f) {
            scheduleEvents(i, true);
            nextEvent();

            m_IsActive = true;
            nextTime   = m_Events.front().time;
        }

        // Got a trigger release while in sustain
        else if (trigger < -0.5f && m_IsActive && nextTime == -1) {
            scheduleEvents(i, false);
            nextEvent();
            nextTime = m_Events.front().time;
        }

        // Got an event
        else if ((int32_t)i == nextTime) {
            auto event = m_Events.front();
            nextEvent();

            // Got a next one
            if (!m_Events.empty()) {
                nextTime = m_Events.front().time;
            }
            // Sustain point, no next event
            else if (event.isSustain) {

                // Gate still active, keep sustain
                if (gate > 0.5f) {
                    nextTime = -1;
                    //logger->debug("Sustain");
                }
                // Gate not active, schedule release
                else {
                    scheduleEvents(i, false);
                    nextEvent();
                    nextTime = m_Events.front().time;
                }
            }
            // End of release
            else {
                nextTime    = -1;
                m_IsActive  = false;
                m_CurrLevel = event.level;
                //logger->debug("End");
            }
        }

        // Generate the envelope
        else {
            m_CurrLevel += m_LevelDelta;
        }

        // Output the envelope
        *ptrOut++ = (float)m_CurrLevel;
    }

    // Store the last gate state
    m_GateState = *(ptrGate - 1);
}

// ============================================================================

}; // Modules
}; // Graph

