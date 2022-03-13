#ifndef GRAPH_MODULES_CONSTANT_HH
#define GRAPH_MODULES_CONSTANT_HH

#include "../module.hh"
#include "../parameter.hh"

#include <string>

#include <cstddef>
#include <cstdint>

namespace Graph {
namespace Modules {

// ============================================================================

class Constant : public Module {
public:

    /// Constructor
    Constant (
        const std::string& a_Name,
        const Module::Attributes& a_Attributes = Module::Attributes()
    );

    /// Creates an instance
    static Module* create (
        const std::string& a_Type,
        const std::string& a_Name,
        const Module::Attributes& a_Attributes = Module::Attributes()
    );

    /// Processes a single audio buffer
    void process () override;

protected:

    /// Output port
    Port* m_Output;

    /// Control parameter
    const Parameter* m_Value = nullptr;
};

// ============================================================================

}; // Modules
}; // Graph

#endif // GRAPH_MODULES_CONSTANT_HH

