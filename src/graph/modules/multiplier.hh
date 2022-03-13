#ifndef GRAPH_MODULES_MULTIPLIER_HH
#define GRAPH_MODULES_MULTIPLIER_HH

#include "../module.hh"

#include <string>
#include <vector>

#include <cstddef>
#include <cstdint>

namespace Graph {
namespace Modules {

// ============================================================================

class Multiplier : public Module {
public:

    /// Constructor
    Multiplier (
        const std::string& a_Name,
        const Module::Attributes& a_Attributes = Module::Attributes()
    );

    /// Creates an instance
    static Module* create (
        const std::string& a_Type,
        const std::string& a_Name,
        const Module::Attributes& a_Attributes = Module::Attributes()
    );
    
    /// Called on the graph initialization
    void prepare (float a_SampleRate, size_t a_BufferSize) override;

    /// Processes a single audio buffer
    void process () override;

protected:

    /// Output port
    Port* m_Output;
    /// Input ports
    std::vector<Port*> m_Inputs;

    /// Common gain
    const Parameter* m_Gain;
    /// Separate bias for each input
    std::vector<Parameter*> m_Bias;
};

// ============================================================================

}; // Modules
}; // Graph

#endif // GRAPH_MODULES_MULTIPLIER_HH

