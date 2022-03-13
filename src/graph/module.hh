#ifndef GRAPH_MODULE_HH
#define GRAPH_MODULE_HH

#include "port.hh"
#include "parameter.hh"

#include <utils/dict.hh>
#include <utils/base_interface.hh>

#include <memory>
#include <string>

#include <cstddef>
#include <cstdint>

namespace Graph {

// ============================================================================

class Module : virtual public IBaseInterface {
public:

    /// Attributes type
    typedef Dict<std::string, std::string> Attributes;
    /// Parameters type
    typedef Dict<std::string, Parameter> Parameters;
    /// Parameter values type
    typedef Dict<std::string, Parameter::Value> ParameterValues;
    /// Ports type
    typedef Dict<std::string, std::shared_ptr<Port>> Ports;
    /// Submodules type
    typedef Dict<std::string, std::shared_ptr<Module>> Submodules;

    /// Constructor
    Module (
        const std::string& a_Type,
        const std::string& a_Name,
        const Attributes& a_Attributes = Attributes()
    );

    /// Queries an interface with the given id. Returns nullptr if not
    /// implemented by the derived class.
    virtual IBaseInterface* queryInterface (iid_t a_Id) override;

    /// Returns sample rate
    float getSampleRate () const;
    /// Returns buffer size
    size_t getBufferSize () const;

    /// Returns the module type
    std::string getType () const;
    /// Returns the module name
    std::string getName () const;
    /// Returns the full module hierarchical name
    std::string getFullName () const;

    /// Returns port map
    const Ports& getPorts ();
    /// Returns port map
    const Ports& getPorts () const;
    /// Returns port with the given name
    Port* getPort (const std::string& a_Name);

    /// Returns connections
    const Dict<Port*, Port*> getConnections () const;

    /// Returns the parent module
    Module* getParent () const;

    /// Returns true when the module is a leaf (has no submodules)
    bool isLeaf () const;
    /// Adds a submodule
    void addSubmodule (Module* a_Module);
    /// Returns the submodule map
    const Submodules& getSubmodules ();
    /// Returns the submodule map
    const Submodules& getSubmodules () const;
    /// Returns a submodule with the given name
    Module* getSubmodule (const std::string& a_Name);

    /// Called on the graph initialization
    virtual void prepare (float a_SampleRate, size_t a_BufferSize);
    /// Called on audio processing start
    virtual void start   ();
    /// Called on audio processing stop
    virtual void stop    ();

    /// Processes a single audio buffer
    virtual void process ();

    /// Returns module attributes
    const Attributes getAttributes () const;
    /// Returns module parameters
    const Parameters getParameters () const;
    /// Updates module parameters
    virtual void updateParameters (const ParameterValues& a_Values);

protected:

    /// Adds a new port, returns a pointer to it
    Port* addPort (Port* a_Port);
    /// Connects two ports. Either a submodule output to a submodule input or
    /// top-level input/output to a submodule input/output.
    void connect (Port* a_Src, Port* a_Dst);

    /// Applies parameter overrides
    void applyParameterOverrides (const Module::Attributes& a_Overrides);

    // ....................................................

    /// Type
    const std::string m_Type;
    /// Name
    const std::string m_Name;

    /// Attributes (immutable)
    const Attributes m_Attributes;
    /// Parameters (mutable)
    Parameters m_Parameters;

    /// Top-level ports
    Ports m_Ports;

    /// Parent module or nullptr
    Module* m_Parent = nullptr;
    /// Submodules
    Submodules m_Submodules;

    /// Connections. Associates each port with its upstream connection
    Dict<Port*, Port*> m_Connections;

    /// Sample rate
    float  m_SampleRate = 0.0f;
    /// Buffer size
    size_t m_BufferSize = 0;

    friend class Port;
    friend class Builder;
};

// ============================================================================

}; // Graph
#endif // GRAPH_MODULE_HH
