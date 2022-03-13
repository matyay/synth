#include "gate_detector.hh"

#include <graph/exception.hh>

#include <stringf.hh>

namespace Graph {
namespace Processing {

// ============================================================================

GateDetector::GateDetector (Edge a_Edge, float a_Threshold) :
    m_Edge      (a_Edge),
    m_Threshold (a_Threshold)
{
    // Empty
}

// ============================================================================

void GateDetector::setThreshold (float a_Threshold) {
    m_Threshold = a_Threshold;
}

void GateDetector::reset (float a_Level) {
    m_Events.clear();
    m_State = a_Level;
}

const std::vector<GateDetector::Event>& GateDetector::getEvents () const {
    return m_Events;
}

// ============================================================================

const std::vector<GateDetector::Event>& GateDetector::process
    (const float* a_Ptr, size_t a_Length)
{
    static auto detectRising = [](const float& prev, const float& curr, const float& th) {
        return (prev <= th) && (curr >  th);
    };

    static auto detectFalling = [](const float& prev, const float& curr, const float& th) {
        return (prev >= th) && (curr <  th);
    };

    static auto detectBoth = [](const float& prev, const float& curr, const float& th) {
        return ((prev <= th) && (curr >  th)) ||
               ((prev >= th) && (curr <  th));
    };

    // Set function pointer
    bool (*func)(const float&, const float&, const float&);
    switch (m_Edge)
    {
    case Edge::RISING:  func = detectRising;  break;
    case Edge::FALLING: func = detectFalling; break;
    case Edge::BOTH:    func = detectBoth;    break;
    default:
        THROW(ProcessingError, "Unsupported edge trigger type");
    }

    // Clear events
    m_Events.clear();

    // First sample
    if (func(0, m_State, a_Ptr[0])) {
        m_Events.push_back(Event(0, a_Ptr[0]));
    }

    // Subsequent samples
    for (size_t i=1; i<a_Length; ++i) {
        if(func(i, a_Ptr[0], a_Ptr[1])) {
            m_Events.push_back(Event(i, a_Ptr[1]));
        }
        ++a_Ptr;
    }

    // Store last state
    m_State = a_Ptr[-1];

    return m_Events;
}

// ============================================================================

}; // Processing
}; // Graph

