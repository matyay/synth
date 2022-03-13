#include "../graph.hh"
#include "constant.hh"

namespace Graph {
namespace Modules {

// ============================================================================

Constant::Constant (const std::string& a_Name,
                    const Module::Attributes& a_Attributes) :
    Module ("constant", a_Name, a_Attributes)
{
    // Output port
    m_Output = addPort(new Port(this, "out", Port::Direction::OUTPUT));

    // The parameter
    m_Parameters.set("value", Parameter(0.0f, 0.0f, 1.0f, 0.01f, "Value"));
    m_Value = &m_Parameters.get("value");

    // Apply overrides
    applyParameterOverrides(a_Attributes);
}

Module* Constant::create (
    const std::string& a_Type,
    const std::string& a_Name,
    const Module::Attributes& a_Attributes)
{
    (void)a_Type;
    return new Constant(a_Name, a_Attributes);
}

// ============================================================================

void Constant::process () {

    // Fill output buffer with the constant value
    auto& buffer = m_Output->getBuffer();
    buffer.fill(m_Value->get().asNumber());
}

// ============================================================================

}; // Modules
}; // Graph

