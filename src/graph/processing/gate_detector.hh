#ifndef GRAPH_PROCESSING_GATE_DETECTOR_HH
#define GRAPH_PROCESSING_GATE_DETECTOR_HH

#include <vector>

#include <cstddef>
#include <cstdint>

namespace Graph {
namespace Processing {

// ============================================================================

class GateDetector {
public:

    /// Edge type
    enum class Edge {
        RISING,
        FALLING,
        BOTH
    };

    /// Event
    struct Event {
        size_t  time;   /// Event time [samples]
        float   value;  /// Gate signal value after the event

        Event (size_t t, float v) : time(t), value(v) {};
        Event (const Event& ref) = default;        
    };

    /// Constructor
    GateDetector (Edge a_Edge = Edge::RISING, float a_Threshold = 0.5f);

    /// Sets new threshold
    void setThreshold (float a_Threshold);
    /// Resets internal state
    void reset (float a_Level = 0.0f);
    /// Returns the event list
    const std::vector<Event>& getEvents () const;

    /// Processes a buffer, returns a list of events
    const std::vector<Event>& process (const float* a_Ptr, size_t a_Length);

private:

    /// Edge type
    Edge  m_Edge;
    /// Threshold
    float m_Threshold;

    /// Last gate state
    float m_State = 0.0f;

    /// Event list
    std::vector<Event> m_Events;
};

// ============================================================================

}; // Processing
}; // Graph

#endif // GRAPH_PROCESSING_GATE_DETECTOR_HH

