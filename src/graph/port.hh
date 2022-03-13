#ifndef GRAPH_PORT_HH
#define GRAPH_PORT_HH

#include <audio/buffer.hh>

#include <string>
#include <vector>

#include <cstddef>
#include <cstdint>

namespace Graph {

// ============================================================================
class Module;

// ============================================================================

/// Base module port class
class Port
{
public:

    /// Port directions
    enum class Direction {
        INPUT,
        OUTPUT
    };

    /// Port type
    enum class Type {
        BUFFERED,
        PROXY
    };

    /// Constructor (Buffered)
    Port (Module* a_Module, const std::string& a_Name,
          Direction a_Direction);

    /// Constructor (Proxy)
    Port (Module* a_Module, const std::string& a_Name,
          Direction a_Direction, float a_Default);

    /// Returns the owner module
    Module* getModule () const;
    /// Returns port name
    std::string getName () const;
    /// Returns a full hierarchical name
    std::string getFullName () const;
    /// Returns port direction
    Direction getDirection () const;
    /// Returns port type
    Type getType () const;

    /// Returns true when the port is connected
    bool isConnected ();

    /// Returns the buffer dirty flag.
    bool isDirty    ();
    /// Sets the buffer dirty flag.
    void setDirty   (bool a_Propagate = true);
    /// Clears the buffer dirty flag.
    void clearDirty ();

    /// Returns the buffer associated with the port.
    Audio::Buffer<float>& getBuffer ();
    /// Returns buffer associated with the port. Invokes processing in all
    /// upstream modules if necessary.
    const Audio::Buffer<float>& process ();

protected:

    /// Updates source and sink lists
    void updateSourcesAndSinks ();
    /// Sets a new audio buffer to be associated with the port
    void setBuffer (const Audio::Buffer<float>& a_Buffer);

    // ....................................................
    
    /// The owner module.
    Module* m_Module = nullptr;

    /// Port name
    const std::string m_Name;
    /// Port direction
    const Direction m_Direction;
    /// Port type
    const Type m_Type;

    /// Default signal value
    const float m_Default;
    /// Audio buffer
    Audio::Buffer<float> m_Buffer;
    /// Dirty flag
    bool m_IsDirty = true;

    /// Connected source port (upstream)
    Port* m_SourcePort = nullptr;
    /// Connected sink ports (downstream)
    std::vector<Port*> m_SinkPorts;

    friend class Module;
};


// ============================================================================

}; // Graph

#endif // GRAPH_PORT_HH

