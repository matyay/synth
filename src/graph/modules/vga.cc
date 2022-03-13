#include "vga.hh"
#include <utils/math.hh>
#include <utils/utils.hh>

#include <cmath>

namespace Graph {
namespace Modules {

using namespace Utils;

// ============================================================================

VGA::VGA (const std::string& a_Name,
          const Module::Attributes& a_Attributes) :
    Module ("vga", a_Name, a_Attributes)
{
    // Input ports
    m_Input  = addPort(new Port(this, "in"  , Port::Direction::INPUT, 0.0f));
    m_Gain   = addPort(new Port(this, "gain", Port::Direction::INPUT, 0.0f));

    // Output port
    m_Output = addPort(new Port(this, "out",  Port::Direction::OUTPUT));

    // Cutoff gain
    float cutoffLevel = Utils::stof(a_Attributes.get("cutoff", "-96.0"));
    m_Cutoff = Math::log2lin(cutoffLevel);
}

Module* VGA::create (
    const std::string& a_Type,
    const std::string& a_Name,
    const Module::Attributes& a_Attributes)
{
    (void)a_Type;
    return new VGA(a_Name, a_Attributes);
}

// ============================================================================

void VGA::process () {

    // Process
    const float* ptrIn   = m_Input->process().data();
    const float* ptrGain = m_Gain->process().data();
    float*       ptrOut  = m_Output->getBuffer().data();

    for (size_t i=0; i<m_BufferSize; ++i) {
        float k = Math::log2lin(*ptrGain++);
        if (k <= m_Cutoff) k = 0.0f;
        *ptrOut++ = *ptrIn++ * k;
    }
}

// ============================================================================

}; // Modules
}; // Graph

