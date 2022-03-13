#ifndef GRAPH_MODULES_SAMPLER_HH
#define GRAPH_MODULES_SAMPLER_HH

#include "../module.hh"
#include "../processing/sampler.hh"

#include <string>

#include <cstddef>
#include <cstdint>

namespace Graph {
namespace Modules {

// ============================================================================

class Sampler : public Module {
public:

    /// Constructor
    Sampler (
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
    virtual void prepare (float a_SampleRate, size_t a_BufferSize);
    /// Called on processing start
    virtual void start   () override;

    /// Processes a single audio buffer
    virtual void process () override;

protected:

    /// Sampler
    Graph::Processing::Sampler m_Sampler;
    /// Base frequency
    float m_BaseFreq;
    /// Current phase accumulator
    float m_Phase = 0.0f;

    /// Frequency (CV) input
    Port*  m_CvIn;
    /// AM input
    Port*  m_AmIn;
    /// FM input
    Port*  m_FmIn;

    /// Output
    Port* m_Output;
};

// ============================================================================

}; // Modules
}; // Graph

#endif // GRAPH_MODULES_SAMPLER_HH

