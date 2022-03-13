#include "graph.hh"
#include "port.hh"
#include "module.hh"

#include <utils/exception.hh>
#include <stringf.hh>

#include <unordered_set>
#include <algorithm>
#include <stdexcept>

#include <cassert>

namespace Graph {

// ============================================================================

Port::Port (Module* a_Module, const std::string& a_Name, Direction a_Direction) :
    m_Module    (a_Module),
    m_Name      (a_Name),
    m_Direction (a_Direction),
    m_Type      (Type::BUFFERED),
    m_Default   (0.0f)
{
    assert(a_Module != nullptr);
}

Port::Port (Module* a_Module, const std::string& a_Name, Direction a_Direction, float a_Default) :
    m_Module    (a_Module),
    m_Name      (a_Name),
    m_Direction (a_Direction),
    m_Type      (Type::PROXY),
    m_Default   (a_Default)
{
    assert(a_Module != nullptr);
}

// ============================================================================

Module* Port::getModule () const {
    return m_Module;
}

std::string Port::getName () const {
    return m_Name;
}

std::string Port::getFullName () const {
    return m_Module->getFullName() + "." + m_Name;
}

Port::Direction Port::getDirection () const {
    return m_Direction;
}

Port::Type Port::getType () const {
    return m_Type;
}

// ============================================================================

bool Port::isConnected () {
    return (m_SourcePort != nullptr) || (!m_SinkPorts.empty());
}

bool Port::isDirty () {

    // Buffered port
    if (m_Type == Type::BUFFERED) {
        return m_IsDirty;
    }

    // Proxy port
    else if (m_Type == Type::PROXY) {

        // Return dirty flag of the upstream port
        if (m_SourcePort != nullptr) {
            return m_SourcePort->isDirty();
        }

        // Not connected so never dirty
        return false;
    }

    return false;
}

void Port::setDirty (bool a_Propagate) {

    // Already set
    if (isDirty()) {
        return;
    }

    // Buffered port
    if (m_Type == Type::BUFFERED) {

        // Set the local flag
        m_IsDirty = true;

        // Walk upstream over all input ports of the module
        if (a_Propagate) {
            for (auto& it : m_Module->getPorts()) {
                auto& port = it.second;
                if (port->getDirection() == Direction::INPUT) {
                    port->setDirty(true);
                }
            }
        }
    }

    // Proxy port, propagate the call
    else if (m_Type == Type::PROXY) {

        if (m_SourcePort != nullptr) {
            m_SourcePort->setDirty(a_Propagate);
        }
    }
}

void Port::clearDirty () {

    // Clear the local dirty flag
    if (m_Type == Type::BUFFERED) {
        m_IsDirty = false;
    }
}

// ============================================================================

void Port::updateSourcesAndSinks () {

    m_SourcePort = nullptr;
    m_SinkPorts.clear();

    // The port is proxy, find its source
    if (m_Type == Type::PROXY) {

        // Walk upstream
        Port* port = this;
        while (1) {

            // Get connected upstream port
            Module* module = port->getModule();
            Port*   next   = module->m_Connections.get(port, nullptr);
            if (next == nullptr) {
                Module* parent = module->getParent();
                if (parent != nullptr) {
                    next = parent->m_Connections.get(port, nullptr);
                }
            }

            // No more expansion possible
            if (next == nullptr) {
                break;
            }

            // Got a buffered port, that's the one.
            if (next->getType() == Type::BUFFERED) {
                m_SourcePort = next;
                break;
            }

            // Continue
            port = next;
        }
    }

    // Recursive downstream walk function.
    std::function<void(Port*)> walkDownstream = [&](Port* port) {

        // Get connected downstream ports
        std::unordered_set<Port*> ports;

        Module* module = port->getModule();
        for (auto& it : module->m_Connections) {
            if (it.second == port) {
                ports.insert(it.first);
            }
        }

        module = module->getParent();
        if (module != nullptr) {
            for (auto& it : module->m_Connections) {
                if (it.second == port) {
                    ports.insert(it.first);
                }
            }
        }

        // Explore them
        for (auto& next : ports) {
            assert(next->getType() == Type::PROXY);

            // Got an input of a leaf module, store it.
            if (next->getModule()->isLeaf()) {
                m_SinkPorts.push_back(next);
            }
            // A non-leaf module, explore further
            else {
                walkDownstream(next);
            }
        }
    };

    // Start walking
    walkDownstream(this);
}

void Port::setBuffer (const Audio::Buffer<float>& a_Buffer) {

    // Set the buffer
    m_Buffer = a_Buffer;

    // For proxy port fill it with the default value
    if (m_Type == Type::PROXY) {
        m_Buffer.fill(m_Default);
    }
}

// ============================================================================

Audio::Buffer<float>& Port::getBuffer () {

    // Buffered port
    if (m_Type == Type::BUFFERED) {
        return m_Buffer;
    }

    // Proxy port
    else if (m_Type == Type::PROXY) {

        // Return buffer of the connected port
        if (m_SourcePort != nullptr) {
            return m_SourcePort->getBuffer();
        }

        // Not connected, return own buffer with constant data
        return m_Buffer;
    }

    // Will not happen but makes the compiler happy
    return m_Buffer;
}

const Audio::Buffer<float>& Port::process () {

    // Buffered port
    if (m_Type == Type::BUFFERED) {

        // We need to update the buffer
        if (m_IsDirty) {

            // Do the processing on the module
            m_Module->process();
            // Clear the dirty flag
            m_IsDirty = false;
        }

        return m_Buffer;
    }

    // Proxy port
    else if (m_Type == Type::PROXY) {

        // Process the buffer on the connected upstream port
        if (m_SourcePort != nullptr) {
            return m_SourcePort->process();
        }

        // Not connected, return own buffer with constant data
        return m_Buffer;
    }

    // Will not happen but makes the compiler happy
    return m_Buffer;
}

// ============================================================================
}; // Graph


