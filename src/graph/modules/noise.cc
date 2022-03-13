#include "noise.hh"
#include "../parameter.hh"
#include <utils/math.hh>

#include <chrono>

namespace Graph {
namespace Modules {

using namespace Utils;

// ============================================================================

Noise::Noise (const std::string& a_Name,
              const Module::Attributes& a_Attributes) :
    Module ("noise", a_Name, a_Attributes)
{
    // Output port
    m_Output = addPort(new Port(this, "out", Port::Direction::OUTPUT));

    // Attributes
    m_Seed = std::stoi(a_Attributes.get("seed", "-1"));

    // Parameters
    m_Parameters.set("amplitude", Parameter(-6.0, -30.0, 0.0f, 0.1f, "Amplitude [dB]"));

    // Apply overrides
    applyParameterOverrides(a_Attributes);
}

Module* Noise::create (
    const std::string& a_Type,
    const std::string& a_Name,
    const Module::Attributes& a_Attributes)
{
    (void)a_Type;
    return new Noise(a_Name, a_Attributes);
}

// ============================================================================

void Noise::start () {

    // Initialize with the default seed
    if (m_Seed == 0) {
        m_Gen = std::mt19937(std::mt19937::default_seed);
    }
    // Initialize with the current time
    else if (m_Seed < 0) {
        auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        m_Gen = std::mt19937(seed);
    }
    // Initialize with the specific seed
    else {
        std::mt19937::result_type seed = m_Seed;
        m_Gen = std::mt19937(seed);
    }
}

// ============================================================================

void Noise::process () {

    // Amplitude
    float A = m_Parameters.get("amplitude").get().asNumber();
    A = Math::log2lin(A);

    // Get pointers
    float* ptr = m_Output->getBuffer().data();

    // White noise
    for (size_t i=0; i<m_BufferSize; ++i) {
        float r = (float)m_Gen() / (float)(1UL << 31) - 1.0f;
        *ptr++ = A * r;
    }
}

// ============================================================================

}; // Modules
}; // Graph

