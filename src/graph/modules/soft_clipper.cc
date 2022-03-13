#include "soft_clipper.hh"
#include <utils/math.hh>

#include <cmath>

namespace Graph {
namespace Modules {

using namespace Utils;

// ============================================================================

SoftClipper::SoftClipper (const std::string& a_Name,
                          const Module::Attributes& a_Attributes) :
    Module ("softClipper", a_Name, a_Attributes)
{
    // Input ports
    m_Input  = addPort(new Port(this, "in",    Port::Direction::INPUT, 0.0f));
    m_Level  = addPort(new Port(this, "level", Port::Direction::INPUT, 0.0f));

    // Output port
    m_Output = addPort(new Port(this, "out",   Port::Direction::OUTPUT));
}

Module* SoftClipper::create (
    const std::string& a_Type,
    const std::string& a_Name,
    const Module::Attributes& a_Attributes)
{
    (void)a_Type;
    return new SoftClipper(a_Name, a_Attributes);
}

// ============================================================================

inline float softClip (float x, float level) {
    const float k = level * 1.5f;

    if (x < -k) {
        return -level;
    }
    else if (x > +k) {
        return +level;
    }
    else {
        return x - (k / 3.0f) * (x / k) * (x / k) * (x / k);
    }
}

void SoftClipper::process () {

    // Get pointers
    const float* ptrIn    = m_Input->process().data();
    const float* ptrLevel = m_Level->process().data();
    float*       ptrOut   = m_Output->getBuffer().data();

    // Process
    for (size_t i=0; i<m_BufferSize; ++i) {

        // Get clipping level and convert to linear scale
        float level = *ptrLevel++;
        level = Math::log2lin(level);

        // Do the clipping
        *ptrOut++ = softClip(*ptrIn++, level);
    }
}

// ============================================================================

}; // Modules
}; // Graph

