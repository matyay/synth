#ifndef GRAPH_MODULES_VCF_HH
#define GRAPH_MODULES_VCF_HH

#include "../module.hh"
#include "../processing/biquad_iir.hh"

#include <string>

#include <cstddef>
#include <cstdint>

namespace Graph {
namespace Modules {

// ============================================================================

class VCF : public Module {
public:

    /// Constructor
    VCF (
        const std::string& a_Name,
        const Module::Attributes& a_Attributes = Module::Attributes()
    );

    /// Creates an instance
    static Module* create (
        const std::string& a_Type,
        const std::string& a_Name,
        const Module::Attributes& a_Attributes = Module::Attributes()
    );

    /// Called on start
    void start () override;

    /// Processes a single audio buffer
    void process () override;

protected:

    /// The filter
    Processing::BiquadIIR m_Filter;

    /// Last input state
    struct {
        int32_t type;
        float   cv;
        float   gain;
        float   q;
    } m_InputState;

    /// Input ports
    Port* m_Input;
    Port* m_Freq;
    Port* m_Gain;
    Port* m_Q;

    /// Output port
    Port* m_Output;
};

// ============================================================================

}; // Modules
}; // Graph

#endif // GRAPH_MODULES_VGA_HH

