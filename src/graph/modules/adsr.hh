#ifndef GRAPH_MODULES_ADSR_HH
#define GRAPH_MODULES_ADSR_HH

#include "envelope.hh"

namespace Graph {
namespace Modules {

// ============================================================================

class ADSR : public Envelope {
public:

    /// Constructor
    ADSR (const std::string& a_Name,
          const Attributes& a_Attributes = Attributes());

    /// Creates an instance
    static Module* create (
        const std::string& a_Type,
        const std::string& a_Name,
        const Attributes& a_Attributes = Attributes()
    );

    /// Updates module parameters
    void updateParameters (const ParameterValues& a_Values) override;

protected:

    /// Updates the envelope points
    void updateEnvelope ();
};

// ============================================================================

}; // Modules
}; // Graph

#endif // GRAPH_MODULES_ADSR_HH

