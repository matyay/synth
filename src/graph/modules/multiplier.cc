#include "multiplier.hh"

#include <stringf.hh>

namespace Graph {
namespace Modules {

// ============================================================================

Multiplier::Multiplier (const std::string& a_Name,
                        const Module::Attributes& a_Attributes) :
    Module ("multiplier", a_Name, a_Attributes)
{
    // Input ports
    size_t numInputs = std::stoi(a_Attributes.get("numInputs", "2"));    
    for (size_t i=0; i<numInputs; ++i) {
        const std::string name = stringf("in%d", i);
        auto port = new Port(this, name, Port::Direction::INPUT, 0.0f);
        m_Inputs.push_back(addPort(port));
    }

    // Output port
    m_Output = addPort(new Port(this, "out", Port::Direction::OUTPUT));

    // Gain parameter
    m_Parameters.set("gain", Parameter(1.0f, 0.0f, +1.0f, 0.01f, "Gain"));
    m_Gain = &m_Parameters.get("gain");

    // Bias parameters
    for (size_t i=0; i<numInputs; ++i) {
        const std::string name = stringf("bias%d", i);
        const std::string desc = stringf("Bias %d", i);

        m_Parameters.set(name, Parameter(0.0f, -1.0f, +1.0f, 0.01f, desc));
        m_Bias.push_back(&m_Parameters.get(name));
    }

    // Apply overrides
    applyParameterOverrides(a_Attributes);
}

Module* Multiplier::create (
    const std::string& a_Type,
    const std::string& a_Name,
    const Module::Attributes& a_Attributes)
{
    (void)a_Type;
    return new Multiplier(a_Name, a_Attributes);
}

// ============================================================================

void Multiplier::prepare (float a_SampleRate, size_t a_BufferSize) {

    // Call the base method
    Module::prepare(a_SampleRate, a_BufferSize);

    // Lock parameters for unconnected ports
    for (size_t i=0; i<m_Inputs.size(); ++i) {
        if (!m_Inputs[i]->isConnected()) {
            m_Bias[i]->setLock(true);
        }
    }
}

// ============================================================================

void Multiplier::process () {

    // Clear output buffer
    m_Output->getBuffer().fill(m_Gain->get().asNumber());

    // Process
    float* ptrOut = m_Output->getBuffer().data();

    for (size_t j=0; j<m_Inputs.size(); ++j) {
        float bias = m_Bias[j]->get().asNumber();
        auto  port = m_Inputs[j];

        const float* ptrIn = port->process().data();
        for (size_t i=0; i<m_BufferSize; ++i) {
            ptrOut[i] *= (ptrIn[i] + bias);
        }
    }
}

// ============================================================================

}; // Modules
}; // Graph

