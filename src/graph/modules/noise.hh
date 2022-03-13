#ifndef GRAPH_MODULES_NOISE_HH
#define GRAPH_MODULES_NOISE_HH

#include "../module.hh"

#include <string>
#include <random>

#include <cstddef>
#include <cstdint>

namespace Graph {
namespace Modules {

// ============================================================================

class Noise : public Module {
public:

    /// Constructor
    Noise (
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
    virtual void start () override;

    /// Processes a single audio buffer
    void process () override;

protected:

    /// Random number generator
    std::mt19937 m_Gen;
    /// Random generator seed. If set to -1 then the current timestamp is used
    int32_t      m_Seed = -1;

    /// Output port
    Port* m_Output;
};

// ============================================================================

}; // Modules
}; // Graph

#endif // GRAPH_MODULES_NOISE_HH

