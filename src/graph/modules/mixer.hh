#ifndef GRAPH_MODULES_MIXER_HH
#define GRAPH_MODULES_MIXER_HH

#include "../module.hh"

#include <string>
#include <vector>

#include <cstddef>
#include <cstdint>

namespace Graph {
namespace Modules {

// ============================================================================

class Mixer : public Module {
public:

    /// Constructor
    Mixer (
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

    /// Gain for each input in dB
    std::vector<Parameter*> m_Gain;
};

// ============================================================================

}; // Modules
}; // Graph

#endif // GRAPH_MODULES_MIXER_HH

