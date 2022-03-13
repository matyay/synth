#ifndef GRAPH_MODULES_ENVELOPE_HH
#define GRAPH_MODULES_ENVELOPE_HH

#include "../module.hh"

#include <memory>
#include <string>
#include <deque>

#include <cstddef>
#include <cstdint>

namespace Graph {
namespace Modules {

// ============================================================================

class Envelope : public Module {
public:

    /// An envelope point
    struct Point {
        float   time;       // Time [seconds]
        float   level;      // Level
        bool    isSustain;  // True when the point is a sustain point

        Point (float t, float l, bool s = false) :
            time      (t),
            level     (l),
            isSustain (s) {}

        Point (const Point& ref) = default;
    };

    /// Constructor
    Envelope (
        const std::string& a_Type,
        const std::string& a_Name,
        const Module::Attributes& a_Attributes = Module::Attributes()
    );

    /// Creates an instance
    static Module* create (
        const std::string& a_Type,
        const std::string& a_Name,
        const Module::Attributes& a_Attributes = Module::Attributes()
    );

    /// Called on processing start
    void start () override;
    /// Called on processing stop
    void stop  () override;

    /// Processes a single audio buffer
    void process () override;

protected:

    // Event
    struct Event {
        int32_t time;       // Time [samples]
        float   level;      // Level
        bool    isSustain;  // True when the point is a sustain point

        Event (int32_t t, const Point& p) : 
            time      (t),
            level     (p.level),
            isSustain (p.isSustain)
            {};

        Event (const Event& ref) = default;
    };

    /// Checks if envelope points are sane. Throws an exception if they are not
    void sanityCheckPoints ();

    /// Schedule envelope curve events
    void scheduleEvents (int32_t a_Sample, bool a_Attack);
    /// Progress to the next event
    void nextEvent ();

    /// Gate input
    Port* m_Gate;
    /// Amplitude output
    Port* m_Output;

    /// Envelope points
    std::vector<Point> m_Points;

    /// Envelope event times. An event correspons to an envelope point
    std::deque<Event>  m_Events;

    /// Gate input state
    float   m_GateState   = 0.0f;

    /// Activity
    bool    m_IsActive    = false;
    /// Current level
    double  m_CurrLevel   = 0.0;
    /// Level delta
    double  m_LevelDelta  = 0.0;
};

// ============================================================================

}; // Modules
}; // Graph

#endif // GRAPH_MODULES_ENVELOPE_HH

