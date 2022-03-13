#ifndef GRAPH_MODULES_SOFT_CLIPPER_HH
#define GRAPH_MODULES_SOFT_CLIPPER_HH

#include "../module.hh"

#include <string>

#include <cstddef>
#include <cstdint>

namespace Graph {
namespace Modules {

// ============================================================================

class SoftClipper : public Module {
public:

    /// Constructor
    SoftClipper (
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

    /// Input ports
    Port* m_Input;
    Port* m_Level;

    /// Output port
    Port* m_Output;
};

// ============================================================================

}; // Modules
}; // Graph

#endif // GRAPH_MODULES_SOFT_CLIPPER_HH
